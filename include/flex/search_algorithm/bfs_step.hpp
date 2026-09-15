#pragma once
#include <vector>
#include <flex/common/defs.hpp>
#include <flex/modeling/states.hpp>
#include <flex/runtime/level_frame.hpp>
#include <flex/search_space/assumption.hpp>

namespace flex::search_algorithm {

enum class StepResult {
    CONTINUE,
    ASSUMPTION_NEEDED,
    FAILURE,
    SUCCESS
};

struct StepOutput {
    StepResult result;
    std::vector<search_space::Assumption> new_candidates;
};

StepOutput bfs_step(runtime::LevelFrame& level_frame,
                    const modeling::StateBits& bits,
                    const std::vector<OutputState>& internal_to_output,
                    const std::vector<std::vector<ExternalState>>& external_state_candidates);

}
