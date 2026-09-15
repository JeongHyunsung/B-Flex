#pragma once 
#include <cstdint>
#include <cstddef>

namespace flex{

using InputState = std::uint16_t;
using InternalState = std::uint16_t;
using OutputState = std::uint16_t;
using ExternalState = std::uint32_t; // (input, output) tuple
using TruthTableIndex = std::uint32_t; // (input, state) tuple

constexpr InternalState DONT_CARE = static_cast<InternalState>(0xFFFFu);

}
