#include <flex/runtime/group_checkpoint.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace flex::runtime {
namespace fs = std::filesystem;

std::string key_string_from_path(std::uint32_t depth, const std::vector<std::uint16_t>& path) {
    std::ostringstream os;
    os << "L" << depth;
    for (std::size_t i = 0; i < path.size(); ++i) {
        os << "_" << path[i];
    }
    return os.str();
}

GroupStatus load_status(const std::string& status_path) {
    GroupStatus gs;
    std::ifstream fin(status_path);
    if (!fin) return gs;
    std::string line;
    while (std::getline(fin, line)) {
        if (line.rfind("solutions:", 0) == 0) {
            gs.solutions = std::strtoull(line.c_str() + 10, nullptr, 10);
        } else if (line.rfind("done:", 0) == 0) {
            int v = std::atoi(line.c_str() + 5);
            gs.done = (v != 0);
        }
    }
    return gs;
}

void save_status_atomic(const std::string& status_path, const GroupStatus& gs) {
    fs::create_directories(fs::path(status_path).parent_path());
    std::string tmp_path = status_path + ".tmp";
    {
        std::ofstream fout(tmp_path, std::ios::trunc);
        fout << "solutions:" << gs.solutions << "\n";
        fout << "done:" << (gs.done ? 1 : 0) << "\n";
    }
    std::error_code ec;
    fs::rename(tmp_path, status_path, ec);
    if (ec) {
        fs::remove(status_path);
        fs::copy_file(tmp_path, status_path, fs::copy_options::overwrite_existing, ec);
        fs::remove(tmp_path);
    }
}

std::string file_suffix(bool use_gzip) {
    return use_gzip ? ".txt.gz" : ".txt";
}

void remove_group_part_files(const std::string& solution_dir, const std::string& group_key, bool use_gzip) {
    if (!fs::exists(solution_dir)) return;
    const std::string suffix = file_suffix(use_gzip);
    for (auto& p : fs::directory_iterator(solution_dir)) {
        if (!p.is_regular_file()) continue;
        auto name = p.path().filename().string();
        std::string prefix = group_key + "_part_";
        if (name.size() >= prefix.size() + suffix.size() &&
            name.rfind(prefix, 0) == 0 &&
            name.find(suffix) == name.size() - suffix.size()) {
            std::error_code ec;
            fs::remove(p.path(), ec);
        }
    }
}

}
