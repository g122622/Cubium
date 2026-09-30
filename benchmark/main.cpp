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

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <benchmark/benchmark.h>

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
[[nodiscard]] bool containsBenchmarkOut(const std::vector<std::string>& args)
{
    for (const auto& arg : args) {
        if (arg.rfind("--benchmark_out=", 0) == 0 || arg.rfind("--benchmark-out=", 0) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 解析并就地剥离 mc_benchmark 自有开关
 *
 * `--benchmark_trace[=true|false]`：是否录制 Perfetto trace。**默认 false**——注册
 * ProfilerManager 后 google/benchmark 会为每个重复额外跑一遍 profile run，并按用例/重复
 * 落盘 .perfetto-trace（每个几十 MB 级，64 档扫描用例就是几十个文件），只有需要火焰图
 * 分析时才值得开启。
 *
 * 必须在 benchmark::Initialize 之前调用：Initialize 只识别它自己的 `--benchmark_*` 已知
 * flag，不认识的参数会原样留在 argv 里然后被忽略，因此由本函数自己消费掉。
 *
 * @param args 参数列表（含 argv[0]），命中的开关会被就地移除
 * @return 是否启用 trace（同一开关多次出现时以最后一次为准）
 */
[[nodiscard]] bool extractTraceFlag(std::vector<std::string>& args)
{
    constexpr std::string_view ENABLE_FLAG = "--benchmark_trace";
    constexpr std::string_view ENABLE_FLAG_TRUE = "--benchmark_trace=true";
    constexpr std::string_view ENABLE_FLAG_FALSE = "--benchmark_trace=false";

    bool enabled = false;
    std::vector<std::string> kept;
    kept.reserve(args.size());
    for (auto& arg : args) {
        const std::string_view view = arg;
        if (view == ENABLE_FLAG || view == ENABLE_FLAG_TRUE) {
            enabled = true;
            continue;
        }
        if (view == ENABLE_FLAG_FALSE) {
            enabled = false;
            continue;
        }
        kept.push_back(std::move(arg));
    }
    args = std::move(kept);
    return enabled;
}

} // namespace

int main(int argc, char** argv)
{
    // 时间戳归档目录必须在 CreateProcess 前确定（结果 JSON 路径要注入 argv），
    // 但 Initialize 须在 argv 注入前完成解析顺序问题——把目录计算提前即可：
    // 归档目录只依赖当前时间，不依赖任何 flag。
    const std::filesystem::path rootDirectory = std::filesystem::current_path();
    const std::filesystem::path resultDirectory = rootDirectory / "benchmark_results" / formatTimestampDirectoryName();

    // 统一构造传给 benchmark::Initialize 的参数列表：保留 argv[0]、剥离自有开关、按需注入
    // 默认 --benchmark_out。这些都必须在 Initialize 之前完成：Initialize 解析后即固化 flag，
    // 且它不认识的参数会被原样忽略（因此自有开关必须在它之前消费掉）。
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc) + 1);
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    const bool traceEnabled = extractTraceFlag(args);
    if (!containsBenchmarkOut(args)) {
        args.emplace_back(fmt::format("--benchmark_out={}", (resultDirectory / "results.json").string()));
    }

    std::vector<char*> argPointers;
    argPointers.reserve(args.size());
    for (auto& arg : args) {
        argPointers.push_back(arg.data());
    }
    int parsedArgc = static_cast<int>(argPointers.size());

    // 命令行其余部分完全交给 google/benchmark 的 flags 解析（--benchmark_filter 等）。
    ::benchmark::Initialize(&parsedArgc, argPointers.data());

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

    // Perfetto trace：默认**不注册** ProfilerManager（不产出任何 .perfetto-trace，也不会
    // 触发展开 profile run 导致的额外一遍执行），仅在显式传 --benchmark_trace 时注册；
    // 注册后在每个用例的 setup/teardown 边界启停 mc::profiler，每用例/每重复一个 trace 文件。
    static mc::benchmark::PerfettoProfilerAdapter profilerAdapter(resultDirectory.string());
    if (traceEnabled) {
        ::benchmark::RegisterProfilerManager(&profilerAdapter);
    }

    ::benchmark::SetDefaultTimeUnit(::benchmark::kMillisecond);

    const size_t matched = ::benchmark::RunSpecifiedBenchmarks();

    ::benchmark::Shutdown();
    std::cout << "results directory: " << resultDirectory.string() << std::endl;
    std::cout << "perfetto trace: " << (traceEnabled ? "enabled" : "disabled (pass --benchmark_trace to record)")
              << std::endl;
    return matched == 0 ? 1 : 0;
}
