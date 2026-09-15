#pragma once
#include <vector>
#include <cstdint>
#include <flex/common/defs.hpp>
#include <flex/modeling/states.hpp>
#include <flex/search_space/transition.hpp>

namespace flex::modeling {

struct Problem {
    StateBits bits;
    std::vector<OutputState> internal_to_output;
    std::vector<std::vector<ExternalState>> external_state_candidates;
    search_space::SystemState initial_state;
};

}
