#pragma once
#include <cstdint>
#include <cstddef>

namespace flex::modeling {

struct StateBits {
    std::uint8_t input_bits = 0;
    std::uint8_t internal_bits = 0;
    std::uint8_t output_bits = 0;

    constexpr std::uint32_t input_mask() const {
        return (input_bits == 32) ? 0xFFFFFFFFu : ((1u << input_bits) - 1u);
    }
    constexpr std::uint32_t internal_mask() const {
        return (internal_bits == 32) ? 0xFFFFFFFFu : ((1u << internal_bits) - 1u);
    }
    constexpr std::uint32_t output_mask() const {
        return (output_bits == 32) ? 0xFFFFFFFFu : ((1u << output_bits) - 1u);
    }
    constexpr std::uint32_t truth_table_size() const {
        return 1u << (input_bits + internal_bits);
    }
    constexpr bool valid() const {
        return input_bits > 0 && internal_bits > 0 && output_bits > 0 &&
               (input_bits + internal_bits + output_bits) < 32;
    }
};

}
