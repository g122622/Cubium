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

#include "common/core/Types.hpp"

#include <benchmark/benchmark.h>

#include <string>

namespace mc::benchmark {

/**
 * @brief Perfetto/Tracy 追踪适配器（挂接 google/benchmark ProfilerManager 钩子）
 *
 * 注册为 google/benchmark 的 ProfilerManager 后，每个 benchmark 用例在
 * "setup 完成 → 开始计时" 边界回调 AfterSetupStart()，在 "计时结束 → teardown"
 * 边界回调 BeforeTeardownStop()。本适配器在这两个边界启停 mc::profiler 的
 * Perfetto 后端，使每个用例输出一个独立的 .perfetto-trace 文件。
 *
 * 注意：google/benchmark 的 ProfilerManager 语义是"额外跑一遍带 profiler 的
 * 基准"（如果启用 ProfilerManager 感知的流程），但当前版本（1.9.x）的默认
 * 实现是在每次重复的 setup/teardown 边界都回调本适配器——因此适配器内部
 * 按文件名递增生成 trace 文件，同一名义用例的多次重复会各自产生独立文件
 * （后缀 .rep<N>），避免互相覆盖。
 *
 * trace 文件命名：<outputDir>/<benchmarkName>.perfetto-trace
 * （benchmarkName 形如 chunk_generation/threads=4，其中 '/' 替换为 '_'。）
 *
 * 已知局限（TODO）：AfterSetupStart 回调不携带用例名，当前用例名依赖用例侧
 * 在循环首行调用 PerfettoProfilerAdapter::setCaseName 显式设置；未设置的用例
 * trace 文件名为 unknown.perfetto-trace。后续可改为经 benchmark::State::name()
 * 在用例函数体内设置。
 */
class PerfettoProfilerAdapter final : public ::benchmark::ProfilerManager {
public:
    /**
     * @brief 构造适配器
     *
     * @param outputDirectory trace 文件输出目录（时间戳归档目录），必须已存在
     */
    explicit PerfettoProfilerAdapter(std::string outputDirectory);

    void AfterSetupStart() override;
    void BeforeTeardownStop() override;

    /// 用例在进入循环前调用：设置本次 trace 的文件名主干。
    static void setCaseName(const std::string& name);

private:
    std::string m_outputDirectory;
    /// 当前运行的用例名（AfterSetupStart 不携带用例名，由用例侧经 setCaseName 设置；
    /// 未设置时用 "unknown"）。
    static std::string s_currentCaseName;
    static u32 s_repeatIndex;
};

} // namespace mc::benchmark
