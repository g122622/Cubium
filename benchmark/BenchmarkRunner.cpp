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

#include "BenchmarkRunner.hpp"

#include "BenchmarkCase.hpp"
#include "BenchmarkRegistry.hpp"
#include "common/profiler/TraceEvents.hpp"

using namespace mc::trace;

namespace mc::benchmark {

Result<std::vector<BenchmarkResult>> BenchmarkRunner::run(const BenchmarkConfig& config) const
{
    std::vector<BenchmarkResult> results;
    results.reserve(config.cases.size());

    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Benchmark.Run, "RunBenchmarkSuite", "caseCount", static_cast<i32>(config.cases.size()));

    for (const auto& caseConfig : config.cases) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run,
            "RunBenchmarkCase",
            "case",
            caseConfig.name,
            "threadCount",
            caseConfig.threadCount);

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
