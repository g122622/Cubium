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

#pragma once

#include <atomic>
#include <mutex>

namespace mc::application {

/**
 * @brief 异步日志管理器（spdlog 异步 logger + 溢出监控）
 *
 * 将 spdlog 全局默认 logger 替换为后台线程消费的 async_logger，使主线程（游戏 tick
 * 线程）的日志调用退化为"入队"操作，不再因 sink 写盘/染色阻塞主线程。
 *
 * 溢出策略采用 overrun_oldest（丢旧保新）：队列满时丢弃队列中最旧的日志腾位给新日志，
 * 主线程永不阻塞。崩溃前最后几条日志通常正是排查根因所需，故丢旧保新比丢新保旧更利于排查。
 *
 * 但 overrun_oldest 会在日志风暴时静默丢日志，排查时无从感知。为此额外起一个轻量监控线程，
 * 周期性采样 spdlog 线程池的 overrun_counter()，发现增量即经 fprintf(stderr) 直接输出告警——
 * 刻意绕过 spdlog 队列本身（否则告警自身可能被 overrun_oldest 丢弃，达不到"一定可见"的目的）。
 *
 * 生命周期：BaseApplicationEntry::run() 最早期调用 initialize()（早于 banner，保证后续所有
 * spdlog 调用走异步）；进程退出前（正常或异常路径）调用 shutdown()，须在 ProfilerManager
 * shutdown 之前，避免日志消费线程访问已销毁资源。
 *
 * 单例（Meyers 风格，与 ProfilerManager 一致）。
 */
class LogManager {
public:
    /// 异步日志队列容量（条数）。8192 条在日志风暴下能吸收瞬时尖峰，溢出由监控线程告警兜底。
    static constexpr size_t kQueueSize = 8192;

    /// 后台消费线程数。spdlog 异步模型单线程消费即可，多线程反而增加锁竞争。
    static constexpr size_t kWorkerCount = 1;

    /**
     * @brief 初始化异步日志：建线程池 + async_logger(overrun_oldest) + 设默认 logger + 启溢出监控。
     *
     * 重复调用安全：已初始化则直接返回。
     */
    void initialize();

    /**
     * @brief 关闭异步日志：停溢出监控线程 + spdlog::shutdown()（刷盘剩余日志）。
     *
     * 须在 ProfilerManager::shutdown() 之前调用。重复调用安全。
     */
    void shutdown();

    static LogManager& instance();

private:
    LogManager();
    ~LogManager();
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    /// 启动溢出监控线程（采样 overrun_counter 增量，fprintf(stderr) 告警）。
    void startOverflowMonitor();
    /// 停止并 join 溢出监控线程。
    void stopOverflowMonitor();

    std::mutex m_mutex;
    bool m_initialized{false};
    std::atomic<bool> m_monitorRunning{false};

    /// 溢出监控线程。裸指针 + 手动 join（避免 <thread> jthread 的 C++20 stop_token 依赖差异）。
    class std::thread* m_monitorThread{nullptr};

    /// 上次采样到的 overrun_counter 基线，用于算增量。
    size_t m_lastOverrun{0};
};

} // namespace mc::application
