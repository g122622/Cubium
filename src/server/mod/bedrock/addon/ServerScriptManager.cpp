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

#include "server/mod/bedrock/addon/ServerScriptManager.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/bossbar/BossInfo.hpp" // bossInfoColorToName/bossInfoOverlayToName（BossBarView color/overlay 字符串化）
#include "server/bossbar/CustomServerBossInfoManager.hpp" // bossBarManager().getIds/get（world.bossbar 回调）
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/dimension/ServerDimensionManager.hpp" // getOverworld（world.getDefaultSpawn 回调取主世界）
#include "server/event/ServerEventBus.hpp"
#include "server/mod/bedrock/addon/bridge/ServerEventSignals.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp" // worldSpawnPoint/spawnAngle（world.getDefaultSpawn 回调）

#include <memory>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::server {

ServerScriptManager::ServerScriptManager(const std::string& globalBehaviorPackDir)
    : m_scriptManager(std::make_unique<mc::mod::bedrock::addon::ScriptManager>())
    , m_globalBehaviorPackDir(globalBehaviorPackDir)
{}

ServerScriptManager::~ServerScriptManager()
{
    shutdown();
}

Result<void> ServerScriptManager::initialize()
{
    if (m_initialized) {
        return Result<void>::ok();
    }

    spdlog::info("[Server] Initializing script system...");

    // 注入服务端事件信号定义到脚本管理器
    auto beforeSignals = getBeforeEventSignals();
    auto afterSignals = getAfterEventSignals();
    beforeSignals.insert(beforeSignals.end(), afterSignals.begin(), afterSignals.end());
    m_scriptManager->setEventSignals(beforeSignals);

    auto result = m_scriptManager->initialize();
    if (result.failed()) {
        return result;
    }

    m_initialized = true;

    // 桥接游戏事件到脚本事件总线
    m_eventBridge.initialize(event::ServerEventBus::instance(), m_scriptManager->eventBus());

    spdlog::info("[Server] Script system initialized");
    return Result<void>::ok();
}

Result<void> ServerScriptManager::loadPlugins(const std::string& worldBehaviorPackDir)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "Script system not initialized");
    }

    spdlog::info("[Server] Loading behavior packs...");

    auto result = m_scriptManager->loadPacks(m_globalBehaviorPackDir, worldBehaviorPackDir);
    if (result.failed()) {
        spdlog::error("[Server] Failed to load behavior packs: {}", result.error().message());
        return result;
    }

    // 自动启动所有已加载的插件
    m_scriptManager->startPlugins();

    spdlog::info("[Server] Behavior packs loaded and plugins started");
    return Result<void>::ok();
}

void ServerScriptManager::startPlugins()
{
    if (!m_initialized) {
        return;
    }

    m_scriptManager->startPlugins();
}

void ServerScriptManager::tick(u64 currentTick)
{
    if (!m_initialized) {
        return;
    }

    // tick流程：
    // 1. 开始tick（看门狗计时）
    m_scriptManager->watchdog().beginTick();

    // 2. 执行调度回调（system.run/runInterval/runTimeout）
    m_scriptManager->scheduler().tick(currentTick);

    // 3. 执行JS pending jobs和插件tick
    m_scriptManager->tickPlugins();
    m_scriptManager->executePendingJobs();

    // 4. 刷新afterEvent队列
    m_scriptManager->eventBus().tick();

    // 5. 结束tick（看门狗检查）
    m_scriptManager->watchdog().endTick();
    m_scriptManager->watchdog().tick(*m_scriptManager);
}

void ServerScriptManager::shutdown()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("[Server] Shutting down script system...");
    m_eventBridge.shutdown();
    m_scriptManager->shutdown();
    m_initialized = false;
    spdlog::info("[Server] Script system shut down");
}

void ServerScriptManager::reload()
{
    if (!m_initialized) {
        return;
    }

    m_scriptManager->reload();
}

bool ServerScriptManager::isInitialized() const
{
    return m_initialized;
}

mc::mod::bedrock::addon::ScriptManager& ServerScriptManager::scriptManager()
{
    return *m_scriptManager;
}

const mc::mod::bedrock::addon::ScriptManager& ServerScriptManager::scriptManager() const
{
    return *m_scriptManager;
}

void ServerScriptManager::setGlobalBehaviorPackDir(const std::string& dir)
{
    m_globalBehaviorPackDir = dir;
}

