#pragma once

#include "BenchmarkTypes.hpp"

namespace mc::benchmark {

[[nodiscard]] Result<void> writeBenchmarkResults(
    const std::filesystem::path& outputPath, const std::vector<BenchmarkResult>& results);

} // namespace mc::benchmark
