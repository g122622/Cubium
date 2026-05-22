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

/**
 * @file TriggerCommandTest.cpp
 * @brief TriggerCommand 单元测试
 *
 * 测试 /trigger 命令的所有子命令：
 * - trigger <objective>: 增加 1 分
 * - trigger <objective> add <value>: 增加指定分数
 * - trigger <objective> set <value>: 设置指定分数
 *
 * 测试触发器锁定机制：
 * - 触发器使用后自动锁定
 * - /scoreboard players enable 可以重新启用
 */

#include <gtest/gtest.h>

#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ScoreboardCommand.hpp"
#include "server/command/commands/TriggerCommand.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"

// 包含 TeamCommandTest.cpp 中定义的 TeamTestServer
#include "server/application/IServer.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/interaction/InventoryManager.hpp"

#include <stdexcept>
#include <vector>

namespace mc {
class ServerDimensionManager;
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
} // namespace mc

namespace mc::server {
class ServerPlayerEntityManager;
class ServerWorld;
class ServerChunkManager;
class EntityTracker;
class ItemPickupManager;
class WeatherManager;
} // namespace mc::server

namespace mc::server::sync {
class EntitySyncManager;
class ChunkSendManager;
class LightSyncManager;
} // namespace mc::server::sync

namespace mc::server::interaction {
class BlockInteractionManager;
class MiningManager;
class ContainerManager;
} // namespace mc::server::interaction

namespace mc::command {

/**
 * @brief 测试用的假连接
 */
class FakeConnectionForTrigger final : public network::IServerConnection {
public:
    void send(const u8* data, size_t size) override
    {
        if (data != nullptr && size > 0) {
            m_sentData.insert(m_sentData.end(), data, data + size);
        }
    }

    void disconnect(const std::string& reason = "") override
    {
        m_disconnectReason = reason;
        m_connected = false;
    }

    [[nodiscard]] bool isConnected() const override { return m_connected; }
    [[nodiscard]] std::string identifier() const override { return "FakeConnection"; }
    [[nodiscard]] network::ConnectionType type() const override { return network::ConnectionType::Local; }
    [[nodiscard]] std::string getAddress() const override { return ""; }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

/**
 * @brief 测试服务器，用于 TriggerCommand 测试
 */
class TriggerTestServer final : public server::IServer {
public:
    TriggerTestServer()
        : m_playerManager(20)
        , m_inventoryManager(m_playerManager)
        , m_connectionManager(m_playerManager)
        , m_timeManager(0, 1000)
        , m_teleportManager(m_playerManager)
        , m_keepAliveManager(m_playerManager, 15000, 30000)
        , m_positionTracker(m_playerManager, 10)
        , m_packetHandler(m_playerManager,
              m_connectionManager,
              m_teleportManager,
              m_keepAliveManager,
              m_positionTracker,
              m_timeManager,
              GameMode::Survival)
        , m_gameModeManager(m_playerManager, m_connectionManager)
        , m_commandRegistry()
        , m_scoreboard(*this)
    {
        // 注册内置判据
        scoreboard::ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
    }

    // IServer 接口实现
    [[nodiscard]] Result<void> initialize() override { return Result<void>::ok(); }
    void shutdown() override { m_running = false; }
    void tick() override {}
    [[nodiscard]] bool isRunning() const override { return m_running; }

    [[nodiscard]] bool isIntegrated() const noexcept override { return false; }
    [[nodiscard]] bool isDedicated() const noexcept override { return true; }

