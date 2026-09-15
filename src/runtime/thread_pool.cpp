#include <flex/runtime/thread_pool.hpp>
#include <flex/runtime/group_checkpoint.hpp>
#include <flex/runtime/solution_output.hpp>
#include <flex/search_algorithm/bfs_step.hpp>
#include <flex/search_algorithm/apply.hpp>
#include <flex/modeling/encoding.hpp>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stack>
#include <exception>

namespace flex::runtime {
namespace fs = std::filesystem;

ThreadPool::ThreadPool(Options options) : options_(std::move(options)) {
    std::size_t nthreads = options_.num_threads;
    if (nthreads == 0) nthreads = std::thread::hardware_concurrency();
    if (nthreads == 0) nthreads = 4;
    workers_.reserve(nthreads);
    for (std::size_t i = 0; i < nthreads; ++i) {
        workers_.emplace_back([this]() { this->worker_loop(); });
    }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::enqueue(Task&& t) {
    {
        std::lock_guard<std::mutex> lk(m_);
        queue_.push(std::move(t));
        ++active_tasks_;
    }
    cv_.notify_one();
}

void ThreadPool::wait_all() {
    std::unique_lock<std::mutex> lk(m_done_);
    cv_done_.wait(lk, [this] { return active_tasks_.load() == 0; });
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lk(m_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& th : workers_) {
        if (th.joinable()) th.join();
    }
    workers_.clear();
}

std::uint64_t ThreadPool::solutions_written() const {
    return solutions_written_.load(std::memory_order_relaxed);
}

std::uint64_t ThreadPool::group_files_written() const {
    return group_files_written_.load(std::memory_order_relaxed);
}

std::uint64_t ThreadPool::error_count() const {
    return error_count_.load(std::memory_order_relaxed);
}

std::string ThreadPool::last_error() const {
    std::lock_guard<std::mutex> lk(error_mu_);
    return last_error_;
}

bool ThreadPool::gzip_available() {
    int rc = std::system("gzip --version > /dev/null 2>&1");
    return rc == 0;
}

static void record_error(std::atomic<std::uint64_t>& error_count,
                         std::mutex& error_mu,
                         std::string& last_error,
                         const std::string& msg) {
    error_count.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lk(error_mu);
    last_error = msg;
}

static void record_current_exception(std::atomic<std::uint64_t>& error_count,
                                     std::mutex& error_mu,
                                     std::string& last_error,
                                     const char* context) {
    try {
        throw;
    } catch (const std::exception& e) {
        record_error(error_count, error_mu, last_error, std::string(context) + ": " + e.what());
    } catch (...) {
        record_error(error_count, error_mu, last_error, std::string(context) + ": unknown exception");
    }
}

void ThreadPool::worker_loop() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] { return stop_ || !queue_.empty(); });
            if (stop_ && queue_.empty()) return;
            task = std::move(queue_.front());
            queue_.pop();
        }
        try {
            run_task(std::move(task));
        } catch (...) {
            record_current_exception(error_count_, error_mu_, last_error_, "worker_loop/run_task");
        }
        if (--active_tasks_ == 0) {
            std::lock_guard<std::mutex> lk(m_done_);
            cv_done_.notify_all();
        }
    }
}

