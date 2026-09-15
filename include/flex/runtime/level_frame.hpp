#pragma once
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>
#include <flex/common/defs.hpp>
#include <flex/modeling/truth_table.hpp>
#include <flex/search_space/assumption.hpp>
#include <flex/search_space/transition.hpp>
#include <flex/search_algorithm/path_finder.hpp>

namespace flex::runtime {

struct LevelFrame {
    std::queue<search_space::SystemState> bfs_queue;
    std::set<search_space::SystemState> reachable_set;
    modeling::TruthTable truth_table;
    std::unordered_map<std::uint32_t, std::size_t> ext_cursor;
    std::size_t current_search_index = 0;
    std::vector<search_space::Assumption> candidates;
    search_algorithm::PathOptions path_options;
};

}
