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

#include "MemoryTraceThread.hpp"
#include "common/perfetto/PerfettoConfig.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/PlatformInfo.hpp"
#include <chrono>
#include <spdlog/spdlog.h>

#if MC_ENABLE_TRACING
#include <perfetto.h>
#endif

using namespace mc::trace;

namespace mc::client {

// 采样间隔：每秒 100 次 = 10ms
static constexpr auto SAMPLE_INTERVAL = std::chrono::milliseconds(10);

MemoryTraceThread::MemoryTraceThread() = default;

MemoryTraceThread::~MemoryTraceThread()
{
    stop();
}

void MemoryTraceThread::start()
{
    if (m_running.load(std::memory_order::acquire)) {
        return;
    }

    m_stop.store(false, std::memory_order::release);
    m_running.store(true, std::memory_order::release);
    m_thread = std::thread(&MemoryTraceThread::run, this);

    spdlog::info("Memory trace thread started (100 Hz sampling)");
}

void MemoryTraceThread::stop()
{
    if (!m_running.load(std::memory_order::acquire)) {
        return;
    }

    m_stop.store(true, std::memory_order::release);

    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_running.store(false, std::memory_order::release);
    spdlog::info("Memory trace thread stopped");
}

void MemoryTraceThread::run()
{
#if MC_ENABLE_TRACING
    // 设置线程名称
    MC_TRACE_SET_THREAD_NAME("MemoryTrace");
#endif

    while (!m_stop.load(std::memory_order::acquire)) {
#if MC_ENABLE_TRACING
        // 采样内存并写入追踪
        const i64 memoryMB = static_cast<i64>(util::PlatformInfo::getProcessMemoryMB());
        MC_TRACE_COUNTER(TraceEvents.Memory.Usage, "ProcessMemory", memoryMB);
#endif

        // 等待下一次采样
        std::this_thread::sleep_for(SAMPLE_INTERVAL);
    }
}

} // namespace mc::client
