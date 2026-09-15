#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <flex/search_algorithm/path_finder.hpp>

namespace flex::engine {

struct Options {
    std::uint32_t parallel_branch_depth = 4;
    std::uint64_t flush_threshold_bytes = 500ull * 1024 * 1024;
    std::string solution_dir = "solutions_out";
    std::size_t num_threads = 0;
    bool use_gzip = true;
    bool deterministic = false;
    search_algorithm::PathOptions path_options;
};

}
