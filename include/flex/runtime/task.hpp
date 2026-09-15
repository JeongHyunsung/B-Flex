#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <flex/modeling/states.hpp>
#include <flex/runtime/level_frame.hpp>

namespace flex::runtime {

struct Task {
    std::uint64_t id = 0;
    std::uint64_t group_id = 0;
    std::uint32_t depth_offset = 0;
    LevelFrame frame;
    modeling::StateBits bits;
    std::vector<OutputState> internal_to_output;
    std::vector<std::vector<ExternalState>> external_state_candidates;

    std::vector<std::uint16_t> path_key;
    std::string group_key;
};

}
