#include "BenchmarkCase.hpp"

#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <chrono>
#include <limits>

namespace mc::benchmark {
namespace {

[[nodiscard]] BenchmarkResult makeFailureResult(
    const std::string& name, BenchmarkStatus status, const std::string& message)
{
    BenchmarkResult result;
    result.name = name;
    result.status = status;
    result.errorMessage = message;
    return result;
}

} // namespace

BenchmarkResult executeBenchmarkCase(IBenchmarkCase& benchmarkCase, const CaseRuntimeConfig& config)
{
    auto validationResult = benchmarkCase.validateConfig(config);
    if (!validationResult.success()) {
        return makeFailureResult(config.name, BenchmarkStatus::ConfigError, validationResult.error().message());
    }

    auto setupResult = benchmarkCase.setUp(config);
    if (!setupResult.success()) {
        benchmarkCase.tearDown();
        return makeFailureResult(config.name, BenchmarkStatus::SetupError, setupResult.error().message());
    }

    for (i32 warmupIndex = 0; warmupIndex < config.measurement.warmupIterations; ++warmupIndex) {
        auto warmupResult = benchmarkCase.runOnce();
        if (!warmupResult.success()) {
            benchmarkCase.tearDown();
            return makeFailureResult(config.name, BenchmarkStatus::RuntimeError, warmupResult.error().message());
        }
    }

    std::vector<double> durationsMs;
    durationsMs.reserve(static_cast<size_t>(config.measurement.measuredIterations));

    const auto measurementStart = std::chrono::steady_clock::now();
    for (i32 measureIndex = 0; measureIndex < config.measurement.measuredIterations; ++measureIndex) {
        MC_TRACE_EVENT("benchmark.case", "BenchmarkIteration", "case", config.name, "iteration", measureIndex);

        const auto start = std::chrono::steady_clock::now();
        auto runResult = benchmarkCase.runOnce();
        const auto end = std::chrono::steady_clock::now();

        if (!runResult.success()) {
            benchmarkCase.tearDown();
            return makeFailureResult(config.name, BenchmarkStatus::RuntimeError, runResult.error().message());
        }

        const std::chrono::duration<double, std::milli> elapsed = end - start;
        durationsMs.push_back(elapsed.count());
    }

    const auto measurementEnd = std::chrono::steady_clock::now();
    benchmarkCase.tearDown();

    const std::chrono::duration<double, std::milli> totalDuration = measurementEnd - measurementStart;
    const double minMs = *std::min_element(durationsMs.begin(), durationsMs.end());
    const double maxMs = *std::max_element(durationsMs.begin(), durationsMs.end());
    const double totalMs = totalDuration.count();
    const double averageMs = totalMs / static_cast<double>(durationsMs.size());

    BenchmarkResult result;
    result.name = config.name;
    result.status = BenchmarkStatus::Success;
    result.metrics = BenchmarkMetrics{
        totalMs,
        averageMs,
        minMs,
        maxMs,
        (static_cast<double>(config.measurement.measuredIterations) * 1000.0) / totalMs,
        config.measurement.warmupIterations,
        config.measurement.measuredIterations,
        config.threadCount};
    return result;
}

} // namespace mc::benchmark
