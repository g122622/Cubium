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
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "common/util/assert/AssertAll.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <thread>
#include <utility>
#include <vector>

namespace mc::application {

namespace {

/// 溢出监控线程的采样间隔。1s 足够及时告警，又不过频占用 CPU。
constexpr auto kMonitorInterval = std::chrono::seconds(1);

/**
 * @brief 把已格式化日志转发给 ILogLineConsumer 的 sink
 *
 * 只在 spdlog 异步消费线程上被调用。格式化后立刻回调消费者；消费者约定只做入队，
 * 因此这里没有额外缓冲。
 */
class LineConsumerSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit LineConsumerSink(std::shared_ptr<ILogLineConsumer> consumer)
        : m_consumer(std::move(consumer))
    {
        // 与默认 logger 的终端 sink 使用同一格式，保证 UI 中看到的文本与控制台一致。
        this->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        // 去掉格式化结果末尾的换行：消费者按"一行一条"存储。
        // fmt::basic_memory_buffer 没有 empty()/back()，按 data()+size() 自行裁剪。
        usize size = formatted.size();
        char* data = formatted.data();
        while (size > 0 && (data[size - 1] == '\n' || data[size - 1] == '\r')) {
            --size;
        }

        if (m_consumer) {
            m_consumer->OnLogLine(msg.level, std::string_view(data, size));
        }
    }

    void flush_() override {}

private:
    std::shared_ptr<ILogLineConsumer> m_consumer;
};

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

void LogManager::installLineConsumer(std::shared_ptr<ILogLineConsumer> consumer)
{
    if (consumer == nullptr) {
        return;
    }

    LogManager& manager = instance();
    std::lock_guard<std::mutex> lock(manager.m_mutex);

    MC_ASSERT_RELEASE_MSG(manager.m_initialized, "LogManager::installLineConsumer requires initialize() first");
    MC_ASSERT_RELEASE_MSG(manager.m_lineSink == nullptr, "LogManager line consumer already installed");

    auto logger = spdlog::default_logger();
    MC_ASSERT_RELEASE_MSG(logger != nullptr, "LogManager default logger missing");

    manager.m_lineConsumer = std::move(consumer);
    manager.m_lineSink = std::make_shared<LineConsumerSink>(manager.m_lineConsumer);
    logger->sinks().push_back(manager.m_lineSink);
}

void LogManager::uninstallLineConsumer()
{
    LogManager& manager = instance();

    // 先取出并置空成员，再在锁外从 logger 摘除：spdlog 异步 logger 的 sinks() 由 logger 自身
    // 互斥保护，与 m_mutex 无关；此处也不依赖二者顺序。
    std::shared_ptr<ILogLineConsumer> consumer;
    std::shared_ptr<spdlog::sinks::sink> sink;
    {
        std::lock_guard<std::mutex> lock(manager.m_mutex);
        consumer = std::move(manager.m_lineConsumer);
        sink = std::move(manager.m_lineSink);
    }
    if (sink == nullptr) {
        return;
    }

    auto logger = spdlog::default_logger();
    if (logger != nullptr) {
        auto& sinks = logger->sinks();
        sinks.erase(std::remove_if(sinks.begin(),
                        sinks.end(),
                        [&sink](const std::shared_ptr<spdlog::sinks::sink>& candidate) {
                            return candidate == sink;
                        }),
            sinks.end());
    }
    // sink/consumer 在函数返回时销毁，晚于从 logger 摘除：异步消费线程不会再访问它们。
}

void LogManager::shutdown()
{
    // 先卸载日志行消费者：消费者可能持有 UI 对象，须在 UI 销毁前停止接收日志。
    uninstallLineConsumer();

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
    // 用 std::thread 而非 jthread，避免对 <stop_token> 的实现差异依赖。
    m_monitorThread = std::make_unique<std::thread>([this]() {
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
    m_monitorThread.reset();
}

} // namespace mc::application
