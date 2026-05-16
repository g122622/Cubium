#pragma once

#include "BenchmarkTypes.hpp"

namespace mc::benchmark {

class IBenchmarkCase {
public:
    virtual ~IBenchmarkCase() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual Result<void> validateConfig(const CaseRuntimeConfig& config) const = 0;
    [[nodiscard]] virtual Result<void> setUp(const CaseRuntimeConfig& config) = 0;
    [[nodiscard]] virtual Result<void> runOnce() = 0;
    virtual void tearDown() = 0;
};

BenchmarkResult executeBenchmarkCase(IBenchmarkCase& benchmarkCase, const CaseRuntimeConfig& config);

} // namespace mc::benchmark
