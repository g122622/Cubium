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

#include "ServerConsole.hpp"

#include "common/application/LogManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/PlatformInfo.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/application/console/bridge/CompletionRequestQueue.hpp"
#include "server/application/console/bridge/ConsoleCommandQueue.hpp"
#include "server/application/console/bridge/ConsoleLogConsumer.hpp"
#include "server/application/console/bridge/GameEventFeed.hpp"
#include "server/application/console/bridge/TerminalCommandSource.hpp"
#include "server/application/console/ui/CommandLineComponent.hpp"
#include "server/application/console/ui/CompletionPopupComponent.hpp"
#include "server/application/console/ui/ConsoleLayout.hpp"
#include "server/application/console/ui/LogViewComponent.hpp"
#include "server/application/console/ui/StatusPanelComponent.hpp"
#include "server/application/console/ui/TerminalUi.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/network/session/ClientSessionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace mc::server::console {

using namespace mc::trace;

namespace {

/// 指标采样间隔：遍历全部玩家与维度的成本不必每 tick 付一次。
constexpr auto METRICS_INTERVAL = std::chrono::milliseconds(500);

/// TPS 的指数平滑系数（0..1，越小越平滑）。
constexpr f64 TPS_SMOOTHING = 0.1;

/// 处理"复制选中文本"的按键：Ctrl+Y（与多数终端复制习惯一致，且不与输入框冲突）。
bool _isCopyShortcut(const ftxui::Event& event)
{
    return event == ftxui::Event::CtrlY;
}

} // namespace

ServerConsole::ServerConsole(ServerConsoleOptions options)
    : m_options(std::move(options))
{}

ServerConsole::~ServerConsole()
{
    stop();
}

void ServerConsole::attach(MinecraftServer& server)
{
    MC_ASSERT_RELEASE_MSG(m_server == nullptr, "ServerConsole::attach called twice");
    MC_ASSERT_RELEASE_MSG(!m_started.load(), "ServerConsole::attach requires start() not called");

    m_server = &server;

    m_logs = std::make_unique<LogRingBuffer>(m_options.logCapacity);
    m_events = std::make_unique<EventRingBuffer>(m_options.eventCapacity);
    m_metrics = std::make_unique<ServerMetricsStore>();
    m_panelDetails = std::make_unique<PanelDetailStore>();
    server.setServerConsole(this);
    m_commands = std::make_unique<ConsoleCommandQueue>();
    m_completionRequests = std::make_unique<CompletionRequestQueue>();

    // 日志：在 UI 建立之前安装，启动期日志会先进入环形缓冲，UI 首帧即回填显示。
    m_logConsumer = std::make_shared<ConsoleLogConsumer>(*m_logs, []() {
        TerminalUi::requestRedraw();
    });
    mc::application::LogManager::installLineConsumer(m_logConsumer);

    // 事件：聊天/上下线/死亡/进度/天气。
    m_eventFeed = std::make_unique<GameEventFeed>(*m_events, []() {
        TerminalUi::requestRedraw();
    });

    // 指标初值：先放入一个空快照，避免 UI 首帧读到未写入的默认值。
    m_metrics->update(ServerMetricsSnapshot{});
    m_lastMetricsSample = std::chrono::steady_clock::now();
    m_lastTickTime = std::chrono::steady_clock::now();

    // 命令执行 / 指标采集 / 补全结算全部在 tick 线程的 post-tick 回调里完成。
    server.addPostTickCallback([this]() {
        _onPostTick();
    });
}

