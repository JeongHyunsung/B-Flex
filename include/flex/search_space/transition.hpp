#pragma once
#include <cstdint>
#include <vector>
#include <flex/common/defs.hpp>

namespace flex::search_space {

struct SystemState {
    InputState input_state = 0;
    InternalState internal_state = 0;
    OutputState output_state = 0;
};

inline bool operator==(const SystemState& a, const SystemState& b) {
    return a.input_state == b.input_state &&
           a.internal_state == b.internal_state &&
           a.output_state == b.output_state;
}
inline bool operator!=(const SystemState& a, const SystemState& b) {
    return !(a == b);
}
inline bool operator<(const SystemState& a, const SystemState& b) {
    if (a.input_state != b.input_state) return a.input_state < b.input_state;
    if (a.internal_state != b.internal_state) return a.internal_state < b.internal_state;
    return a.output_state < b.output_state;
}

struct Transition {
    SystemState from;
    SystemState to;
};

}
