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

#include "LogManager.hpp"

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdio>
#include <thread>

namespace mc::application {

namespace {

/// 溢出监控线程的采样间隔。1s 足够及时告警，又不过频占用 CPU。
constexpr auto kMonitorInterval = std::chrono::seconds(1);

} // namespace

LogManager::LogManager() = default;

LogManager::~LogManager()
{
    shutdown();
}

LogManager& LogManager::instance()
{
    static LogManager s_instance;
    return s_instance;
}

void LogManager::initialize()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) {
        return;
    }

    // 1. 初始化 spdlog 全局线程池（队列 + 单消费线程）。init_thread_pool 内部建好
    //    details::thread_pool 并注册为全局，后续 spdlog::thread_pool() 取回它。
    spdlog::init_thread_pool(kQueueSize, kWorkerCount);

    // 2. stdout 彩色 sink（与原默认同步 logger 行为一致）。
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // 3. 建 async_logger，溢出策略 overrun_oldest（丢旧保新，主线程永不阻塞）。
    //    spdlog::thread_pool() 返回 shared_ptr，async_logger 取 weak_ptr 持有。
    auto logger = std::make_shared<spdlog::async_logger>(
        "mc_async", sink, spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);

    // 4. 统一日志格式：[时间] [级别] [线程id] 消息。
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    // 设为全局默认 logger，此后所有 spdlog::info/warn/... 调用走异步。
    spdlog::set_default_logger(logger);

    // 默认级别 info（CODE_CONVENTIONS §4 禁 debug/trace）。
    spdlog::set_level(spdlog::level::info);

    // 5. 启溢出监控线程。
    startOverflowMonitor();

    m_initialized = true;
}

void LogManager::shutdown()
{
    // 停溢出监控线程（须在 spdlog::shutdown 之前，避免监控线程访问已销毁的 thread_pool）。
    stopOverflowMonitor();

    const std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) {
        return;
    }

    // 刷盘剩余日志并销毁 logger/sink。spdlog::shutdown 会 flush 默认 logger 并清理注册表。
    spdlog::shutdown();
    m_initialized = false;
    m_lastOverrun = 0;
}

void LogManager::startOverflowMonitor()
{
    if (m_monitorRunning) {
        return;
    }
    m_monitorRunning = true;
    // 原子标志控制线程退出（监控线程只读，shutdown 置 false）。
    // 用裸 std::thread 而非 jthread，避免对 <stop_token> 的实现差异依赖。
    m_monitorThread = new std::thread([this]() {
        while (m_monitorRunning.load(std::memory_order_relaxed)) {
            // 采样 spdlog 线程池的 overrun_counter（被 overrun_oldest 丢弃的旧日志累计数）。
            auto tp = spdlog::thread_pool();
            if (tp == nullptr) {
                std::this_thread::sleep_for(kMonitorInterval);
                continue;
            }
            const size_t current = tp->overrun_counter();
            const size_t delta = current - m_lastOverrun;
            m_lastOverrun = current;
            if (delta > 0) {
                // 刻意绕过 spdlog 队列直接写 stderr：否则告警自身可能被 overrun_oldest
                // 丢弃，达不到"日志风暴时一定可见"的目的。fflush 确保立即输出。
                std::fprintf(stderr,
                    "[LogManager] WARNING: async log buffer overrun, %zu log(s) dropped "
                    "(overrun_oldest). Log storm detected, some old logs were discarded.\n",
                    delta);
                std::fflush(stderr);
            }
            std::this_thread::sleep_for(kMonitorInterval);
        }
    });
}

void LogManager::stopOverflowMonitor()
{
    if (!m_monitorRunning) {
        return;
    }
    m_monitorRunning = false;
    if (m_monitorThread != nullptr && m_monitorThread->joinable()) {
        m_monitorThread->join();
    }
    delete m_monitorThread;
    m_monitorThread = nullptr;
}

} // namespace mc::application
