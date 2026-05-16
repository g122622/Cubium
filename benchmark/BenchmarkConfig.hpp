#pragma once

#include "BenchmarkTypes.hpp"

namespace mc::benchmark {

[[nodiscard]] Result<BenchmarkConfig> loadBenchmarkConfig(const std::filesystem::path& rootDirectory);

} // namespace mc::benchmark