void ServerScriptManager::setServer(MinecraftServer* server)
{
    if (!server) {
        return;
    }

    auto& accessor = mc::mod::bedrock::addon::ScriptWorldAccessor::instance();

    // 桥接world.sendMessage到服务器日志（原 IServer::broadcastServerMessage 实现即只
    // spdlog 不发包；批5b 已从 IServer 删除该纯虚，脚本消息直接打日志）。
    accessor.setMessageCallback([](const std::string& message) { spdlog::info("[System] {}", message); });

    // 桥接world.getAllPlayers到玩家管理器
    accessor.setGetPlayerNamesCallback([server]() -> std::vector<std::string> {
        std::vector<std::string> names;
        server->playerManager().forEachPlayer(
            [&names](const mc::server::ServerPlayerData& data) { names.push_back(data.username); });
        return names;
    });

    // 桥接system.currentTick到服务器tick计数
    accessor.setCurrentTickCallback([server]() -> u64 { return server->currentTick(); });

    // 桥接world.getDimension到ServerDimensionManager（按维度名解析->ServerDimension->world()）。
    // 读入归一化：基岩 API 用 "minecraft:nether"，内部注册表（DimensionType.cpp:79）注册的是
    // "minecraft:the_nether"，须在回调内做名称归一化。Dimension.id 输出侧用基岩名（对称）。
    accessor.setGetDimensionCallback([server](const std::string& dimensionId) -> mc::IWorld* {
        std::string normalized = dimensionId;
        if (normalized == "minecraft:nether") {
            normalized = "minecraft:the_nether";
        }
        const mc::DimensionId dimId = server->dimensionManager().getDimensionIdByName(normalized);
        auto* dim = server->dimensionManager().getDimension(dimId);
        return dim != nullptr ? dim->world() : nullptr; // ServerWorld* -> IWorld*
    });

    // 桥接 world.scoreboard 到服务器 ServerScoreboard（向上转为 Scoreboard 基类指针）。
    // 供 GameTest JS 经 world.scoreboard.getObjective(name).getScore(participant) 读取分数做断言。
    accessor.setGetScoreboardCallback([server]() -> mc::scoreboard::Scoreboard* { return &server->scoreboard(); });

    // 桥接 world.bossbar 到服务器 CustomServerBossInfoManager。BossBar 类型在 server 层，common 层以
    // BossBarView 值快照桥接（避免 common→server 依赖）。供 /bossbar 命令测试经 world.bossbar.get(id)
    // 读取 BossBar 属性（value/max/color/overlay/visible/name）做断言。每次访问重新取快照保证实时性。
    accessor.setGetBossBarIdsCallback([server]() -> std::vector<std::string> {
        std::vector<std::string> ids;
        for (const auto& id : server->bossBarManager().getIds()) {
            ids.push_back(id.toString());
        }
        return ids;
    });
    accessor.setGetBossBarCallback([server](const std::string& idStr) -> mc::mod::bedrock::addon::BossBarView {
        mc::mod::bedrock::addon::BossBarView view;
        const ResourceLocation id(idStr);
        const auto* bar = server->bossBarManager().get(id);
        if (bar == nullptr) {
            return view; // exists=false
        }
        view.exists = true;
        view.id = bar->id().toString();
        view.name = bar->name().getUnformattedText();
        view.value = bar->value();
        view.max = bar->max();
        view.color = bossInfoColorToName(bar->color());
        view.overlay = bossInfoOverlayToName(bar->overlay());
        view.visible = bar->visible();
        // players 用 playerUuids（UUID 字符串集合）；脚本侧断言成员数足够，名字转换留待需要时补。
        for (const auto& uuid : bar->playerUuids()) {
            view.players.push_back(uuid);
        }
        return view;
    });

    // 桥接 world.getDefaultSpawn 到主世界 ServerWorld::worldSpawnPoint/spawnAngle。ServerWorld 在 server 层，
    // common 层以 WorldSpawnView 值快照桥接。供 /setworldspawn 命令测试读取世界出生点做断言。每次访问
    // 重新取快照保证 set 后 JS 立即可见。/setworldspawn 作用于命令执行者所在维度，测试在主世界故取 overworld。
    accessor.setGetWorldSpawnCallback([server]() -> mc::mod::bedrock::addon::WorldSpawnView {
        mc::mod::bedrock::addon::WorldSpawnView view;
        auto* overworld = server->dimensionManager().getOverworld();
        if (overworld == nullptr || overworld->world() == nullptr) {
            return view; // exists=false
        }
        const auto* world = overworld->world();
        view.exists = true;
        view.x = world->worldSpawnPoint().x;
        view.y = world->worldSpawnPoint().y;
        view.z = world->worldSpawnPoint().z;
        view.angle = world->spawnAngle();
        return view;
    });

    spdlog::info("[Server] ScriptWorldAccessor bridged to MinecraftServer");
}

} // namespace mc::server
