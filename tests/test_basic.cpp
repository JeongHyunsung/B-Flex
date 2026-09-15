// keep assert() active even when built with NDEBUG (Release/RelWithDebInfo), otherwise CI checks nothing
#undef NDEBUG
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <flex/common/errors.hpp>
#include <flex/engine/engine.hpp>
#include <flex/modeling/encoding.hpp>
#include <flex/modeling/problem.hpp>
#include <flex/runtime/thread_pool.hpp>
#include <flex/search_algorithm/path_finder.hpp>

namespace fs = std::filesystem;

namespace {

flex::modeling::Problem make_example_problem() {
    using namespace flex;
    modeling::Problem problem;
    problem.bits = {1, 1, 1};
    problem.internal_to_output = {0, 1};
    problem.external_state_candidates = {
        {0},
        {1},
        {2},
        {3},
    };
    problem.initial_state = {0, 0, 0};
    return problem;
}

fs::path make_temp_dir(const std::string& tag) {
    static std::uint64_t counter = 0;
    const auto stamp = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    fs::path p = fs::temp_directory_path() /
                 ("bflex_" + tag + "_" + std::to_string(stamp) + "_" + std::to_string(++counter));
    fs::create_directories(p);
    return p;
}

std::vector<fs::path> list_part_files(const fs::path& dir, const std::string& suffix) {
    std::vector<fs::path> out;
    if (!fs::exists(dir)) return out;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        const auto name = e.path().filename().string();
        if (name.find("_part_") == std::string::npos) continue;
        if (name.size() >= suffix.size() && name.rfind(suffix) == name.size() - suffix.size()) {
            out.push_back(e.path());
        }
    }
    return out;
}

