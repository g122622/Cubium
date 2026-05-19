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

#include "BenchmarkResultWriter.hpp"

#include <fstream>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace mc::benchmark {
namespace {

[[nodiscard]] std::string escapeCsvField(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped += ch;
        }
    }
    return escaped;
}

} // namespace

Result<void> writeBenchmarkResults(const std::filesystem::path& outputPath, const std::vector<BenchmarkResult>& results)
{
    nlohmann::json root = nlohmann::json::object();
    root["results"] = nlohmann::json::array();

    for (const auto& result : results) {
        nlohmann::json resultJson = {
            {"name", result.name},
            {"status", static_cast<i32>(result.status)},
            {"errorMessage", result.errorMessage},
        };

        if (result.metrics.has_value()) {
            resultJson["metrics"] = {
                {"totalMs", result.metrics->totalMs},
                {"averageMs", result.metrics->averageMs},
                {"minMs", result.metrics->minMs},
                {"maxMs", result.metrics->maxMs},
                {"operationsPerSecond", result.metrics->operationsPerSecond},
                {"warmupIterations", result.metrics->warmupIterations},
                {"measuredIterations", result.metrics->measuredIterations},
                {"threadCount", result.metrics->threadCount},
            };
        }

        resultJson["iterationDurationsMs"] = result.iterationDurationsMs;

        root["results"].push_back(std::move(resultJson));
    }

    std::ofstream output(outputPath);
    if (!output.is_open()) {
        return Error(
            ErrorCode::FileOpenFailed, std::string("failed to open benchmark result file: ") + outputPath.string());
    }

    output << root.dump(2);
    return Result<void>::ok();
}

Result<void> writeBenchmarkResultCsv(
    const std::filesystem::path& outputPath, const std::vector<BenchmarkResult>& results)
{
    std::ofstream output(outputPath);
    if (!output.is_open()) {
        return Error(
            ErrorCode::FileOpenFailed, std::string("failed to open benchmark csv file: ") + outputPath.string());
    }

    output << "caseName,status,errorMessage,threadCount,warmupIterations,measuredIterations,iteration,durationMs\n";

    for (const auto& result : results) {
        const i32 threadCount = result.metrics.has_value() ? result.metrics->threadCount : 0;
        const i32 warmupIterations = result.metrics.has_value() ? result.metrics->warmupIterations : 0;
        const i32 measuredIterations = result.metrics.has_value() ? result.metrics->measuredIterations : 0;

        if (result.iterationDurationsMs.empty()) {
            output << fmt::format("\"{}\",{},\"{}\",{},{},{},,\n",
                escapeCsvField(result.name),
                static_cast<i32>(result.status),
                escapeCsvField(result.errorMessage),
                threadCount,
                warmupIterations,
                measuredIterations);
            continue;
        }

        for (size_t iterationIndex = 0; iterationIndex < result.iterationDurationsMs.size(); ++iterationIndex) {
            output << fmt::format("\"{}\",{},\"{}\",{},{},{},{},{}\n",
                escapeCsvField(result.name),
                static_cast<i32>(result.status),
                escapeCsvField(result.errorMessage),
                threadCount,
                warmupIterations,
                measuredIterations,
                iterationIndex,
                result.iterationDurationsMs[iterationIndex]);
        }
    }

    return Result<void>::ok();
}

} // namespace mc::benchmark
