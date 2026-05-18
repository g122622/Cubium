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

#include <fstream>
#include <nlohmann/json.hpp>

namespace mc::benchmark {
namespace {

using json = nlohmann::json;

[[nodiscard]] Result<i32> requireInt(const json& object, std::string_view fieldName)
{
    if (!object.contains(fieldName) || !object.at(fieldName).is_number_integer()) {
        return Error(ErrorCode::InvalidArgument, std::string("Missing or invalid integer field: ") + std::string(fieldName));
    }
    return object.at(fieldName).get<i32>();
}

[[nodiscard]] Result<bool> requireBool(const json& object, std::string_view fieldName)
{
    if (!object.contains(fieldName) || !object.at(fieldName).is_boolean()) {
        return Error(ErrorCode::InvalidArgument, std::string("Missing or invalid boolean field: ") + std::string(fieldName));
    }
    return object.at(fieldName).get<bool>();
}

[[nodiscard]] Result<std::string> requireString(const json& object, std::string_view fieldName)
{
    if (!object.contains(fieldName) || !object.at(fieldName).is_string()) {
        return Error(ErrorCode::InvalidArgument, std::string("Missing or invalid string field: ") + std::string(fieldName));
    }
    return object.at(fieldName).get<std::string>();
}

[[nodiscard]] Result<MeasurementConfig> parseMeasurementConfig(const json& object)
{
    auto warmupIterations = requireInt(object, "warmupIterations");
    if (!warmupIterations.success()) {
        return warmupIterations.error();
    }

    auto measuredIterations = requireInt(object, "measuredIterations");
    if (!measuredIterations.success()) {
        return measuredIterations.error();
    }

    auto minDurationMs = requireInt(object, "minDurationMs");
    if (!minDurationMs.success()) {
        return minDurationMs.error();
    }

    return MeasurementConfig{warmupIterations.value(), measuredIterations.value(), minDurationMs.value()};
}

} // namespace

Result<BenchmarkConfig> loadBenchmarkConfig(const std::filesystem::path& rootDirectory)
{
    const std::filesystem::path configPath = rootDirectory / "benchmark.json";
    if (!std::filesystem::exists(configPath)) {
        return Error(ErrorCode::NotFound, std::string("benchmark config not found: ") + configPath.string());
    }

    std::ifstream input(configPath);
    if (!input.is_open()) {
        return Error(ErrorCode::FileOpenFailed, std::string("failed to open benchmark config: ") + configPath.string());
    }

    json root;
    try {
        input >> root;
    } catch (const json::exception& exception) {
        return Error(ErrorCode::InvalidArgument, std::string("failed to parse benchmark config: ") + exception.what());
    }

    if (!root.is_object()) {
        return Error(ErrorCode::InvalidArgument, "benchmark config root must be an object");
    }

    auto traceEnabled = requireBool(root, "traceEnabled");
    if (!traceEnabled.success()) {
        return traceEnabled.error();
    }

    auto globalThreadCount = requireInt(root, "threadCount");
    if (!globalThreadCount.success()) {
        return globalThreadCount.error();
    }

    auto outputDirectory = requireString(root, "outputDirectory");
    if (!outputDirectory.success()) {
        return outputDirectory.error();
    }

    auto resultJsonFileName = requireString(root, "resultJsonFileName");
    if (!resultJsonFileName.success()) {
        return resultJsonFileName.error();
    }

    auto resultCsvFileName = requireString(root, "resultCsvFileName");
    if (!resultCsvFileName.success()) {
        return resultCsvFileName.error();
    }

    auto traceFileName = requireString(root, "traceFileName");
    if (!traceFileName.success()) {
        return traceFileName.error();
    }

    auto visualizeScriptPath = requireString(root, "visualizeScriptPath");
    if (!visualizeScriptPath.success()) {
        return visualizeScriptPath.error();
    }

    auto pythonExecutable = requireString(root, "pythonExecutable");
    if (!pythonExecutable.success()) {
        return pythonExecutable.error();
    }

    if (!root.contains("measurement") || !root.at("measurement").is_object()) {
        return Error(ErrorCode::InvalidArgument, "missing or invalid measurement object");
    }
    auto measurementConfig = parseMeasurementConfig(root.at("measurement"));
    if (!measurementConfig.success()) {
        return measurementConfig.error();
    }

    if (!root.contains("cases") || !root.at("cases").is_array()) {
        return Error(ErrorCode::InvalidArgument, "missing or invalid cases array");
    }

    BenchmarkConfig config{
        traceEnabled.value(),
        globalThreadCount.value(),
        measurementConfig.value(),
        std::filesystem::path(outputDirectory.value()),
        std::filesystem::path(resultJsonFileName.value()),
        std::filesystem::path(resultCsvFileName.value()),
        std::filesystem::path(traceFileName.value()),
        std::filesystem::path(visualizeScriptPath.value()),
        std::filesystem::path(pythonExecutable.value()),
        {}};

    for (const auto& caseJson : root.at("cases")) {
        if (!caseJson.is_object()) {
            return Error(ErrorCode::InvalidArgument, "case entry must be an object");
        }

        auto caseName = requireString(caseJson, "name");
        if (!caseName.success()) {
            return caseName.error();
        }

        auto caseThreadCount = requireInt(caseJson, "threadCount");
        if (!caseThreadCount.success()) {
            return caseThreadCount.error();
        }

        if (!caseJson.contains("measurement") || !caseJson.at("measurement").is_object()) {
            return Error(ErrorCode::InvalidArgument, std::string("missing measurement for case: ") + caseName.value());
        }

        auto caseMeasurement = parseMeasurementConfig(caseJson.at("measurement"));
        if (!caseMeasurement.success()) {
            return caseMeasurement.error();
        }

        if (!caseJson.contains("parameters") || !caseJson.at("parameters").is_object()) {
            return Error(ErrorCode::InvalidArgument, std::string("missing parameters for case: ") + caseName.value());
        }

        config.cases.push_back(CaseRuntimeConfig{
            caseName.value(),
            caseThreadCount.value(),
            caseMeasurement.value(),
            config.traceFileName,
            caseJson.at("parameters")});
    }

    return config;
}

} // namespace mc::benchmark
