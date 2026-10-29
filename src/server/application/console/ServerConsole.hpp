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

#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Types.hpp"
#include "server/application/console/ServerConsoleOptions.hpp"
#include "server/application/console/buffer/EventRingBuffer.hpp"
#include "server/application/console/buffer/LogRingBuffer.hpp"
#include "server/application/console/metrics/ServerMetricsSnapshot.hpp"
#include "server/application/console/ui/PanelDetailStore.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <ftxui/component/component_base.hpp>

namespace mc::server {
class MinecraftServer;
}

namespace mc::server::console {

class CommandLineComponent;
class CompletionRequestQueue;
class ConsoleCommandQueue;
class ConsoleLayout;
class ConsoleLogConsumer;
class CompletionPopupComponent;
class GameEventFeed;
class LogViewComponent;
class StatusPanelComponent;

/**
 * @brief 交互式终端门面（--interactive）
 *
 * 生命周期：构造 → attach(server) → start() → …… → stop()
 * - attach：安装日志行消费者、订阅游戏事件、注册 post-tick 回调（指标采样 + 命令执行 +
 *   补全结算）；
 * - start：创建 UI 线程，在专用线程上跑 FTXUI（`TerminalUi::run`）；
 * - stop：请求 UI 退出并 join，再卸载日志消费者与事件订阅，最后恢复入口的信号处理。
 *
 * 线程边界：世界状态（玩家、区块、命令树）只在 tick 线程访问；UI 线程只读取指标快照、
 * 环形缓冲与命令在 tick 线程产出的结果。
 */
class ServerConsole {
public:
    /**
     * @brief 构造
     * @param options 终端显示参数
     */
    explicit ServerConsole(ServerConsoleOptions options);

    ~ServerConsole();

    ServerConsole(const ServerConsole&) = delete;
    ServerConsole& operator=(const ServerConsole&) = delete;

    /**
     * @brief 绑定服务端并安装各条数据通路
     *
     * 须在服务端 initialize() 之后、run() 之前调用：日志消费者此时安装，启动期日志
     * 会进入环形缓冲并在 UI 首帧回填显示。
     *
     * @param server 服务端（须比本对象存活更久）
     */
    void attach(MinecraftServer& server);

    /**
     * @brief 启动 UI 线程（须在 attach 之后调用）
     */
    void start();

    /**
     * @brief 停止 UI 并拆除全部数据通路（幂等）
     */
    void stop();

    /**
     * @brief UI 是否已请求退出（用户 Ctrl+Q/Ctrl+C 或 /stop 后的通知）
     *
     * 入口主循环据此提前进入关服流程；UI 退出本身不会停止服务端。
     */
    [[nodiscard]] bool exitRequested() const;

    /**
     * @brief 右侧面板「自定义指标」段的存储
     *
     * 由 `/panel` 命令在 tick 线程写入（区块状态/方块分布/群系分布等按需统计），
     * UI 线程读取。须在 attach() 之后使用。
     *
     * @return 面板自定义行存储
     */
    [[nodiscard]] PanelDetailStore& panelDetails() noexcept
    {
        return *m_panelDetails;
    }

private:
    /**
     * @brief post-tick 回调（tick 线程）
     */
    void _onPostTick();

    /**
     * @brief 采集指标快照并写入快照存储（tick 线程）
     */
    void _collectMetrics();

    /**
     * @brief 执行排队中的控制台命令（tick 线程）
     */
    void _drainCommands();

    /**
     * @brief 结算挂起的补全请求（tick 线程）
     */
    void _drainCompletionRequests();

    /**
     * @brief UI 线程：命令输入提交
     * @param command 不含 '/' 的命令文本
     */
    void _submitCommand(const std::string& command);

    /**
     * @brief UI 线程：补全查询（内部转交 tick 线程同步结算）
     */
    mc::command::Suggestions _queryCompletion(const std::string& input, i32 cursor);

    ServerConsoleOptions m_options;

    MinecraftServer* m_server = nullptr;

    // 数据缓冲（生产端与 UI 端共享）
    std::unique_ptr<LogRingBuffer> m_logs;
    std::unique_ptr<EventRingBuffer> m_events;
    std::unique_ptr<ServerMetricsStore> m_metrics;
    /// `/panel` 写入、面板读取的自定义指标行。
    std::unique_ptr<PanelDetailStore> m_panelDetails;
    std::unique_ptr<ConsoleCommandQueue> m_commands;
    std::unique_ptr<CompletionRequestQueue> m_completionRequests;

    // 桥接层
    std::shared_ptr<ConsoleLogConsumer> m_logConsumer;
    std::unique_ptr<GameEventFeed> m_eventFeed;

    // UI 组件
    std::unique_ptr<LogViewComponent> m_logView;
    std::unique_ptr<StatusPanelComponent> m_statusPanel;
    std::unique_ptr<CommandLineComponent> m_commandLine;
    std::unique_ptr<CompletionPopupComponent> m_completionPopup;
    /// 顶层组件：包装布局并处理"复制选中文本"快捷键（布局自身不知道剪贴板）。
    std::shared_ptr<ftxui::ComponentBase> m_rootComponent;
    /// 布局本体；作为组件挂到 m_rootComponent 下，同时保留引用供 UI 线程使用。
    std::shared_ptr<ConsoleLayout> m_layout;

    std::unique_ptr<std::thread> m_uiThread;
    std::atomic<bool> m_started{false};

    /// 指标采集节流：最多每 500ms 采一次（遍历玩家的成本不必每 tick 付）。
    std::chrono::steady_clock::time_point m_lastMetricsSample;
    /// 上一 tick 的时间戳，用于估算 TPS。
    std::chrono::steady_clock::time_point m_lastTickTime;
    f64 m_smoothedTps = 0.0;
};

} // namespace mc::server::console