void ServerConsole::start()
{
    MC_ASSERT_RELEASE_MSG(m_server != nullptr, "ServerConsole::start requires attach() first");
    MC_ASSERT_RELEASE_MSG(!m_started.load(), "ServerConsole::start called twice");

    const ServerInfo info{m_options.version, m_options.worldName, m_options.port};
    const auto startedAt = std::chrono::steady_clock::now();

    m_logView = std::make_unique<LogViewComponent>(*m_logs, *m_events);
    m_statusPanel =
        std::make_unique<StatusPanelComponent>(*m_metrics, *m_panelDetails, *m_commandLine, info, startedAt);
    m_commandLine = std::make_unique<CommandLineComponent>(
        [this](const std::string& command) {
            _submitCommand(command);
        },
        [this](const std::string& input, i32 cursor) {
            return _queryCompletion(input, cursor);
        });
    m_completionPopup = std::make_unique<CompletionPopupComponent>(m_commandLine->completionState());

    m_layout = std::make_shared<ConsoleLayout>(
        *m_logView, *m_statusPanel, *m_commandLine, *m_completionPopup, m_options.serverName + " " + m_options.version);

    // 顶层包装：在布局处理之前拦截"复制选中文本"快捷键。
    m_rootComponent = ftxui::CatchEvent(m_layout, [this](ftxui::Event event) {
        if (_isCopyShortcut(event)) {
            if (TerminalUi::copySelectionToClipboard()) {
                m_events->push(TerminalEventKind::Info, "selection copied to clipboard");
            }
            return true;
        }
        return false;
    });

    m_started.store(true);
    m_uiThread = std::make_unique<std::thread>([this]() {
        TerminalUi::run(m_layout);
    });
}

void ServerConsole::stop()
{
    // 解绑服务端侧引用放在最前且不受 m_started 门控：attach 之后即便从未 start，
    // 服务端也不得再持有指向本对象（尤其析构路径）的指针。
    if (m_server != nullptr) {
        m_server->setServerConsole(nullptr);
    }

    if (!m_started.exchange(false)) {
        return;
    }

    // 1. 让 UI 循环退出并等待其恢复终端。
    TerminalUi::requestExit();
    if (m_uiThread != nullptr && m_uiThread->joinable()) {
        m_uiThread->join();
    }
    m_uiThread.reset();

    // 2. 唤醒可能仍在等待补全结果的 UI 调用（UI 线程已退出，这里只是兜底）。
    if (m_completionRequests != nullptr) {
        m_completionRequests->cancel();
    }

    // 3. 拆数据通路：先摘日志消费者，再退订事件。
    mc::application::LogManager::uninstallLineConsumer();
    m_logConsumer.reset();
    m_eventFeed.reset();
    m_server = nullptr;
}

bool ServerConsole::exitRequested() const
{
    return TerminalUi::exitRequested();
}

void ServerConsole::_onPostTick()
{
    // 控制台在 tick 线程上的全部工作：UI 线程从不直接触碰世界，这段就是跨线程开销的
    // 全部来源（指标采样 / 命令执行 / 补全结算三条支路分别有子事件）。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "ServerConsole::postTick");
    _collectMetrics();
    _drainCommands();
    _drainCompletionRequests();
}

