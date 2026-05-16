#pragma once

#include "BenchmarkTypes.hpp"

namespace mc::benchmark {

class BenchmarkRunner {
public:
    [[nodiscard]] Result<std::vector<BenchmarkResult>> run(const BenchmarkConfig& config) const;
};

} // namespace mc::benchmark
