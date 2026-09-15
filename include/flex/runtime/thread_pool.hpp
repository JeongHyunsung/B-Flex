#pragma once
#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <string>
#include <flex/runtime/task.hpp>

namespace flex::runtime {

class ThreadPool {
public:
    struct Options {
        std::uint32_t parallel_branch_depth = 4;
        std::uint64_t flush_threshold_bytes = 500ull * 1024 * 1024;
        std::string solution_dir = "solutions_out";
        std::size_t num_threads = 0;
        bool use_gzip = true;
    };

    explicit ThreadPool(Options options);
    ~ThreadPool();

    void enqueue(Task&& t);
    void wait_all();
    void shutdown();
    std::uint64_t solutions_written() const;
    std::uint64_t group_files_written() const;
    std::uint64_t error_count() const;
    std::string last_error() const;

    static bool gzip_available();

private:
    void worker_loop();
    void run_task(Task&& t);

    Options options_;

    std::vector<std::thread> workers_;
    std::queue<Task> queue_;
    std::mutex m_;
    std::condition_variable cv_;

    std::atomic<bool> stop_{false};
    std::atomic<std::uint64_t> active_tasks_{0};
    std::mutex m_done_;
    std::condition_variable cv_done_;

    std::atomic<std::uint64_t> next_task_id_{1};
    std::atomic<std::uint64_t> next_group_id_{1};
    std::atomic<std::uint64_t> total_groups_{0};
    std::atomic<std::uint64_t> completed_groups_{0};
    std::atomic<std::uint64_t> solutions_written_{0};
    std::atomic<std::uint64_t> group_files_written_{0};
    std::atomic<std::uint64_t> error_count_{0};
    mutable std::mutex error_mu_;
    std::string last_error_;
};

}
