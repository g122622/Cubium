#include "BenchmarkResultWriter.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace mc::benchmark {

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

        root["results"].push_back(std::move(resultJson));
    }

    std::ofstream output(outputPath);
    if (!output.is_open()) {
        return Error(ErrorCode::IOError, std::string("failed to open benchmark result file: ") + outputPath.string());
    }

    output << root.dump(2);
    return Result<void>::ok();
}

} // namespace mc::benchmark
