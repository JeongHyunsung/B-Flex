#pragma once
#include <vector>
#include <flex/common/defs.hpp>
#include <flex/modeling/states.hpp>

namespace flex::modeling {

using TruthTable = std::vector<InternalState>;

inline TruthTable make_truth_table(const StateBits& bits) {
    return TruthTable(bits.truth_table_size(), DONT_CARE);
}

}
