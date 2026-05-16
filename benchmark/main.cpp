#include "BenchmarkConfig.hpp"
#include "BenchmarkResultWriter.hpp"
#include "BenchmarkRunner.hpp"

#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <iostream>

int main()
{
    const std::filesystem::path rootDirectory = std::filesystem::current_path();
    auto configResult = mc::benchmark::loadBenchmarkConfig(rootDirectory);
    if (!configResult.success()) {
        std::cerr << configResult.error().message() << std::endl;
        return 1;
    }

    const mc::benchmark::BenchmarkConfig& config = configResult.value();

    mc::perfetto::TraceConfig traceConfig;
    traceConfig.enabled = config.traceEnabled;
    traceConfig.outputToFile = true;
    traceConfig.outputPath = config.traceOutputPath.string();
    mc::perfetto::PerfettoManager::instance().initialize(traceConfig);
    mc::perfetto::PerfettoManager::instance().setProcessName("mc_benchmarks");
    mc::perfetto::PerfettoManager::instance().setThreadName("benchmark-main");
    mc::perfetto::PerfettoManager::instance().startTracing();

    mc::benchmark::BenchmarkRunner runner;
    auto runResult = runner.run(config);
    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();

    if (!runResult.success()) {
        std::cerr << runResult.error().message() << std::endl;
        return 1;
    }

    const auto& results = runResult.value();
    auto writeResult = mc::benchmark::writeBenchmarkResults(config.resultOutputPath, results);
    if (!writeResult.success()) {
        std::cerr << writeResult.error().message() << std::endl;
        return 1;
    }

    bool hasFailure = false;
    for (const auto& result : results) {
        std::cout << result.name << ": ";
        if (result.status == mc::benchmark::BenchmarkStatus::Success && result.metrics.has_value()) {
            std::cout << result.metrics->averageMs << " ms avg, " << result.metrics->operationsPerSecond << " ops/s";
        } else {
            hasFailure = true;
            std::cout << "FAILED: " << result.errorMessage;
        }
        std::cout << std::endl;
    }

    return hasFailure ? 1 : 0;
}
