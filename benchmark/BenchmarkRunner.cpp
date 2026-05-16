#include "BenchmarkRunner.hpp"

#include "BenchmarkCase.hpp"
#include "BenchmarkRegistry.hpp"
#include "common/perfetto/TraceEvents.hpp"

namespace mc::benchmark {

Result<std::vector<BenchmarkResult>> BenchmarkRunner::run(const BenchmarkConfig& config) const
{
    std::vector<BenchmarkResult> results;
    results.reserve(config.cases.size());

    MC_TRACE_EVENT("benchmark.run", "RunBenchmarkSuite", "caseCount", static_cast<i32>(config.cases.size()));

    for (const auto& caseConfig : config.cases) {
        MC_TRACE_EVENT("benchmark.case", "RunBenchmarkCase", "case", caseConfig.name, "threadCount", caseConfig.threadCount);

        auto benchmarkCase = BenchmarkRegistry::instance().create(caseConfig.name);
        if (benchmarkCase == nullptr) {
            BenchmarkResult result;
            result.name = caseConfig.name;
            result.status = BenchmarkStatus::ConfigError;
            result.errorMessage = "benchmark case not registered";
            results.push_back(std::move(result));
            continue;
        }

        results.push_back(executeBenchmarkCase(*benchmarkCase, caseConfig));
    }

    return results;
}

} // namespace mc::benchmark
