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

#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdlib>
#endif

#include <fmt/format.h>

namespace {

[[nodiscard]] std::string formatTimestampDirectoryName()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    return fmt::format("{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}",
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec);
}

[[nodiscard]] int runVisualizeScript(const std::filesystem::path& pythonExecutable,
    const std::filesystem::path& scriptPath,
    const std::filesystem::path& resultDirectory,
    const std::filesystem::path& csvPath)
{
#ifdef _WIN32
    std::string commandLine = fmt::format("\"{}\" \"{}\" \"{}\" \"{}\"",
        pythonExecutable.string(),
        scriptPath.string(),
        resultDirectory.string(),
        csvPath.string());

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInformation{};
    const BOOL createResult = CreateProcessA(
        nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInformation);
    if (createResult == FALSE) {
        return static_cast<int>(GetLastError());
    }

    WaitForSingleObject(processInformation.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(processInformation.hProcess, &exitCode);
    CloseHandle(processInformation.hThread);
    CloseHandle(processInformation.hProcess);
    return static_cast<int>(exitCode);
#else
    std::string command = fmt::format("\"{}\" \"{}\" \"{}\" \"{}\"",
        pythonExecutable.string(),
        scriptPath.string(),
        resultDirectory.string(),
        csvPath.string());
    return std::system(command.c_str());
#endif
}

} // namespace

int main()
{
    const std::filesystem::path rootDirectory = std::filesystem::current_path();
    auto configResult = mc::benchmark::loadBenchmarkConfig(rootDirectory);
    if (!configResult.success()) {
        std::cerr << configResult.error().message() << std::endl;
        return 1;
    }

    const mc::benchmark::BenchmarkConfig& config = configResult.value();
    const std::filesystem::path resultRootDirectory = rootDirectory / config.outputDirectory;
    const std::filesystem::path resultDirectory = resultRootDirectory / formatTimestampDirectoryName();

    std::error_code directoryError;
    if (!std::filesystem::create_directories(resultDirectory, directoryError) && directoryError) {
        std::cerr << fmt::format("failed to create benchmark result directory: {}", resultDirectory.string())
                  << std::endl;
        return 1;
    }

    const std::filesystem::path traceOutputPath = resultDirectory / config.traceFileName;
    const std::filesystem::path resultJsonPath = resultDirectory / config.resultJsonFileName;
    const std::filesystem::path resultCsvPath = resultDirectory / config.resultCsvFileName;
    const std::filesystem::path visualizeScriptPath = rootDirectory / config.visualizeScriptPath;
    const std::filesystem::path pythonExecutablePath = config.pythonExecutable;

    mc::profiler::TraceConfig traceConfig;
    traceConfig.enabled = config.traceEnabled;
    traceConfig.outputToFile = true;
    traceConfig.outputPath = traceOutputPath.string();
    mc::profiler::ProfilerManager::instance().initialize(traceConfig);
    mc::profiler::ProfilerManager::instance().setProcessName("mc_benchmarks");
    mc::profiler::ProfilerManager::instance().setThreadName("benchmark-main");
    mc::profiler::ProfilerManager::instance().startTracing();

    mc::benchmark::BenchmarkRunner runner;
    auto runResult = runner.run(config);
    mc::profiler::ProfilerManager::instance().stopTracing();
    mc::profiler::ProfilerManager::instance().shutdown();

    if (!runResult.success()) {
        std::cerr << runResult.error().message() << std::endl;
        return 1;
    }

    const auto& results = runResult.value();
    auto writeResult = mc::benchmark::writeBenchmarkResults(resultJsonPath, results);
    if (!writeResult.success()) {
        std::cerr << writeResult.error().message() << std::endl;
        return 1;
    }

    auto writeCsvResult = mc::benchmark::writeBenchmarkResultCsv(resultCsvPath, results);
    if (!writeCsvResult.success()) {
        std::cerr << writeResult.error().message() << std::endl;
        return 1;
    }

    const int visualizeExitCode =
        runVisualizeScript(pythonExecutablePath, visualizeScriptPath, resultDirectory, resultCsvPath);
    if (visualizeExitCode != 0) {
        std::cerr << fmt::format("benchmark visualization failed with exit code {}", visualizeExitCode) << std::endl;
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

    std::cout << "results directory: " << resultDirectory.string() << std::endl;

    return hasFailure ? 1 : 0;
}