void ThreadPool::run_task(Task&& t) {
    using search_algorithm::StepOutput;
    using search_algorithm::StepResult;

    std::vector<modeling::TruthTable> local_solutions;
    std::stack<LevelFrame> level_stack;
    level_stack.push(std::move(t.frame));

    std::size_t local_memory_usage = 0;
    bool task_failed = false;

    const bool at_group_level = (t.depth_offset == options_.parallel_branch_depth);
    std::string group_key = t.group_key;
    std::string status_path;
    GroupStatus status;

    auto mark_failed = [&]() { task_failed = true; };

    try {
        std::size_t part_index = 0;

        if (at_group_level) {
            if (group_key.empty()) {
                group_key = key_string_from_path(options_.parallel_branch_depth, t.path_key);
            }
            fs::create_directories(options_.solution_dir);
            status_path = (fs::path(options_.solution_dir) / (group_key + ".status.txt")).string();
            status = load_status(status_path);
            if (status.done) {
                std::uint64_t done = completed_groups_.fetch_add(1, std::memory_order_relaxed) + 1;
                std::uint64_t total = total_groups_.load(std::memory_order_relaxed);
                std::cerr << "[progress] level " << options_.parallel_branch_depth
                          << " " << done << "/" << total << " (skip done " << group_key << ")\n";
                return;
            }
            remove_group_part_files(options_.solution_dir, group_key, options_.use_gzip);
            status = {};
            save_status_atomic(status_path, status);
        }

        auto flush_now = [&](bool force) -> bool {
            if (local_solutions.empty()) return true;
            if (!force && local_memory_usage < options_.flush_threshold_bytes) return true;
            if (error_count_.load(std::memory_order_relaxed) > 0) {
                mark_failed();
                return false;
            }

            fs::create_directories(options_.solution_dir);

            std::string base_name;
            if (at_group_level) {
                std::ostringstream fname;
                fname << group_key << "_part_" << part_index << file_suffix(options_.use_gzip);
                base_name = (fs::path(options_.solution_dir) / fname.str()).string();
            } else {
                std::ostringstream fname;
                fname << "group_" << t.group_id
                      << "_task_" << t.id
                      << "_part_" << part_index
                      << file_suffix(options_.use_gzip);
                base_name = (fs::path(options_.solution_dir) / fname.str()).string();
            }

            std::string output_error;
            if (!write_solution_part_file(base_name, local_solutions, options_.use_gzip, output_error)) {
                record_error(error_count_, error_mu_, last_error_, output_error);
                mark_failed();
                return false;
            }

            if (at_group_level) {
                status.solutions += static_cast<std::uint64_t>(local_solutions.size());
                save_status_atomic(status_path, status);
            }

            solutions_written_.fetch_add(static_cast<std::uint64_t>(local_solutions.size()), std::memory_order_relaxed);
            group_files_written_.fetch_add(1, std::memory_order_relaxed);
            local_solutions.clear();
            local_memory_usage = 0;
            ++part_index;
            return true;
        };

        while (!level_stack.empty()) {
            if (error_count_.load(std::memory_order_relaxed) > 0) {
                mark_failed();
                break;
            }

            LevelFrame& current_level_frame = level_stack.top();
            StepOutput step_output = search_algorithm::bfs_step(
                current_level_frame,
                t.bits,
                t.internal_to_output,
                t.external_state_candidates);
            // if success, add to local solutions, flush, backtrack
            if (step_output.result == StepResult::SUCCESS) {
                local_solutions.push_back(current_level_frame.truth_table);
                local_memory_usage += current_level_frame.truth_table.size() * sizeof(InternalState);
                if (!flush_now(false)) break;
                level_stack.pop();
                search_algorithm::backtrack_to_next_choice(level_stack);
                continue;
            }
            // if fail, backtrack
            if (step_output.result == StepResult::FAILURE) {
                level_stack.pop();
                search_algorithm::backtrack_to_next_choice(level_stack);
                continue;
            }
            // if assumptions needed, spawn child tasks until parallel depth, otherwise continue locally
            if (step_output.result == StepResult::ASSUMPTION_NEEDED) {
                const auto& cands = step_output.new_candidates;
                std::size_t current_depth_global =
                    t.depth_offset + ((level_stack.size() > 0) ? level_stack.size() - 1 : 0);
                if (current_depth_global >= options_.parallel_branch_depth) {
                    if (current_level_frame.candidates.empty()) {
                        current_level_frame.current_search_index = 0;
                        current_level_frame.candidates = cands;
                    }
                    if (!search_algorithm::spawn_child_from_parent(level_stack)) {
                        level_stack.pop();
                        search_algorithm::backtrack_to_next_choice(level_stack);
                    }
                    continue;
                }

                for (std::size_t i = 0; i < cands.size(); ++i) {
                    LevelFrame child = current_level_frame;
                    child.current_search_index = 0;
                    child.candidates.clear();
                    if (search_algorithm::apply_assumption(child, cands[i])) {
                        Task child_task;
                        child_task.id = next_task_id_++;
                        child_task.frame = std::move(child);
                        child_task.bits = t.bits;
                        child_task.internal_to_output = t.internal_to_output;
                        child_task.external_state_candidates = t.external_state_candidates;

                        std::uint32_t child_depth_global = static_cast<std::uint32_t>(current_depth_global + 1);
                        child_task.depth_offset = child_depth_global;
                        child_task.path_key = t.path_key;
                        child_task.path_key.push_back(static_cast<std::uint16_t>(i));

                        if (child_depth_global == options_.parallel_branch_depth) {
                            child_task.group_id = next_group_id_++;
                            total_groups_.fetch_add(1, std::memory_order_relaxed);
                            child_task.group_key = key_string_from_path(options_.parallel_branch_depth, child_task.path_key);
                        } else {
                            child_task.group_id = t.group_id;
                        }

                        enqueue(std::move(child_task));
                    }
                }
                break;
            }
        }

        if (!flush_now(true)) {
            mark_failed();
        }

        if (at_group_level && !task_failed && error_count_.load(std::memory_order_relaxed) == 0) {
            status.done = true;
            save_status_atomic(status_path, status);
        }
    } catch (...) {
        mark_failed();
        record_current_exception(error_count_, error_mu_, last_error_, "ThreadPool::run_task");
    }

    if (t.depth_offset == options_.parallel_branch_depth) {
        std::uint64_t done = completed_groups_.fetch_add(1, std::memory_order_relaxed) + 1;
        std::uint64_t total = total_groups_.load(std::memory_order_relaxed);
        std::cerr << "[progress] level " << options_.parallel_branch_depth
                  << " " << done << "/" << total
                  << (at_group_level ? (" [" + group_key + "]") : "") << "\n";
    }
}

}
