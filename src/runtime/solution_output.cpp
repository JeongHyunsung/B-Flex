#include <flex/runtime/solution_output.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

namespace flex::runtime {
namespace fs = std::filesystem;

static std::string shell_single_quote(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('\'');
    for (char c : s) {
        if (c == '\'') {
            out.append("'\\''");
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

static void best_effort_remove(const std::string& path) {
    std::error_code ec;
    fs::remove(path, ec);
}

bool write_solution_part_file(const std::string& base_name,
                              const std::vector<modeling::TruthTable>& solutions,
                              bool use_gzip,
                              std::string& error_msg) {
    std::string tmp_name = base_name + ".tmp";

    if (use_gzip) {
        std::ostringstream cmd;
        cmd << "gzip -c > " << shell_single_quote(tmp_name);
        FILE* gp = popen(cmd.str().c_str(), "w");
        if (!gp) {
            std::ostringstream msg;
            msg << "gzip popen failed for " << tmp_name << ": " << std::strerror(errno);
            error_msg = msg.str();
            return false;
        }
        for (const auto& table : solutions) {
            std::string s;
            s.reserve(table.size() * 3 + 1);
            for (InternalState v : table) {
                s.append(std::to_string(static_cast<int>(v)));
                s.push_back(' ');
            }
            s.push_back('\n');
            if (fwrite(s.data(), 1, s.size(), gp) != s.size()) {
                std::ostringstream msg;
                msg << "gzip write failed for " << tmp_name << ": " << std::strerror(errno);
                error_msg = msg.str();
                pclose(gp);
                best_effort_remove(tmp_name);
                return false;
            }
        }
        int rc = pclose(gp);
        if (rc != 0) {
            std::ostringstream msg;
            msg << "gzip process failed for " << tmp_name << " (exit=" << rc << ")";
            error_msg = msg.str();
            best_effort_remove(tmp_name);
            return false;
        }
    } else {
        std::ofstream fout(tmp_name, std::ios::binary | std::ios::trunc);
        if (!fout) {
            std::ostringstream msg;
            msg << "output file open failed for " << tmp_name;
            error_msg = msg.str();
            return false;
        }
        for (const auto& table : solutions) {
            for (InternalState v : table) {
                fout << static_cast<int>(v) << ' ';
            }
            fout << '\n';
        }
        if (!fout) {
            std::ostringstream msg;
            msg << "output file write failed for " << tmp_name;
            error_msg = msg.str();
            fout.close();
            best_effort_remove(tmp_name);
            return false;
        }
    }

    std::error_code ec;
    fs::rename(tmp_name, base_name, ec);
    if (ec) {
        fs::remove(base_name);
        fs::copy_file(tmp_name, base_name, fs::copy_options::overwrite_existing, ec);
        fs::remove(tmp_name);
        if (ec) {
            std::ostringstream msg;
            msg << "output file rename/copy failed for " << base_name << ": " << ec.message();
            error_msg = msg.str();
            return false;
        }
    }

    return true;
}

}
