#include <flex/engine/engine.hpp>
#include <flex/common/errors.hpp>
#include <flex/modeling/truth_table.hpp>
#include <flex/runtime/thread_pool.hpp>

namespace flex::engine {

Engine::Engine(Options options) : options_(std::move(options)) {}

static void validate_problem(const modeling::Problem& problem) {
    // StateBits validation
    if (!problem.bits.valid()) {
        throw InvalidConfig("StateBits is invalid");
    }
    if (problem.bits.input_bits > 16 || problem.bits.internal_bits > 15 || problem.bits.output_bits > 15) {
        throw InvalidConfig("bit width exceeds limit (input<=16, internal/output<=15; 0xFFFF is reserved)");
    }
    const std::size_t internal_size = 1u << problem.bits.internal_bits;
    const std::size_t io_size = 1u << (problem.bits.input_bits + problem.bits.output_bits);

    // Problem input data structure validation
    if (problem.internal_to_output.size() != internal_size) {
        throw InvalidConfig("internal_to_output size mismatch");
    }
    if (problem.external_state_candidates.size() != io_size) {
        throw InvalidConfig("external_state_candidates size mismatch");
    }

    // Initial state validation
    if (problem.initial_state.input_state >= (1u << problem.bits.input_bits) ||
        problem.initial_state.internal_state >= (1u << problem.bits.internal_bits) ||
        problem.initial_state.output_state >= (1u << problem.bits.output_bits)) {
        throw InvalidConfig("initial_state out of range");
    }
    if (problem.internal_to_output[problem.initial_state.internal_state] != problem.initial_state.output_state) {
        throw InvalidConfig("initial_state output does not match internal_to_output");
    }

    // Problem external state candidate validation
    for (const auto& row : problem.external_state_candidates) {
        if (row.size() != problem.bits.input_bits) {
            throw InvalidConfig("external_state_candidates row size mismatch");
        }
        for (ExternalState ext : row) {
            InputState in = static_cast<InputState>(ext >> problem.bits.output_bits);
            OutputState out = static_cast<OutputState>(ext & problem.bits.output_mask());
            if (in >= (1u << problem.bits.input_bits) || out >= (1u << problem.bits.output_bits)) {
                throw InvalidConfig("external_state_candidates has out-of-range value");
            }
        }
    }
}

Results Engine::run(const modeling::Problem& problem) {
    validate_problem(problem);

    runtime::ThreadPool::Options rt_opts;
    rt_opts.parallel_branch_depth = options_.parallel_branch_depth;
    rt_opts.flush_threshold_bytes = options_.flush_threshold_bytes;
    rt_opts.solution_dir = options_.solution_dir;
    rt_opts.num_threads = options_.num_threads;
    rt_opts.use_gzip = options_.use_gzip;

    if (options_.deterministic) {
        rt_opts.num_threads = 1;
        rt_opts.parallel_branch_depth = 0;
    }
    if (rt_opts.use_gzip && !runtime::ThreadPool::gzip_available()) {
        throw InvalidConfig("gzip is required but not available on PATH");
    }

    // Initialize thread pool with options
    runtime::ThreadPool pool(rt_opts);

    // Create root level frame 
    runtime::LevelFrame root;
    root.truth_table = modeling::make_truth_table(problem.bits);
    root.bfs_queue.push(problem.initial_state);
    root.path_options = options_.path_options;

    // Create root task (including root frame)
    runtime::Task root_task;
    root_task.id = 0;
    root_task.group_id = 0;
    root_task.depth_offset = 0;
    root_task.frame = std::move(root);
    root_task.bits = problem.bits;
    root_task.internal_to_output = problem.internal_to_output;
    root_task.external_state_candidates = problem.external_state_candidates;

    // Enqueue root task and wait for completion
    pool.enqueue(std::move(root_task));
    pool.wait_all();
    pool.shutdown();

    // Get result and return
    Results results;
    results.solutions_written = pool.solutions_written();
    results.solution_dir = options_.solution_dir;
    results.error_count = pool.error_count();

    results.group_files_written = pool.group_files_written();
    if (results.error_count > 0) {
        throw RuntimeError(pool.last_error());
    }
    return results;
}

}