std::string slurp(const fs::path& p) {
    std::ifstream fin(p, std::ios::binary);
    std::ostringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

void remove_tree_if_exists(const fs::path& p) {
    std::error_code ec;
    fs::remove_all(p, ec);
}

void test_encoding_and_path_finder_smoke() {
    using namespace flex;

    // Encoding round-trip
    {
        modeling::StateBits bits{1, 2, 1};
        search_space::SystemState s{1, 2, 1};
        auto code = modeling::encode_system_state(bits, s);
        auto dec = modeling::decode_system_state(bits, code);
        assert(dec.input_state == s.input_state);
        assert(dec.internal_state == s.internal_state);
        assert(dec.output_state == s.output_state);
    }

    // Path finder minimal test
    {
        modeling::StateBits bits{1, 2, 1};
        std::vector<OutputState> internal_to_output = {0, 1, 0, 1};
        search_space::Transition tr;
        tr.from = {0, 0, 0};
        tr.to = {0, 1, 1};

        search_algorithm::PathOptions opts;
        opts.min_steps = 1;
        opts.max_steps = 1;
        opts.monotone_only = false;
        opts.max_step_hamming = 1;

        auto paths = search_algorithm::find_all_paths(bits, tr, internal_to_output, opts);
        assert(paths.size() == 1);
        assert(paths[0].size() == 2);
        assert(paths[0][0].internal_state == 0);
        assert(paths[0][1].internal_state == 1);
    }
}

void test_problem_validation_errors() {
    using namespace flex;
    modeling::Problem p;
    p.bits = {1, 1, 1};
    p.internal_to_output = {0};
    p.external_state_candidates = {{0}, {1}, {2}, {3}};
    p.initial_state = {0, 0, 0};

    engine::Engine eng;
    bool threw = false;
    try {
        (void)eng.run(p);
    } catch (const InvalidConfig&) {
        threw = true;
    }
    assert(threw && "InvalidConfig expected for internal_to_output size mismatch");
}

void test_plaintext_output_and_format() {
    using namespace flex;

    const auto problem = make_example_problem();
    const fs::path dir = make_temp_dir("plain");

    engine::Options options;
    options.deterministic = true;
    options.use_gzip = false;
    options.solution_dir = dir.string();

    engine::Engine eng(options);
    const auto res = eng.run(problem);

    assert(res.error_count == 0);
    assert(res.solutions_written > 0);
    assert(res.group_files_written > 0);

    const auto part_files = list_part_files(dir, ".txt");
    assert(!part_files.empty());
    assert(part_files.size() == res.group_files_written);

    std::ifstream fin(part_files.front());
    std::string line;
    std::getline(fin, line);
    assert(!line.empty());

    std::istringstream iss(line);
    std::uint32_t token_count = 0;
    int value = 0;
    while (iss >> value) {
        ++token_count;
    }
    assert(token_count == problem.bits.truth_table_size());

    const std::string status = slurp(dir / "L0.status.txt");
    assert(status.find("done:1") != std::string::npos);

    remove_tree_if_exists(dir);
}

void test_group_files_written_current_run_and_done_resume_skip() {
    using namespace flex;

    const auto problem = make_example_problem();
    const fs::path dir = make_temp_dir("resume_done");

    engine::Options options;
    options.deterministic = true;
    options.use_gzip = false;
    options.solution_dir = dir.string();

    engine::Engine eng(options);
    const auto first = eng.run(problem);
    assert(first.group_files_written > 0);
    assert(first.solutions_written > 0);

    // This file would incorrectly affect `group_files_written` if results were computed by directory scan.
    {
        std::ofstream fout(dir / "foreign_part_123.txt");
        fout << "junk\n";
    }

    const auto second = eng.run(problem);
    assert(second.error_count == 0);
    assert(second.solutions_written == 0);
    assert(second.group_files_written == 0);

    remove_tree_if_exists(dir);
}

void test_incomplete_resume_overwrites_stale_group_parts() {
    using namespace flex;

    const auto problem = make_example_problem();
    const fs::path dir = make_temp_dir("resume_incomplete");

    {
        std::ofstream stale_part(dir / "L0_part_99.txt");
        stale_part << "stale\n";
        std::ofstream stale_status(dir / "L0.status.txt");
        stale_status << "solutions:123\n";
        stale_status << "done:0\n";
    }

    engine::Options options;
    options.deterministic = true;
    options.use_gzip = false;
    options.solution_dir = dir.string();

    engine::Engine eng(options);
    const auto res = eng.run(problem);

    assert(res.error_count == 0);
    assert(res.group_files_written > 0);
    assert(!fs::exists(dir / "L0_part_99.txt"));
    assert(fs::exists(dir / "L0.status.txt"));
    assert(slurp(dir / "L0.status.txt").find("done:1") != std::string::npos);

    remove_tree_if_exists(dir);
}

void test_gzip_output_if_available() {
    using namespace flex;
    if (!runtime::ThreadPool::gzip_available()) {
        return;
    }

    const auto problem = make_example_problem();
    const fs::path dir = make_temp_dir("gzip");

    engine::Options options;
    options.deterministic = true;
    options.use_gzip = true;
    options.solution_dir = dir.string();

    const auto res = engine::Engine(options).run(problem);
    assert(res.error_count == 0);
    assert(res.group_files_written > 0);

    const auto gz_parts = list_part_files(dir, ".txt.gz");
    assert(!gz_parts.empty());
    assert(gz_parts.size() == res.group_files_written);
    assert(fs::exists(dir / "L0.status.txt"));

    remove_tree_if_exists(dir);
}

void test_worker_exception_converted_to_runtime_error() {
    using namespace flex;

    const auto problem = make_example_problem();
    const fs::path dir = make_temp_dir("worker_exception");
    const fs::path not_dir = dir / "not_a_directory";
    {
        std::ofstream fout(not_dir);
        fout << "regular file";
    }

    engine::Options options;
    options.deterministic = true;
    options.use_gzip = false;
    options.solution_dir = not_dir.string();

    bool threw_runtime = false;
    try {
        (void)engine::Engine(options).run(problem);
    } catch (const RuntimeError&) {
        threw_runtime = true;
    }
    assert(threw_runtime);

    remove_tree_if_exists(dir);
}

} 

int main() {
    test_encoding_and_path_finder_smoke();
    test_problem_validation_errors();
    test_plaintext_output_and_format();
    test_group_files_written_current_run_and_done_resume_skip();
    test_incomplete_resume_overwrites_stale_group_parts();
    test_gzip_output_if_available();
    test_worker_exception_converted_to_runtime_error();

    std::cout << "test_basic passed" << std::endl;
    return 0;
}
