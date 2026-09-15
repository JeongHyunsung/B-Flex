#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace flex::runtime {

struct GroupStatus {
    std::uint64_t solutions = 0;
    bool done = false;
};

std::string key_string_from_path(std::uint32_t depth, const std::vector<std::uint16_t>& path);
GroupStatus load_status(const std::string& status_path);
void save_status_atomic(const std::string& status_path, const GroupStatus& gs);
std::string file_suffix(bool use_gzip);
void remove_group_part_files(const std::string& solution_dir, const std::string& group_key, bool use_gzip);

}
