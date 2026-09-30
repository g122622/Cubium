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

#include "MemoryProfiler.hpp"
#include "PerfettoProfilerAdapter.hpp"

#include <benchmark/benchmark.h>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

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

// 检测用户是否显式传了 --benchmark_out（在 benchmark::Initialize 之前扫描：
// Initialize 会把已识别的 flag 从 argv 中剔除）。
[[nodiscard]] bool argvContainsBenchmarkOut(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg.rfind("--benchmark_out=", 0) == 0 || arg.rfind("--benchmark-out=", 0) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    // 时间戳归档目录必须在 CreateProcess 前确定（结果 JSON 路径要注入 argv），
    // 但 Initialize 须在 argv 注入前完成解析顺序问题——把目录计算提前即可：
    // 归档目录只依赖当前时间，不依赖任何 flag。
    const std::filesystem::path rootDirectory = std::filesystem::current_path();
    const std::filesystem::path resultDirectory =
        rootDirectory / "benchmark_results" / formatTimestampDirectoryName();

    // 用户未显式传 --benchmark_out 时，把默认 JSON 输出路径
    // "<归档目录>/results.json" 作为附加参数注入 argv（库原生 file reporter 路径）。
    // 必须在 Initialize 之前注入：Initialize 解析后即固化 FLAGS_benchmark_out。
    const bool userProvidedOut = argvContainsBenchmarkOut(argc, argv);
    std::vector<std::string> injectedArgs;
    std::vector<char*> injectedArgv;
    if (!userProvidedOut) {
        for (int i = 0; i < argc; ++i) {
            injectedArgs.emplace_back(argv[i]);
        }
        injectedArgs.emplace_back(
            fmt::format("--benchmark_out={}", (resultDirectory / "results.json").string()));
        injectedArgv.reserve(injectedArgs.size());
        for (auto& arg : injectedArgs) {
            injectedArgv.push_back(arg.data());
        }
        argc = static_cast<int>(injectedArgv.size());
        argv = injectedArgv.data();
    }

    // 命令行完全交给 google/benchmark 的 flags 解析（--benchmark_filter 等）。
    ::benchmark::Initialize(&argc, argv);

    std::error_code directoryError;
    if (!std::filesystem::create_directories(resultDirectory, directoryError) && directoryError) {
        std::cerr << fmt::format("failed to create benchmark result directory: {}", resultDirectory.string())
                  << std::endl;
        return 1;
    }

    // 内存指标：注册 MemoryManager（全局 operator new/delete 钩子在进程加载期即生效，
    // enableHook 仅作文档化声明）。注册后每个用例的 JSON 结果自动携带
    // num_allocs / max_bytes_used / total_allocated_bytes / net_heap_growth 指标。
    static mc::benchmark::MemoryProfiler& memoryProfiler = mc::benchmark::MemoryProfiler::instance();
    mc::benchmark::MemoryProfiler::enableHook();
    ::benchmark::RegisterMemoryManager(&memoryProfiler);

    // Perfetto trace：注册 ProfilerManager 适配器，在每个用例的 setup/teardown 边界
    // 启停 mc::profiler，每用例一个 .perfetto-trace 文件（写入时间戳归档目录）。
    static mc::benchmark::PerfettoProfilerAdapter profilerAdapter(resultDirectory.string());
    ::benchmark::RegisterProfilerManager(&profilerAdapter);

    ::benchmark::SetDefaultTimeUnit(::benchmark::kMillisecond);

    const size_t matched = ::benchmark::RunSpecifiedBenchmarks();

    ::benchmark::Shutdown();
    std::cout << "results directory: " << resultDirectory.string() << std::endl;
    return matched == 0 ? 1 : 0;
}
