#pragma once
#include <stack>
#include <flex/search_space/assumption.hpp>
#include <flex/runtime/level_frame.hpp>

namespace flex::search_algorithm {

bool apply_assumption(runtime::LevelFrame& level_frame, const search_space::Assumption& assumption);
bool spawn_child_from_parent(std::stack<runtime::LevelFrame>& level_stack);
bool backtrack_to_next_choice(std::stack<runtime::LevelFrame>& level_stack);

}
