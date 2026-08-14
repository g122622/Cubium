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
 * @file ProfilerManager.cpp
 * @brief 性能追踪门面实现（Perfetto + Tracy 双轨）
 *
 * 生命周期方法（initialize/shutdown/start/stop/flush）仅委托给 PerfettoBackend——
 * Tracy 的采集由 client 自动完成，无需门面驱动。进程/线程命名（setProcessName/
 * setThreadName）双写：同时写给 PerfettoBackend（若启用）与 tracy（若启用）。
 */

#include "ProfilerManager.hpp"

#if MC_PROFILER_ENABLED

#if MC_ENABLE_TRACING
#include "PerfettoBackend.hpp"
#include "ProfilerConfig.hpp"
#include "TraceCategories.hpp"
#include "TraceEvents.hpp"
#endif

#if MC_ENABLE_TRACY
// Tracy C++ 客户端头：TracySetProgramName（安全宏，禁用时空展开）+
// tracy::SetThreadName（C++ API，始终编译，TRACY_API 导出）。
// 注意：TracyCSetThreadName 在 TRACY_ENABLE 未定义时会展开为对未定义符号的调用，
// 故线程命名走 tracy::SetThreadName 而非该宏。
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <tracy/Tracy.hpp>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif // MC_ENABLE_TRACY

#include <chrono>
#include <memory>
#include <string>
#include <common/TracySystem.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace profiler {

ProfilerManager& ProfilerManager::instance()
{
    static ProfilerManager instance;
    return instance;
}

ProfilerManager::ProfilerManager() = default;

ProfilerManager::~ProfilerManager()
{
    // 先停内存采样线程，避免它在 profiler 后端销毁后继续写 counter。
    m_memoryStop.store(true, std::memory_order::release);
    if (m_memoryThread.joinable()) {
        m_memoryThread.join();
    }

#if MC_ENABLE_TRACING
    if (m_perfetto) {
        if (m_tracing) {
            m_perfetto->stopTracing();
        }
        if (m_initialized) {
            m_perfetto->shutdown();
        }
    }
#endif
}

void ProfilerManager::initialize(const TraceConfig& config)
{
#if MC_ENABLE_TRACING
    if (m_initialized) {
        spdlog::warn("[Perfetto] Already initialized, skipping");
        return;
    }

    m_config = config;

    if (!m_perfetto) {
        m_perfetto = std::make_unique<PerfettoBackend>();
    }
    m_perfetto->initialize(config);
    m_initialized = m_perfetto->isInitialized();
    m_enabled = config.enabled;
#else
    // 仅 Tracy：无需初始化生命周期（client 自动采集），但缓存 config 供 config() 返回。
    m_config = config;
    m_initialized = true;
    m_enabled = config.enabled;
    spdlog::info("[Tracy] Profiler initialized (in-memory, no perfetto backend)");
#endif
}

void ProfilerManager::shutdown()
{
#if MC_ENABLE_TRACING
    if (m_perfetto) {
        m_perfetto->shutdown();
        m_initialized = m_perfetto->isInitialized();
        m_tracing = m_perfetto->isTracing();
    }
#else
    m_initialized = false;
    m_tracing = false;
    spdlog::info("[Tracy] Profiler shutdown");
#endif
}

void ProfilerManager::startTracing()
{
#if MC_ENABLE_TRACING
    if (!m_perfetto) {
        spdlog::error("[Perfetto] Cannot start tracing: not initialized");
        return;
    }
    m_perfetto->startTracing();
    m_tracing = m_perfetto->isTracing();
#else
    // Tracy 自动采集，无显式 start 概念
#endif

    // 启动内存采样后台线程（若尚未运行）。仅当上层已通过 setMemorySampler 注入
    // 采样回调时才起线程——ProfilerManager 处于比 PlatformInfo 更底层的 mc_profiler 库，
    // 不能直接采样进程内存，故由调用方注入回调。MC_TRACE_COUNTER 双轨宏自动按启用的后端写入。
    if (!m_memoryThread.joinable() && m_memorySampler) {
        m_memoryStop.store(false, std::memory_order::release);
        m_memoryThread = std::thread(&ProfilerManager::_runMemoryTrace, this);
        spdlog::info("Memory trace thread started (100 Hz sampling)");
    }
}

