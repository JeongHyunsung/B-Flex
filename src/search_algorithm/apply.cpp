#include <flex/search_algorithm/apply.hpp>

namespace flex::search_algorithm {

bool apply_assumption(runtime::LevelFrame& level_frame, const search_space::Assumption& assumption) {
    // Check if assumption is appliable, and if so, apply it to the current level frame
    for (const auto& [idx, state] : assumption.required_rows) {
        if (level_frame.truth_table[idx] != DONT_CARE && level_frame.truth_table[idx] != state) {
            return false;
        }
    }
    for (const auto& [idx, state] : assumption.required_rows) {
        level_frame.truth_table[idx] = state;
    }
    level_frame.bfs_queue.push(assumption.transition.to);
    return true;
}

bool spawn_child_from_parent(std::stack<runtime::LevelFrame>& level_stack) {
    // Check if we can spawn a child frame from the current parent frame by applying one of its candidate assumptions
    // After discover one success assumption, push to stack and immediately return true.
    if (level_stack.empty()) return false;
    runtime::LevelFrame& parent_frame = level_stack.top();
    if (parent_frame.candidates.empty()) return false;

    for (std::size_t i = parent_frame.current_search_index; i < parent_frame.candidates.size(); ++i) {
        runtime::LevelFrame child_frame = parent_frame;
        child_frame.current_search_index = 0;
        child_frame.candidates.clear();

        if (apply_assumption(child_frame, parent_frame.candidates[i])) {
            level_stack.push(child_frame);
            parent_frame.current_search_index = i;
            return true;
        }
    }
    return false;
}

bool backtrack_to_next_choice(std::stack<runtime::LevelFrame>& level_stack) {
    // Backtrack to the next untried assumption in the parent frame if we can
    while (!level_stack.empty()) {
        runtime::LevelFrame& parent_frame = level_stack.top();
        if (!parent_frame.candidates.empty()) {
            std::size_t next_index = parent_frame.current_search_index + 1;
            while (next_index < parent_frame.candidates.size()) {
                runtime::LevelFrame child_frame = parent_frame;
                child_frame.candidates.clear();
                child_frame.current_search_index = 0;

                if (apply_assumption(child_frame, parent_frame.candidates[next_index])) {
                    level_stack.push(child_frame);
                    parent_frame.current_search_index = next_index;
                    return true;
                }
                ++next_index;
            }
            level_stack.pop();
        } else {
            level_stack.pop();
        }
    }
    return false;
}

}
