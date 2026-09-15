#include <flex/search_algorithm/bfs_step.hpp>
#include <flex/modeling/encoding.hpp>
#include <flex/search_algorithm/path_finder.hpp>

namespace flex::search_algorithm {

StepOutput bfs_step(runtime::LevelFrame& level_frame,
                    const modeling::StateBits& bits,
                    const std::vector<OutputState>& internal_to_output,
                    const std::vector<std::vector<ExternalState>>& external_state_candidates) {

    // if queue is empty, it means that this frame has completed validation process without any conflict or contradiction, 
    // so we can consider this frame as successfully finished and return SUCCESS.
    if (level_frame.bfs_queue.empty()) {
        return {StepResult::SUCCESS, {}};
    }

    search_space::SystemState current_state = level_frame.bfs_queue.front();
    level_frame.bfs_queue.pop();

    // if the current state is aleady in reachable set, we can skip this because this frame is aleady validated in this context.
    if (level_frame.reachable_set.count(current_state) != 0) {
        return {StepResult::CONTINUE, {}};
    }

    // to record validation progress for current system state, we maintain cursor.
    const std::uint32_t key_global = modeling::encode_system_state(bits, current_state);
    std::size_t& cur = level_frame.ext_cursor[key_global];

    const std::uint32_t key_io = (static_cast<std::uint32_t>(current_state.input_state) << bits.output_bits) |
                                 static_cast<std::uint32_t>(current_state.output_state);
    const std::size_t num_cands = bits.input_bits;

    const TruthTableIndex tt_index = modeling::is_to_index(bits, current_state.input_state, current_state.internal_state);

    // stability must be checked at previous bfs step, 
    // however, check stability condition again here for safety. especially corner case.

    // if system stability condition is not filled, fill the truth table with stability condition.
    if (level_frame.truth_table[tt_index] == DONT_CARE) {
        level_frame.truth_table[tt_index] = current_state.internal_state;

    // if system is unstable, we can consider this frame as failed.
    } else if (level_frame.truth_table[tt_index] != current_state.internal_state) {
        return {StepResult::FAILURE, {}};
    }

    if (cur >= num_cands) {
        level_frame.reachable_set.insert(current_state);
        return {StepResult::CONTINUE, {}};
    }

    // Specify validation object for current bfs step by checking current cursor position, and find all possible assumptions for current bfs step.
    ExternalState next_external_state = external_state_candidates[key_io][cur];
    InputState next_input_state = static_cast<InputState>(next_external_state >> bits.output_bits);
    OutputState next_output_state = static_cast<OutputState>(next_external_state & bits.output_mask());

    std::vector<search_space::Assumption> valid_assumptions;

    const InternalState internal_size = static_cast<InternalState>(1u << bits.internal_bits);

    // for all next internal state possibility, check if it can lead to valid assumpition without conflict.
    for (InternalState next_internal_state = 0; next_internal_state < internal_size; ++next_internal_state) {
        if (next_internal_state >= internal_to_output.size()) break;

        // if output state does not match with internal_to_output mapping, this next internal state is not possible skip it
        if (internal_to_output[next_internal_state] != next_output_state) {
            continue;
        }

        search_space::SystemState next_state{next_input_state, next_internal_state, next_output_state};
        search_space::Transition current_transition{current_state, next_state};

        auto all_paths = find_all_paths(bits, current_transition, internal_to_output, level_frame.path_options);

        for (const auto& path : all_paths) {
            search_space::Assumption assumption;
            assumption.transition = current_transition;
            bool conflict = false;
            
            // Transition condition
            for (std::size_t step = 0; step + 1 < path.size(); ++step) {
                const auto& from_state = path[step];
                const auto& to_state = path[step + 1];
                TruthTableIndex req_index = modeling::is_to_index(bits, from_state.input_state, from_state.internal_state);
                InternalState req_next_internal_state = to_state.internal_state;
                assumption.required_rows.push_back({req_index, req_next_internal_state});
                if (level_frame.truth_table[req_index] != DONT_CARE &&
                    level_frame.truth_table[req_index] != req_next_internal_state) {
                    conflict = true;
                    break;
                }
            }

            // Stability condition
            const auto& final_state = path.back();
            TruthTableIndex req_index = modeling::is_to_index(bits, final_state.input_state, final_state.internal_state);
            InternalState req_next_internal_state = final_state.internal_state;
            assumption.required_rows.push_back({req_index, req_next_internal_state});
            if (level_frame.truth_table[req_index] != DONT_CARE &&
                level_frame.truth_table[req_index] != req_next_internal_state) {
                conflict = true;
            }
            
            // for assumptions without conflict, we can consider them as valid assumptions for next exploration
            if (!conflict) {
                valid_assumptions.push_back(std::move(assumption));
            }
        }
    }

    ++cur;

    // Need further assumption to completely validate current state, so push again else, mark as finished
    if (cur < num_cands) {
        level_frame.bfs_queue.push(current_state);
    } else {
        level_frame.reachable_set.insert(current_state);
    }

    // If # assumptions: 0 => Fail, 1 => apply and continue, 2~ => branch
    if (valid_assumptions.empty()) {
        return {StepResult::FAILURE, {}};
    }
    if (valid_assumptions.size() == 1) {
        const auto& assumption = valid_assumptions.front();
        for (const auto& [idx, state] : assumption.required_rows) {
            level_frame.truth_table[idx] = state;
        }
        level_frame.bfs_queue.push(assumption.transition.to);
        return {StepResult::CONTINUE, {}};
    }

    return {StepResult::ASSUMPTION_NEEDED, std::move(valid_assumptions)};
}

}
