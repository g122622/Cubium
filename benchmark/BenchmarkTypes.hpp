#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mc::benchmark {

enum class BenchmarkStatus : u8 {
    Success = 0,
    ConfigError = 1,
    SetupError = 2,
    RuntimeError = 3,
};

struct MeasurementConfig {
    i32 warmupIterations;
    i32 measuredIterations;
    i32 minDurationMs;
};

struct CaseRuntimeConfig {
    std::string name;
    i32 threadCount;
    MeasurementConfig measurement;
    std::filesystem::path tracePath;
    nlohmann::json parameters;
};

struct BenchmarkConfig {
    bool traceEnabled;
    std::filesystem::path traceOutputPath;
    std::filesystem::path resultOutputPath;
    i32 threadCount;
    MeasurementConfig measurement;
    std::vector<CaseRuntimeConfig> cases;
};

struct BenchmarkMetrics {
    double totalMs;
    double averageMs;
    double minMs;
    double maxMs;
    double operationsPerSecond;
    i32 warmupIterations;
    i32 measuredIterations;
    i32 threadCount;
};

struct BenchmarkResult {
    std::string name;
    BenchmarkStatus status;
    std::string errorMessage;
    std::optional<BenchmarkMetrics> metrics;
};

} // namespace mc::benchmark
