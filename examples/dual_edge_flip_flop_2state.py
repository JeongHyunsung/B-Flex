from pyflex import Engine, Options, Problem, StateBits, SystemState


def main() -> None:
    p = Problem()
    p.bits = StateBits()
    p.bits.input_bits = 2
    p.bits.internal_bits = 2
    p.bits.output_bits = 1

    p.internal_to_output = [1, 0, 1, 0]
    p.external_state_candidates = [
        [2, 4],
        [2, 5],
        [0, 6],
        [1, 7],
        [0, 7],
        [1, 7],
        [2, 4],
        [3, 5],
    ]

    p.initial_state = SystemState()
    p.initial_state.input_state = 1
    p.initial_state.internal_state = 3
    p.initial_state.output_state = 0

    opt = Options()
    opt.parallel_branch_depth = 4
    opt.flush_threshold_bytes = 500 * 1024 * 1024
    opt.solution_dir = "solutions_out_py_example_2state"
    opt.num_threads = 0
    opt.use_gzip = True
    opt.deterministic = False
    # NOTE: path_options is not exposed in current pyflex bindings.

    eng = Engine(opt)
    res = eng.run(p)

    print("solutions_written =", res.solutions_written)
    print("group_files_written =", res.group_files_written)
    print("solution_dir =", res.solution_dir)


if __name__ == "__main__":
    main()