    [[nodiscard]] server::core::PlayerManager& playerManager() override { return m_playerManager; }
    [[nodiscard]] const server::core::PlayerManager& playerManager() const override { return m_playerManager; }
    [[nodiscard]] server::core::ConnectionManager& connectionManager() override { return m_connectionManager; }
    [[nodiscard]] const server::core::ConnectionManager& connectionManager() const override
    {
        return m_connectionManager;
    }
    [[nodiscard]] server::core::TimeManager& timeManager() override { return m_timeManager; }
    [[nodiscard]] const server::core::TimeManager& timeManager() const override { return m_timeManager; }
    [[nodiscard]] server::core::TeleportManager& teleportManager() override { return m_teleportManager; }
    [[nodiscard]] const server::core::TeleportManager& teleportManager() const override { return m_teleportManager; }
    [[nodiscard]] server::core::KeepAliveManager& keepAliveManager() override { return m_keepAliveManager; }
    [[nodiscard]] const server::core::KeepAliveManager& keepAliveManager() const override { return m_keepAliveManager; }
    [[nodiscard]] server::core::PositionTracker& positionTracker() override { return m_positionTracker; }
    [[nodiscard]] const server::core::PositionTracker& positionTracker() const override { return m_positionTracker; }
    [[nodiscard]] server::core::PacketHandler& packetHandler() override { return m_packetHandler; }
    [[nodiscard]] const server::core::PacketHandler& packetHandler() const override { return m_packetHandler; }
    [[nodiscard]] server::core::GameModeManager& gameModeManager() override { return m_gameModeManager; }
    [[nodiscard]] const server::core::GameModeManager& gameModeManager() const override { return m_gameModeManager; }
    [[nodiscard]] server::core::WhitelistManager& whitelistManager() override { return m_whitelistManager; }
    [[nodiscard]] const server::core::WhitelistManager& whitelistManager() const override { return m_whitelistManager; }
    [[nodiscard]] server::core::BannedPlayerList& bannedPlayerList() override { return m_bannedPlayerList; }
    [[nodiscard]] const server::core::BannedPlayerList& bannedPlayerList() const override { return m_bannedPlayerList; }
    [[nodiscard]] server::core::BannedIpList& bannedIpList() override { return m_bannedIpList; }
    [[nodiscard]] const server::core::BannedIpList& bannedIpList() const override { return m_bannedIpList; }
    [[nodiscard]] server::core::OpListManager& opListManager() override { return m_opListManager; }
    [[nodiscard]] const server::core::OpListManager& opListManager() const override { return m_opListManager; }

    [[nodiscard]] ServerDimensionManager& dimensionManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override { throw std::logic_error("unused"); }

    [[nodiscard]] server::ServerWorld* getPlayerWorld(PlayerId) override { return nullptr; }

    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] const server::ServerPlayerEntityManager& playerEntityManager() const override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] server::interaction::BlockInteractionManager& blockInteractionManager() override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] const server::interaction::BlockInteractionManager& blockInteractionManager() const override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] server::interaction::MiningManager& miningManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::MiningManager& miningManager() const override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] server::interaction::ContainerManager& containerManager() override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] const server::interaction::ContainerManager& containerManager() const override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] server::interaction::InventoryManager& inventoryManager() override { return m_inventoryManager; }
    [[nodiscard]] const server::interaction::InventoryManager& inventoryManager() const override
    {
        return m_inventoryManager;
    }
    [[nodiscard]] PlayerInventory* playerInventory(PlayerId playerId) override
    {
        return m_inventoryManager.getInventory(playerId);
    }
    [[nodiscard]] const PlayerInventory* playerInventory(PlayerId playerId) const override
    {
        return m_inventoryManager.getInventory(playerId);
    }

    [[nodiscard]] CommandRegistry& commandRegistry() override { return m_commandRegistry; }
    [[nodiscard]] const CommandRegistry& commandRegistry() const override { return m_commandRegistry; }

    [[nodiscard]] server::ServerScoreboard& scoreboard() override { return m_scoreboard; }
    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override { return m_scoreboard; }
    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override
    {
        throw std::logic_error("unused");
    }

    [[nodiscard]] resource::DataPackList& dataPackList() override { throw std::logic_error("unused"); }
    [[nodiscard]] const resource::DataPackList& dataPackList() const override { throw std::logic_error("unused"); }

    [[nodiscard]] loot::LootTableManager& lootTableManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const loot::LootTableManager& lootTableManager() const override { throw std::logic_error("unused"); }

    [[nodiscard]] i32 viewDistance() const override { return m_viewDistance; }
    [[nodiscard]] i32 maxPlayers() const override { return m_maxPlayers; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return m_timeManager.currentTick(); }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty difficulty) override { m_difficulty = difficulty; }
    [[nodiscard]] GameMode defaultGameMode() const override { return m_defaultGameMode; }
    void setDefaultGameMode(GameMode mode) override { m_defaultGameMode = mode; }
    [[nodiscard]] i32 playerIdleTimeoutMinutes() const override { return m_idleTimeoutMinutes; }
    void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) override { m_idleTimeoutMinutes = timeoutMinutes; }
    void broadcastServerMessage(std::string_view message) override { m_lastBroadcastMessage = std::string(message); }
    void requestStop() override
    {
        m_stopRequested = true;
        m_running = false;
    }

    void broadcastParticleInRange(u32, f64, f64, f64, f32, f32, f32, f32, f32, f32, u32, f32) override {}

    void sendSoundToPlayer(PlayerId, const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {}

private:
    i32 m_viewDistance = 10;
    i32 m_maxPlayers = 20;
    u64 m_seed = 0;
    bool m_running = true;
    Difficulty m_difficulty = Difficulty::Normal;
    GameMode m_defaultGameMode = GameMode::Survival;
    i32 m_idleTimeoutMinutes = 0;
    bool m_stopRequested = false;
    std::string m_lastBroadcastMessage;

    server::core::PlayerManager m_playerManager;
    server::interaction::InventoryManager m_inventoryManager;
    server::core::ConnectionManager m_connectionManager;
    server::core::TimeManager m_timeManager;
    server::core::TeleportManager m_teleportManager;
    server::core::KeepAliveManager m_keepAliveManager;
    server::core::PositionTracker m_positionTracker;
    server::core::PacketHandler m_packetHandler;
    server::core::GameModeManager m_gameModeManager;
    server::core::WhitelistManager m_whitelistManager;
    server::core::BannedPlayerList m_bannedPlayerList;
    server::core::BannedIpList m_bannedIpList;
    server::core::OpListManager m_opListManager;
    CommandRegistry m_commandRegistry;
    server::ServerScoreboard m_scoreboard;
};

class TriggerCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        TriggerCommand::registerTo(m_server.commandRegistry().dispatcher());
        ScoreboardCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    TriggerTestServer m_server;
};

// ========== 命令注册测试 ==========

TEST_F(TriggerCommandTest, TriggerCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "trigger") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "trigger command should be registered";
}

// ========== 触发器基本功能测试 ==========

TEST_F(TriggerCommandTest, TriggerObjectiveNotExists)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger nonexistent", source);
    // 目标不存在时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, TriggerNotTriggerCriteria)
{
    auto& scoreboard = m_server.scoreboard();
    auto* dummyCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(dummyCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_dummy", *dummyCriteria);
    ASSERT_NE(objective, nullptr);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger test_dummy", source);
    // 非 trigger 判据时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, TriggerNotPrimed)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 玩家没有分数，触发器未准备
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger test_trigger", source);
    // 未准备时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, TriggerSuccess)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 为玩家准备触发器（创建分数并解锁）
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setLocked(false);

    ServerCommandSource source(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger", source);

    EXPECT_EQ(result.value(), 1);
    EXPECT_EQ(score->getScorePoints(), 1);
    EXPECT_TRUE(score->isLocked()); // 触发后自动锁定
}

TEST_F(TriggerCommandTest, TriggerAddSuccess)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(10);
    score->setLocked(false);

    ServerCommandSource source(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger add 5", source);

    EXPECT_EQ(result.value(), 15);
    EXPECT_EQ(score->getScorePoints(), 15);
    EXPECT_TRUE(score->isLocked());
}

TEST_F(TriggerCommandTest, TriggerSetSuccess)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(10);
    score->setLocked(false);

    ServerCommandSource source(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger set 100", source);

    EXPECT_EQ(result.value(), 100);
    EXPECT_EQ(score->getScorePoints(), 100);
    EXPECT_TRUE(score->isLocked());
}

TEST_F(TriggerCommandTest, TriggerAlreadyUsed)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setLocked(true); // 已锁定

    ServerCommandSource source(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger", source);
    // 已锁定的触发器应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

// ========== Scoreboard players enable 测试 ==========

TEST_F(TriggerCommandTest, EnableTriggerViaScoreboard)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 使用 scoreboard players enable 启用触发器
    ServerCommandSource adminSource(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto enableResult =
        m_server.commandRegistry().execute("/scoreboard players enable Steve test_trigger", adminSource);
    EXPECT_EQ(enableResult.value(), 1);

    // 验证触发器已准备
    auto* score = scoreboard.getScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    EXPECT_FALSE(score->isLocked());

    // 现在可以使用触发器
    ServerCommandSource playerSource(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto triggerResult = m_server.commandRegistry().execute("/trigger test_trigger", playerSource);
    EXPECT_EQ(triggerResult.value(), 1);
    EXPECT_EQ(score->getScorePoints(), 1);
    EXPECT_TRUE(score->isLocked());
}

TEST_F(TriggerCommandTest, EnableNonTriggerObjective)
{
    auto& scoreboard = m_server.scoreboard();
    auto* dummyCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(dummyCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_dummy", *dummyCriteria);
    ASSERT_NE(objective, nullptr);

    ServerCommandSource source(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto result = m_server.commandRegistry().execute("/scoreboard players enable Steve test_dummy", source);
    // 非 trigger 判据时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, EnableNonExistentObjective)
{
    ServerCommandSource source(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto result = m_server.commandRegistry().execute("/scoreboard players enable Steve nonexistent", source);
    // 目标不存在时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

// ========== 分数锁定机制测试 ==========

TEST_F(TriggerCommandTest, ScoreLockMechanism)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("trigger_test", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 新分数默认不锁定
    auto* score = scoreboard.getOrCreateScore("Player1", *objective);
    EXPECT_FALSE(score->isLocked());

    // 手动设置锁定
    score->setLocked(true);
    EXPECT_TRUE(score->isLocked());

    // reset() 会重置锁定状态
    score->reset();
    EXPECT_FALSE(score->isLocked());
    EXPECT_EQ(score->getScorePoints(), 0);
}

// ========== 触发器重新启用测试 ==========

TEST_F(TriggerCommandTest, ReenableTrigger)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("retrigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 准备触发器
    ServerCommandSource adminSource(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    m_server.commandRegistry().execute("/scoreboard players enable Player1 retrigger", adminSource);

    // 第一次触发
    ServerCommandSource playerSource(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Player1");
    auto result1 = m_server.commandRegistry().execute("/trigger retrigger add 10", playerSource);
    EXPECT_EQ(result1.value(), 10);

    auto* score = scoreboard.getScore("Player1", *objective);
    ASSERT_NE(score, nullptr);
    EXPECT_TRUE(score->isLocked());

    // 尝试再次触发应该失败（已锁定）
    auto result2 = m_server.commandRegistry().execute("/trigger retrigger add 5", playerSource);
    EXPECT_EQ(result2.value(), 0);
    // 分数应该仍然是 10
    EXPECT_EQ(score->getScorePoints(), 10);

    // 重新启用触发器
    m_server.commandRegistry().execute("/scoreboard players enable Player1 retrigger", adminSource);
    EXPECT_FALSE(score->isLocked());

    // 现在可以再次触发
    auto result3 = m_server.commandRegistry().execute("/trigger retrigger add 5", playerSource);
    EXPECT_EQ(result3.value(), 15); // 10 + 5
}

// ========== 玩家权限测试 ==========

TEST_F(TriggerCommandTest, ConsoleCannotUseTrigger)
{
    // 控制台命令源（不是玩家）
    ServerCommandSource consoleSource = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger test", consoleSource);
    // 控制台不能使用 trigger 命令
    EXPECT_EQ(result.value(), 0);
}

// ========== /scoreboard players list 测试 ==========

TEST_F(TriggerCommandTest, ListPlayers)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 为玩家设置分数
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(42);

    ServerCommandSource source(&m_server, nullptr, nullptr, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto result = m_server.commandRegistry().execute("/scoreboard players list Steve", source);
    EXPECT_EQ(result.value(), 1);
}

} // namespace mc::command