void ServerConsole::_collectMetrics()
{
    const auto now = std::chrono::steady_clock::now();
    const f64 deltaSeconds = std::chrono::duration<f64>(now - m_lastTickTime).count();
    m_lastTickTime = now;

    if (deltaSeconds > 0.0) {
        const f64 instantTps = 1.0 / deltaSeconds;
        m_smoothedTps =
            m_smoothedTps <= 0.0 ? instantTps : m_smoothedTps + TPS_SMOOTHING * (instantTps - m_smoothedTps);
    }

    if (now - m_lastMetricsSample < METRICS_INTERVAL) {
        return;
    }
    m_lastMetricsSample = now;

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "ServerConsole::collectMetrics");

    ServerMetricsSnapshot snapshot;
    snapshot.tps = m_smoothedTps;
    snapshot.tickTimeMs = static_cast<f64>(m_server->debugStats().smoothedTickTimeMs.load(std::memory_order_relaxed));
    snapshot.workingSetMiB = util::PlatformInfo::getProcessMemoryMB();
    snapshot.commitMiB = util::PlatformInfo::getProcessCommitMB();
    if (const auto* sessions = m_server->clientSessionManager(); sessions != nullptr) {
        snapshot.sessionCount = sessions->sessionCount();
    }

    // 全部维度的区块/实体统计（与 _mainLoop 中的 trace 计数口径一致）。
    // ticking 区块与方块实体需要逐个区块查询，成本随区块数线性增长；采样间隔
    // METRICS_INTERVAL 把它们摊薄到每秒两次，且全部在 tick 线程完成。
    m_server->dimensionManager().forEachDimension([&snapshot](Dimension& dimension) {
        auto* serverDimension = static_cast<ServerDimension*>(&dimension);
        auto* world = serverDimension->world();
        if (world == nullptr) {
            return;
        }
        auto* chunkManager = world->chunkManager();
        snapshot.chunkHolderCount += static_cast<i64>(world->chunkCount());
        snapshot.loadedChunkCount += static_cast<i64>(world->loadedChunkCount());
        snapshot.entityCount += static_cast<i64>(world->entityManager().entityCount());
        if (chunkManager == nullptr) {
            return;
        }

        const i32 simulationDistance = world->entityManager().simulationDistance();
        chunkManager->forEachLoadedChunk([&snapshot, chunkManager, simulationDistance](const ChunkData& chunk) {
            if (chunkManager->ticketManager().isTickingChunk(chunk.x(), chunk.z(), simulationDistance)) {
                ++snapshot.tickingChunkCount;
            }
            snapshot.blockEntityCount += chunk.blockEntityCount();
            return true;
        });
    });

    // 在线玩家列表。
    m_server->playerManager().forEachPlayer([this, &snapshot](ServerPlayerData& data) {
        PlayerSummary summary;
        summary.username = data.username;
        summary.x = static_cast<f64>(data.x);
        summary.y = static_cast<f64>(data.y);
        summary.z = static_cast<f64>(data.z);

        auto* world = m_server->getPlayerWorld(data.playerId);
        const auto* dimension =
            world != nullptr ? m_server->dimensionManager().getDimension(world->dimension()) : nullptr;
        summary.dimensionName = dimension != nullptr ? dimension->type().name() : std::string("unknown");

        if (world != nullptr) {
            if (auto* entity = m_server->playerEntityManager().getPlayerEntity(data.playerId, *world);
                entity != nullptr) {
                summary.health = entity->health();
            }
        }
        snapshot.players.push_back(std::move(summary));
    });
    snapshot.playerCount = snapshot.players.size();
    std::sort(snapshot.players.begin(), snapshot.players.end(), [](const PlayerSummary& lhs, const PlayerSummary& rhs) {
        return lhs.username < rhs.username;
    });

    m_metrics->update(std::move(snapshot));
    // 空闲的 UI 线程阻塞等待唤醒；快照更新后必须主动叫醒它，否则面板要等到
    // 下一次用户输入才会刷新（旧实现靠 60 FPS 轮询，掩盖了这个缺失）。
    TerminalUi::requestRedraw();
}

void ServerConsole::_drainCommands()
{
    const auto commands = m_commands->drain();
    if (commands.empty()) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Console, "ServerConsole::drainCommands", "count", static_cast<i64>(commands.size()));

    // 控制台命令源与回执缓冲每批复用一份：命令源持有缓冲指针，构造开销可以忽略。
    TerminalCommandSource source(m_server, *m_events);
    for (const auto& command : commands) {
        auto result = m_server->commandRegistry().execute(command, source);
        if (result.failed()) {
            m_events->push(TerminalEventKind::Error, result.error().message());
        }
    }
    TerminalUi::requestRedraw();
}

void ServerConsole::_drainCompletionRequests()
{
    bool started = false;
    const auto input = m_completionRequests->takePending(started);
    if (!started) {
        return;
    }
    MC_ASSERT_RELEASE(input.has_value());

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "ServerConsole::completionRequest");

    TerminalCommandSource source(m_server, *m_events);
    auto future = m_server->commandRegistry().getSuggestions(*input, source);
    m_completionRequests->respond(future.get());
}

void ServerConsole::_submitCommand(const std::string& command)
{
    m_commands->submit(command);
    TerminalUi::requestRedraw();
}

mc::command::Suggestions ServerConsole::_queryCompletion(const std::string& input, i32 /*cursor*/)
{
    if (m_server == nullptr) {
        return mc::command::Suggestions::empty();
    }
    return m_completionRequests->query(input);
}

} // namespace mc::server::console
