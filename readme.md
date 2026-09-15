<p align="center">
  <img src="docs/logo.png" alt="B-Flex" width="420">
</p>


<p align="center">
B-Flex: Exploration of Broader Flip-Flop Design Space Based on FSM Exhaustive Search (DAC'26)
</p>

## About

B-Flex exhaustively searches **Asynchronous FSM** space for a **given sequential element specification**, and reports back every internal-state truth table that actually satisfies it. The core search engine (`flex`) is written in C++17 and runs a thread-pool-based parallel search; `pyflex` wraps the same engine for Python via pybind11.

- Primary supported platform: Linux (Ubuntu 22.04+ recommended)
- CI-tested compilers: GCC, Clang

## Quick Start

Build and test:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run the bundled C++ example:

```bash
./build/example_dual_edge_flip_flop_3state
```

Note: the bundled examples run a full exhaustive search, not a quick smoke test. Expect it to take several minutes to tens of minutes depending on hardware, and to write on the order of gigabytes of solutions under `solutions_out*/`.

## Python (`pyflex`)

Install from a local checkout:

```bash
python3 -m pip install -e .
```

Install directly from GitHub:

```bash
python3 -m pip install "git+https://github.com/JeongHyunsung/B-Flex.git"
```

Run the bundled Python example:

```bash
python3 examples/dual_edge_flip_flop_3state.py
```

## Constraints

- `input_bits <= 16`
- `internal_bits <= 15`
- `output_bits <= 15`
- `input_bits + internal_bits + output_bits < 32`

`0xFFFF` is reserved as the `DONT_CARE` sentinel.

## Output Files

See `docs/output_format_spec.md` for part-file and status-file details.

## Citation

Citation details will be added once the paper is publicly available.

## License

Released under the MIT License — see [LICENSE](LICENSE) for the full text.
