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

#include "PerfettoProfilerAdapter.hpp"

#include "common/profiler/ProfilerManager.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::benchmark {

std::string PerfettoProfilerAdapter::s_currentCaseName;
u32 PerfettoProfilerAdapter::s_repeatIndex = 0;

PerfettoProfilerAdapter::PerfettoProfilerAdapter(std::string outputDirectory)
    : m_outputDirectory(std::move(outputDirectory))
{}

void PerfettoProfilerAdapter::setCaseName(const std::string& name)
{
    // 仅当用例名变化时才重置重复计数：google/benchmark 的每个重复都会先跑一遍
    // 不带 profiler 的正常 run（用例函数体在执行体首行调用本函数），再跑一遍带
    // profiler 的 run（AfterSetupStart/BeforeTeardownStop 之间）。若每次调用都重置，
    // 重复序号永远是 1，多次重复会互相覆盖同一个 trace 文件。
    if (s_currentCaseName != name) {
        s_currentCaseName = name;
        s_repeatIndex = 0;
    }
}

void PerfettoProfilerAdapter::AfterSetupStart()
{
    auto& profiler = mc::profiler::ProfilerManager::instance();

    // 文件名：<case>.perfetto-trace；同一用例的多次重复追加 .rep<N>（N≥2）避免覆盖。
    std::string name = s_currentCaseName.empty() ? std::string("unknown") : s_currentCaseName;
    std::replace(name.begin(), name.end(), '/', '_');
    std::replace(name.begin(), name.end(), '\\', '_');
    ++s_repeatIndex;
    if (s_repeatIndex > 1) {
        name += ".rep" + std::to_string(s_repeatIndex);
    }
    const std::string outputPath = m_outputDirectory + "/" + name + ".perfetto-trace";

    if (!profiler.isInitialized()) {
        // 首次回调：完成进程级初始化（Perfetto 的 Tracing::Initialize 只能做一次）。
        mc::profiler::TraceConfig traceConfig;
        traceConfig.enabled = true;
        traceConfig.outputToFile = true;
        traceConfig.outputPath = outputPath;
        profiler.initialize(traceConfig);
    } else {
        // 后续用例/重复：每个 trace 文件独立落盘，只切换输出路径。
        // 重新 initialize() 会被门面拒绝（Already initialized），这正是此前所有用例的
        // trace 都写进首个用例文件、互相覆盖的原因。
        profiler.setOutputPath(outputPath);
    }

    // 每次 startTracing() 都会新建 Perfetto session（TrackEvent 的 track descriptor
    // 属于 session，必须在 start 之后重发），故进程名/线程名逐用例重设。
    profiler.startTracing();
    profiler.setProcessName("mc_benchmark");
    profiler.setThreadName("benchmark-main");
}

void PerfettoProfilerAdapter::BeforeTeardownStop()
{
    auto& profiler = mc::profiler::ProfilerManager::instance();
    profiler.stopTracing();
    spdlog::info("[mc_benchmark] trace written: {}", profiler.config().outputPath);
}

} // namespace mc::benchmark
