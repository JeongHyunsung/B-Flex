#include <flex/search_algorithm/path_finder.hpp>
#include <flex/common/bitops.hpp>
#include <limits>
#include <functional>

namespace flex::search_algorithm {

std::vector<std::vector<search_space::SystemState>>
find_all_paths(const modeling::StateBits& bits,
               const search_space::Transition& tr,
               const std::vector<OutputState>& internal_to_output,
               const PathOptions& options) {
    const InputState fixed_input = tr.to.input_state;
    const InternalState start_int = tr.from.internal_state;
    const InternalState end_int = tr.to.internal_state;

    if (start_int >= internal_to_output.size() || end_int >= internal_to_output.size()) return {};

    const OutputState start_out = internal_to_output[start_int];
    const OutputState end_out = internal_to_output[end_int];

    if (tr.from.output_state != start_out) return {};
    if (tr.to.output_state != end_out) return {};

    const int min_steps = options.min_steps < 0 ? 0 : options.min_steps;
    const int max_steps = options.max_steps < 0 ? std::numeric_limits<int>::max() : options.max_steps;
    const int max_step_h = options.max_step_hamming < 0 ? static_cast<int>(bits.internal_bits) : options.max_step_hamming;

    std::vector<std::vector<search_space::SystemState>> all_paths;

    const std::uint32_t internal_size = 1u << bits.internal_bits;
    std::vector<char> visited(internal_size, 0);
    visited[start_int] = 1;

    std::vector<search_space::SystemState> path;
    path.push_back({fixed_input, start_int, start_out});

    auto for_each_neighbor_1flip = [&](InternalState u, auto&& fun) {
        for (std::uint8_t i = 0; i < bits.internal_bits; ++i) {
            fun(static_cast<InternalState>(u ^ (InternalState(1) << i)));
        }
    };

    auto for_each_neighbor = [&](InternalState u, auto&& fun) {
        if (max_step_h == 1) {
            for_each_neighbor_1flip(u, fun);
            return;
        }
        const InternalState N = static_cast<InternalState>(internal_size);
        if (max_step_h >= static_cast<int>(bits.internal_bits)) {
            for (InternalState v = 0; v < N; ++v) {
                if (v != u) fun(v);
            }
            return;
        }
        const InternalState all_mask = static_cast<InternalState>(N - 1);
        for (InternalState mask = 1; mask <= all_mask; ++mask) {
            if (flex::popcnt(mask) > max_step_h) continue;
            InternalState v = static_cast<InternalState>(u ^ mask);
            if (v != u) fun(v);
        }
    };

    std::function<void(InternalState, OutputState, int, int)> dfs =
        [&](InternalState u, OutputState last_out, int depth, int out_toggles) {
            if (u == end_int) {
                if (out_toggles <= 1 && depth >= min_steps) all_paths.push_back(path);
                return;
            }
            if (depth >= max_steps) return;

            const int dist = flex::hamming(u, end_int);

            for_each_neighbor(u, [&](InternalState v) {
                if (visited[v]) return;
                if (options.monotone_only && flex::hamming(v, end_int) >= dist) return;

                if (v >= internal_to_output.size()) return;
                OutputState v_out = internal_to_output[v];
                int toggles = out_toggles + (v_out != last_out ? 1 : 0);
                if (toggles > 1) return;

                visited[v] = 1;
                path.push_back({fixed_input, v, v_out});
                dfs(v, v_out, depth + 1, toggles);
                path.pop_back();
                visited[v] = 0;
            });
        };

    dfs(start_int, start_out, 0, 0);
    return all_paths;
}

}
