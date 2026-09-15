#pragma once
#include <vector>
#include <flex/common/defs.hpp>
#include <flex/modeling/states.hpp>
#include <flex/search_space/transition.hpp>

namespace flex::search_algorithm {

struct PathOptions {
    int min_steps = -1;
    int max_steps = -1;
    bool monotone_only = false;
    int max_step_hamming = 1;
};

std::vector<std::vector<search_space::SystemState>>
find_all_paths(const modeling::StateBits& bits,
               const search_space::Transition& tr,
               const std::vector<OutputState>& internal_to_output,
               const PathOptions& options);

}
