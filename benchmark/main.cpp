/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

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
