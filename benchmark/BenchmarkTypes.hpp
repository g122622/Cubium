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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <nlohmann/json.hpp>
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
    i32 threadCount;
    MeasurementConfig measurement;
    std::filesystem::path outputDirectory;
    std::filesystem::path resultJsonFileName;
    std::filesystem::path resultCsvFileName;
    std::filesystem::path traceFileName;
    std::filesystem::path visualizeScriptPath;
    std::filesystem::path pythonExecutable;
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
    std::vector<double> iterationDurationsMs;
};

} // namespace mc::benchmark
