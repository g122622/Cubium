/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

/**
 * @file PerfettoBackend.hpp
 * @brief Perfetto 后端
 *
 * 封装 Perfetto SDK 的全部交互逻辑：TracingSession 生命周期、root track
 * descriptor（uuid=0，启用显式线程排序）、写文件、进程/线程命名。
 * 仅在 MC_ENABLE_TRACING=1 时编译。由 ProfilerManager 门面持有。
 *
 * 线程排序机制（PR #6219）：根 track 设 thread_ordering=EXPLICIT，每线程设
 * sibling_order_rank，UI 据此升序排列。根 track descriptor 必须用
 * TrackEvent::Trace + NewTracePacket 直接写 packet（SetTrackDescriptor 对
 * uuid=0 永远 defer 不落盘）。
 */

#pragma once

#include "ProfilerConfig.hpp"

#if MC_ENABLE_TRACING

#include "ProfilerManager.hpp" // TraceConfig

#include <memory>
#include <string>

namespace mc {
namespace profiler {

/**
 * @brief Perfetto 后端
 *
 * 管理 Perfetto TracingSession 的完整生命周期。非单例，由 ProfilerManager 持有。
 * 使用 Pimpl 模式隐藏 Perfetto SDK 类型。
 */
class PerfettoBackend {
public:
    PerfettoBackend();
    ~PerfettoBackend();

    PerfettoBackend(const PerfettoBackend&) = delete;
    PerfettoBackend& operator=(const PerfettoBackend&) = delete;

    /**
     * @brief 初始化 Perfetto 追踪系统
     *
     * 幂等：重复调用直接返回。
     *
     * @param config 配置选项
     */
    void initialize(const TraceConfig& config);

    /** @brief 关闭追踪系统，刷新数据并释放资源 */
    void shutdown();

    /** @brief 启动追踪会话，发根 track descriptor */
    void startTracing();

    /** @brief 停止追踪会话，读取数据写入文件 */
    void stopTracing();

    /** @brief 刷新 TrackEvent 数据源 */
    void flush();

    /** @brief 是否已初始化 */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /** @brief 是否正在录制 */
    [[nodiscard]] bool isTracing() const noexcept { return m_tracing; }

    /** @brief 设置进程名称 */
    void setProcessName(const std::string& name);

    /** @brief 设置当前线程名称（rank 自动查表） */
    void setThreadName(const std::string& name);

    /**
     * @brief 设置当前线程名称和排序 rank
     *
     * @param name 线程名称
     * @param siblingOrderRank 排序 rank（值越小越靠前）
     */
    void setThreadName(const std::string& name, int siblingOrderRank);

private:
    bool m_initialized = false;
    bool m_tracing = false;

    /** @brief 缓存 initialize() 传入的配置，供 startTracing/stopTracing 使用 */
    TraceConfig m_config;

    /** Pimpl 模式隐藏 Perfetto 特定类型 */
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace profiler
} // namespace mc

#endif // MC_ENABLE_TRACING
