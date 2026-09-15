#pragma once 
#include <stdexcept>
#include <string>

namespace flex{

struct FlexError : public std::runtime_error{
    explicit FlexError(const std::string& msg) : std::runtime_error(msg) {}
};

struct InvalidConfig : public FlexError {
    explicit InvalidConfig(const std::string& msg) : FlexError("[InvalidConfig]: " + msg) {}
};

struct Infeasible : public FlexError {
    explicit Infeasible(const std::string& msg) : FlexError("[Infeasible]: " + msg) {}
};

struct RuntimeError : public FlexError {
    explicit RuntimeError(const std::string& msg) : FlexError("[RuntimeError]: " + msg) {}
};

}
