#pragma once
#include <vector>
#include <utility>
#include <flex/common/defs.hpp>
#include <flex/search_space/transition.hpp>

namespace flex::search_space {

struct Assumption {
    Transition transition;
    std::vector<std::pair<TruthTableIndex, InternalState>> required_rows;
};

}
