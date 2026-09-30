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
    s_currentCaseName = name;
    s_repeatIndex = 0;
}

void PerfettoProfilerAdapter::AfterSetupStart()
{
    auto& profiler = mc::profiler::ProfilerManager::instance();

    mc::profiler::TraceConfig traceConfig;
    traceConfig.enabled = true;
    traceConfig.outputToFile = true;

    // 文件名：<case>.perfetto-trace，重复运行追加 .rep<N>（N≥2）避免覆盖。
    std::string name = s_currentCaseName.empty() ? std::string("unknown") : s_currentCaseName;
    std::replace(name.begin(), name.end(), '/', '_');
    std::replace(name.begin(), name.end(), '\\', '_');
    ++s_repeatIndex;
    if (s_repeatIndex > 1) {
        name += ".rep" + std::to_string(s_repeatIndex);
    }
    traceConfig.outputPath = m_outputDirectory + "/" + name + ".perfetto-trace";

    // initialize 幂等：ProfilerManager::initialize 在已初始化时为 no-op。
    profiler.initialize(traceConfig);
    profiler.setProcessName("mc_benchmark");
    profiler.setThreadName("benchmark-main");
    profiler.startTracing();
}

void PerfettoProfilerAdapter::BeforeTeardownStop()
{
    auto& profiler = mc::profiler::ProfilerManager::instance();
    profiler.stopTracing();
    spdlog::info("[mc_benchmark] trace written: {}", profiler.config().outputPath);
}

} // namespace mc::benchmark
