#pragma once
#include <cstdint>
#include <flex/common/defs.hpp>
#include <flex/modeling/states.hpp>
#include <flex/search_space/transition.hpp>

namespace flex::modeling {

inline std::uint32_t encode_system_state(const StateBits& bits, const search_space::SystemState& s) {
    return (static_cast<std::uint32_t>(s.input_state) << (bits.internal_bits + bits.output_bits)) |
           (static_cast<std::uint32_t>(s.internal_state) << bits.output_bits) |
           static_cast<std::uint32_t>(s.output_state);
}

inline search_space::SystemState decode_system_state(const StateBits& bits, std::uint32_t code) {
    search_space::SystemState s;
    s.input_state = static_cast<InputState>((code >> (bits.internal_bits + bits.output_bits)) & bits.input_mask());
    s.internal_state = static_cast<InternalState>((code >> bits.output_bits) & bits.internal_mask());
    s.output_state = static_cast<OutputState>(code & bits.output_mask());
    return s;
}

inline TruthTableIndex is_to_index(const StateBits& bits, InputState input_state, InternalState internal_state) {
    TruthTableIndex idx = 0;
    idx |= static_cast<TruthTableIndex>(input_state) << bits.internal_bits;
    idx |= static_cast<TruthTableIndex>(internal_state);
    return idx;
}

}