void ProfilerManager::stopTracing()
{
    // 先停内存采样线程，确保不再有 counter 写入后端后再停止 tracing 会话。
    m_memoryStop.store(true, std::memory_order::release);
    if (m_memoryThread.joinable()) {
        m_memoryThread.join();
    }

#if MC_ENABLE_TRACING
    if (m_perfetto) {
        m_perfetto->stopTracing();
        m_tracing = m_perfetto->isTracing();
    }
#else
    // Tracy 自动采集，无显式 stop 概念
#endif
}

void ProfilerManager::flush()
{
#if MC_ENABLE_TRACING
    if (m_perfetto) {
        m_perfetto->flush();
    }
#endif
}

bool ProfilerManager::isEnabled() const noexcept
{
#if MC_ENABLE_TRACING
    return m_initialized && m_enabled && m_tracing;
#else
    return false;
#endif
}

void ProfilerManager::setProcessName(const std::string& name)
{
#if MC_ENABLE_TRACING
    if (m_perfetto) {
        m_perfetto->setProcessName(name);
    }
#endif

#if MC_ENABLE_TRACY
    // TracySetProgramName 是安全宏：TRACY_ENABLE 未定义时空展开
    TracySetProgramName(name.c_str());
#endif
}

void ProfilerManager::setThreadName(const std::string& name)
{
#if MC_ENABLE_TRACING
    if (m_perfetto) {
        m_perfetto->setThreadName(name); // 内部查表注入 sibling_order_rank
    }
#endif

#if MC_ENABLE_TRACY
    // tracy::SetThreadName 始终编译（TRACY_API），仅当链接 TracyClient 时有定义
    tracy::SetThreadName(name.c_str());
#endif
}

void ProfilerManager::setThreadName(const std::string& name, int siblingOrderRank)
{
#if MC_ENABLE_TRACING
    if (m_perfetto) {
        m_perfetto->setThreadName(name, siblingOrderRank);
    }
#endif

#if MC_ENABLE_TRACY
    tracy::SetThreadName(name.c_str()); // Tracy 不参与排序，仅记录线程名
#endif
}

void ProfilerManager::_runMemoryTrace()
{
    // 设置采样线程名称（双轨：Perfetto sibling_order_rank + Tracy）。
    setThreadName("MemoryTrace");

    // 采样间隔：1000 Hz = 1ms 一次。
    constexpr auto kSampleInterval = std::chrono::milliseconds(1);

    while (!m_memoryStop.load(std::memory_order::acquire)) {
        // 调用上层注入的采样回调获取进程内存（工作集 + 提交量），写入
        // TraceEvents.Memory.Usage 计数器：
        // - ProcessMemory：工作集（WorkingSetSize），当前驻留物理 RAM 的页。
        // - ProcessCommit：提交量（PagefileUsage），进程向 OS 申请保留的总虚拟内存。
        // 工作集受页面复用影响，释放堆后常纹丝不动；提交量更及时反映结构优化是否
        // 真实降低占用。两者并列便于在 Tracy/Perfetto 中对照分析。
        // MC_TRACE_COUNTER 为双轨宏：按启用的后端（Perfetto/Tracy）自动写入。
        const auto [memoryMB, commitMB] = m_memorySampler();
        MC_TRACE_COUNTER(trace::TraceEvents.Memory.Usage, "ProcessMemory", memoryMB);
        MC_TRACE_COUNTER(trace::TraceEvents.Memory.Usage, "ProcessCommit", commitMB);

        std::this_thread::sleep_for(kSampleInterval);
    }
}

} // namespace profiler
} // namespace mc

#endif // MC_PROFILER_ENABLED
