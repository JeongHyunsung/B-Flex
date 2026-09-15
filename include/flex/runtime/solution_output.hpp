#pragma once
#include <string>
#include <vector>
#include <flex/modeling/truth_table.hpp>

namespace flex::runtime {

bool write_solution_part_file(const std::string& base_name,
                              const std::vector<modeling::TruthTable>& solutions,
                              bool use_gzip,
                              std::string& error_msg);

}
