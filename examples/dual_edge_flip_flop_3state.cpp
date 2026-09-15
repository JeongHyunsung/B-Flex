#include <iostream>
#include <vector>

#include <flex/engine/engine.hpp>
#include <flex/modeling/problem.hpp>

int main() {
    using namespace flex;

    modeling::Problem problem;
    problem.bits = {2, 3, 1};

    problem.internal_to_output = {
        1, 0, 1, 0, 1, 0, 1, 0
    };

    problem.external_state_candidates = {
        {2, 4},
        {2, 5},
        {0, 6},
        {1, 7},
        {0, 7},
        {1, 7},
        {2, 4},
        {3, 5}
    };

    problem.initial_state = {1, 3, 0};

    engine::Options options;
    options.parallel_branch_depth = 4;
    options.flush_threshold_bytes = 500ull * 1024ull * 1024ull;
    options.solution_dir = "solutions_out_cpp_example_3state";
    options.num_threads = 0;
    options.use_gzip = true;
    options.deterministic = false;
    options.path_options.min_steps = -1;
    options.path_options.max_steps = -1;
    options.path_options.monotone_only = false;
    options.path_options.max_step_hamming = 1;

    engine::Engine engine(options);
    auto results = engine.run(problem);

    std::cout << "Solutions written: " << results.solutions_written << "\n";
    std::cout << "Group files written: " << results.group_files_written << "\n";
    std::cout << "Solution dir: " << results.solution_dir << "\n";

    return 0;
}
