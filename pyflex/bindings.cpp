#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <flex/common/errors.hpp>
#include <flex/engine/engine.hpp>
#include <flex/modeling/problem.hpp>
#include <flex/search_space/transition.hpp>

namespace py = pybind11;

PYBIND11_MODULE(pyflex, m) {
    m.doc() = "B-Flex Python bindings (C++ core async FSM truth-table search)";

    auto py_flex_error = py::register_exception<flex::FlexError>(m, "FlexError");
    py::register_exception<flex::InvalidConfig>(m, "InvalidConfig", py_flex_error.ptr());
    py::register_exception<flex::Infeasible>(m, "Infeasible", py_flex_error.ptr());
    py::register_exception<flex::RuntimeError>(m, "RuntimeError", py_flex_error.ptr());

    py::class_<flex::modeling::StateBits>(m, "StateBits")
        .def(py::init<>())
        .def_readwrite("input_bits", &flex::modeling::StateBits::input_bits)
        .def_readwrite("internal_bits", &flex::modeling::StateBits::internal_bits)
        .def_readwrite("output_bits", &flex::modeling::StateBits::output_bits);

    py::class_<flex::search_space::SystemState>(m, "SystemState")
        .def(py::init<>())
        .def_readwrite("input_state", &flex::search_space::SystemState::input_state)
        .def_readwrite("internal_state", &flex::search_space::SystemState::internal_state)
        .def_readwrite("output_state", &flex::search_space::SystemState::output_state);

    py::class_<flex::modeling::Problem>(m, "Problem")
        .def(py::init<>())
        .def_readwrite("bits", &flex::modeling::Problem::bits)
        .def_readwrite("internal_to_output", &flex::modeling::Problem::internal_to_output)
        .def_readwrite("external_state_candidates", &flex::modeling::Problem::external_state_candidates)
        .def_readwrite("initial_state", &flex::modeling::Problem::initial_state);

    py::class_<flex::engine::Options>(m, "Options")
        .def(py::init<>())
        .def_readwrite("parallel_branch_depth", &flex::engine::Options::parallel_branch_depth)
        .def_readwrite("flush_threshold_bytes", &flex::engine::Options::flush_threshold_bytes)
        .def_readwrite("solution_dir", &flex::engine::Options::solution_dir)
        .def_readwrite("num_threads", &flex::engine::Options::num_threads)
        .def_readwrite("use_gzip", &flex::engine::Options::use_gzip)
        .def_readwrite("deterministic", &flex::engine::Options::deterministic);

    py::class_<flex::engine::Results>(m, "Results")
        .def_readonly("solutions_written", &flex::engine::Results::solutions_written)
        .def_readonly("group_files_written", &flex::engine::Results::group_files_written)
        .def_readonly("solution_dir", &flex::engine::Results::solution_dir)
        .def_readonly("error_count", &flex::engine::Results::error_count);

    py::class_<flex::engine::Engine>(m, "Engine")
        .def(py::init<flex::engine::Options>(), py::arg("options") = flex::engine::Options{})
        .def("run", &flex::engine::Engine::run, py::arg("problem"));

    m.attr("__doc_constraints__") =
        "StateBits: input_bits<=16, internal_bits<=15, output_bits<=15 (0xFFFF reserved sentinel). "
        "If use_gzip=True, gzip must be available on PATH.";
}
