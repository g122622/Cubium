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
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <spdlog/common.h>
#include <spdlog/sinks/sink.h>

namespace mc::application {

/**
 * @brief 可选的日志行消费者
 *
 * 供"把日志同时送往终端 UI"等场景使用：安装后，每条已格式化日志会额外回调一次
 * OnLogLine，回调运行在 spdlog 的异步消费线程上，实现方**不得在此阻塞**（只做入队）。
 *
 * 未安装消费者时（客户端、单元测试、默认的独立服务端），日志链路与历史行为完全一致。
 */
class ILogLineConsumer {
public:
    virtual ~ILogLineConsumer() = default;

    /**
     * @brief 接收一行已格式化日志
     * @param level 日志级别
     * @param line 已格式化文本（与终端 sink 输出同格式，不含换行）
     */
    virtual void OnLogLine(spdlog::level::level_enum level, std::string_view line) = 0;
};

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
    /// 异步日志队列容量（条数）。
    ///
    /// 该容量直接决定常驻内存：队列在初始化时一次性分配（槽位 = 容量 + 1，每条 async_msg 约
    /// 408 字节），实测 2048 条为 832KB、256 条为 104KB。策略为 overrun_oldest（丢旧保新、主线程
    /// 永不阻塞），故容量只影响「日志风暴时能吸收多长的尖峰」，不影响正确性；溢出由监控线程
    /// 经 stderr 告警兜底。启动期实测日志不过数百行、且消费线程与生产同速，256 条足以吸收
    /// 常规尖峰，同时把常驻内存压到最小。
    static constexpr size_t kQueueSize = 256;

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
     * 会先卸载已安装的日志行消费者（见 installLineConsumer）。
     */
    void shutdown();

    /**
     * @brief 安装可选的日志行消费者
     *
     * 向默认 logger 追加一个仅转发给消费者的 sink；现有的 stdout sink 保持不变，
     * 因此终端输出不重复、不改变。消费者必须存活到 uninstallLineConsumer() /
     * shutdown() 被调用为止。
     *
     * 须在 initialize() 之后调用，且只能安装一次（重复安装触发断言；先
     * uninstallLineConsumer() 再安装）。
     *
     * @param consumer 消费者；传 nullptr 为空操作
     */
    static void installLineConsumer(std::shared_ptr<ILogLineConsumer> consumer);

    /**
     * @brief 卸载日志行消费者（移除转发 sink）。未安装时为空操作。
     */
    static void uninstallLineConsumer();

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

    /// 溢出监控线程。显式 join（避免 jthread 的 C++20 stop_token 依赖差异）。
    std::unique_ptr<std::thread> m_monitorThread;

    /// 上次采样到的 overrun_counter 基线，用于算增量。
    size_t m_lastOverrun{0};

    /// 已安装的日志行消费者（空 = 未安装）。
    std::shared_ptr<ILogLineConsumer> m_lineConsumer;
    /// 转发 consumer 的 sink；安装时追加到默认 logger，卸载时移除。
    std::shared_ptr<spdlog::sinks::sink> m_lineSink;
};

} // namespace mc::application
