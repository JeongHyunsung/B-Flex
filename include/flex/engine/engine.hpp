#pragma once
#include <flex/engine/options.hpp>
#include <flex/engine/results.hpp>
#include <flex/modeling/problem.hpp>

namespace flex::engine {

class Engine {
public:
    explicit Engine(Options options = {});
    Results run(const modeling::Problem& problem);

private:
    Options options_;
};

}
