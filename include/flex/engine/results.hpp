#pragma once
#include <string>
#include <cstdint>
#include <flex/common/defs.hpp>
#include <flex/modeling/truth_table.hpp>

namespace flex::engine {

struct Results {
    std::uint64_t solutions_written = 0;
    std::uint64_t group_files_written = 0;
    std::string solution_dir;
    std::uint64_t error_count = 0;
};

}
