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

namespace mc::benchmark {

/**
 * @brief google/benchmark 内存指标采集器
 *
 * 继承 benchmark::MemoryManager，通过全局 operator new/delete 钩子统计每次
 * Start/Stop 区间内的堆分配行为，Stop 时填充：
 * - num_allocs：分配次数
 * - max_bytes_used：峰值在用字节数（在用 = 已分配 - 已释放）
 * - total_allocated_bytes：累计分配字节数
 * - net_heap_growth：累计分配 - 累计释放
 *
 * 采集原理：本类注册后，main() 在进入 benchmark 主流程前调用 enableHook()，
 * 把全局 operator new/delete 重定向到本文件的统计实现（Linux/macOS/Windows
 * 统一走 C++ 全局替换，行为跨平台一致）。Start/Stop 只是置位"统计开关"并
 * 快照计数器，本身不分配内存。
 *
 * 线程安全：计数器为原子量，多线程 benchmark（->Threads(N)）下的统计是
 * 全进程口径（与 google/benchmark 内存指标的进程级语义一致）。
 *
 * 注意：全局 operator new 替换会作用于整个进程（包括被测代码之外的部分）。
 * Start/Stop 边界之外的分配不计入结果，benchmark 输出的指标只反映
 * benchmark 循环体内的分配行为。
 */
class MemoryProfiler final : public ::benchmark::MemoryManager {
public:
    /// 进程内单例（google/benchmark 以裸指针注册，须比 RunSpecifiedBenchmarks 活得久）。
    [[nodiscard]] static MemoryProfiler& instance() noexcept;

    /// 安装全局 operator new/delete 钩子（幂等）。必须在任意 benchmark 运行前调用。
    static void enableHook() noexcept;

    void Start() override;
    void Stop(Result& result) override;

private:
    MemoryProfiler() = default;

    // Start 时的计数器快照（Stop 时以当前值减去快照得到区间增量）。
    // 定义为嵌套类型以便 .cpp 中的成员函数直接使用。
    struct Snapshot {
        std::uint64_t allocCount = 0;
        std::uint64_t allocBytes = 0;
        std::uint64_t freeCount = 0;
        std::uint64_t freeBytes = 0;
    };

    [[nodiscard]] static Snapshot takeSnapshot();

    Snapshot m_startSnapshot;
};

} // namespace mc::benchmark
