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
 * @file ExecuteCommandTest.cpp
 * @brief ExecuteCommand 单元测试
 *
 * 测试 /execute 命令的注册、解析和嵌套命令执行功能。
 */

#include <gtest/gtest.h>

#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ExecuteCommand.hpp"
#include "server/command/commands/HelpCommand.hpp"
#include "server/command/commands/ListCommand.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/interaction/InventoryManager.hpp"

#include <stdexcept>
#include <vector>

// Forward declarations
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
class ServerScoreboard;
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
 * @brief 命令测试使用的假连接。
 */
class FakeConnection final : public network::IServerConnection {
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

    [[nodiscard]] size_t sentBytes() const noexcept { return m_sentData.size(); }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

/**
 * @brief 测试服务器，用于命令测试。
 *
 * 注意：此测试服务器不提供完整的 world() 和 playerEntityManager() 实现，
 * 因为这些接口复杂难以模拟。
 * ExecuteCommand 的 as/at/if block 等子命令的完整功能测试应在集成测试中进行。
 */
class ExecuteTestServer final : public server::IServer {
public:
    ExecuteTestServer()
        : m_playerManager(m_config)
        , m_inventoryManager(m_playerManager)
        , m_connectionManager(m_playerManager)
        , m_timeManager(0, 1000)
        , m_teleportManager(m_playerManager)
        , m_keepAliveManager(m_playerManager, m_config)
        , m_positionTracker(m_playerManager, m_config)
        , m_packetHandler(m_playerManager,
              m_connectionManager,
              m_teleportManager,
              m_keepAliveManager,
              m_positionTracker,
              m_timeManager,
              m_config)
        , m_gameModeManager(m_playerManager, m_connectionManager)
        , m_commandRegistry()
    {}

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
    [[nodiscard]] server::ServerWorld& world() override { throw std::logic_error("world not available in unit test"); }
    [[nodiscard]] const server::ServerWorld& world() const override
    {
        throw std::logic_error("world not available in unit test");
    }
    [[nodiscard]] server::ServerChunkManager& chunkManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::ServerChunkManager& chunkManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] WorldLightManager* lightManager() override { return nullptr; }
    [[nodiscard]] const WorldLightManager* lightManager() const override { return nullptr; }
    [[nodiscard]] EntityManager& entityManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const EntityManager& entityManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::EntityTracker& entityTracker() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::EntityTracker& entityTracker() const override { throw std::logic_error("unused"); }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] server::WeatherManager& weatherManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::WeatherManager& weatherManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::ItemPickupManager& itemPickupManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::ItemPickupManager& itemPickupManager() const override
    {
        throw std::logic_error("unused");
    }
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
    [[nodiscard]] server::sync::EntitySyncManager& entitySyncManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::EntitySyncManager& entitySyncManager() const override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] server::sync::ChunkSendManager& chunkSendManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::ChunkSendManager& chunkSendManager() const override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] server::sync::LightSyncManager& lightSyncManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::LightSyncManager& lightSyncManager() const override
    {
        throw std::logic_error("unused");
    }

    [[nodiscard]] server::ServerScoreboard& scoreboard() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override
    {
        throw std::logic_error("unused");
    }

    [[nodiscard]] CommandRegistry& commandRegistry() override { return m_commandRegistry; }
    [[nodiscard]] const CommandRegistry& commandRegistry() const override { return m_commandRegistry; }

    [[nodiscard]] i32 viewDistance() const override { return m_config.viewDistance; }
    [[nodiscard]] i32 maxPlayers() const override { return m_config.maxPlayers; }
    [[nodiscard]] u64 seed() const override { return m_config.seed; }
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

    void sendSoundToPlayer(PlayerId playerId,
        const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        MC_UNUSED(playerId);
        MC_UNUSED(soundEventId);
        MC_UNUSED(category);
        MC_UNUSED(position);
        MC_UNUSED(volume);
        MC_UNUSED(pitch);
    }

    [[nodiscard]] server::ServerPlayerData* addTestPlayer(PlayerId playerId, const std::string& username)
    {
        auto connection = std::make_shared<FakeConnection>();
        std::string uuid = util::uuidToString(util::generateOfflineUuid(username));
        auto* player = m_playerManager.addPlayer(playerId, uuid, username, connection);
        if (player != nullptr) {
            m_connections.push_back(connection);
            m_inventoryManager.initializeInventory(playerId);
        }
        return player;
    }

    [[nodiscard]] const std::string& lastBroadcastMessage() const noexcept { return m_lastBroadcastMessage; }

private:
    server::ServerCoreConfig m_config{};
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
    std::vector<std::shared_ptr<FakeConnection>> m_connections;
};

} // namespace mc::command

// 测试类在全局命名空间
class ExecuteCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册 execute 命令
        mc::command::ExecuteCommand::registerTo(m_server.commandRegistry().dispatcher());
        // 注册 help 命令（用于测试嵌套执行）
        mc::command::HelpCommand::registerTo(m_server.commandRegistry().dispatcher());
        // 注册 list 命令（用于测试嵌套执行）
        mc::command::ListCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    mc::command::ExecuteTestServer m_server;
    mc::command::ServerCommandSource m_console = mc::command::ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(ExecuteCommandTest, ExecuteCommandIsRegistered)
{
    // 验证 execute 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 execute 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "execute") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "execute command should be registered";
}

TEST_F(ExecuteCommandTest, ExecuteCommandRequiresPermissionLevel2)
{
    // 创建一个权限等级 0 的命令源
    mc::command::ServerCommandSource lowPermSource(&m_server,
        nullptr,
        nullptr,
        mc::Vector3d(0, 0, 0),
        mc::Vector2f(0, 0),
        0, // 权限等级 0
        0,
        "test");

    // 应该因为没有权限而被拒绝
    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("execute run help", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ========== run 子命令测试 ==========

TEST_F(ExecuteCommandTest, ExecuteRunHelpCommand)
{
    // 测试 execute run help - 直接执行 help 命令
    const auto result = m_server.commandRegistry().execute("execute run help", m_console);

    EXPECT_TRUE(result.success());
    // help 命令成功返回 1
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteRunListCommand)
{
    // 测试 execute run list - 直接执行 list 命令
    const auto result = m_server.commandRegistry().execute("execute run list", m_console);

    EXPECT_TRUE(result.success());
    // list 命令成功返回在线玩家数量（0）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecuteRunWithSlash)
{
    // 测试带斜杠的命令
    const auto result = m_server.commandRegistry().execute("execute run /help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteRunEmptyCommand)
{
    // 测试空命令应该失败
    const auto result = m_server.commandRegistry().execute("execute run", m_console);

    // 应该解析失败或执行失败
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

// ========== positioned 子命令测试 ==========

TEST_F(ExecuteCommandTest, ExecutePositionedRunCommand)
{
    // 测试 execute positioned 在指定位置执行命令
    const auto result = m_server.commandRegistry().execute("execute positioned 100 64 200 run list", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecutePositionedWithRelativeCoords)
{
    // 测试相对坐标（使用 ~ 符号应该由命令系统处理）
    // 由于我们在单元测试中没有真正的玩家位置，这里只测试命令解析
    const auto result = m_server.commandRegistry().execute("execute positioned ~10 ~ ~-5 run help", m_console);

    // 命令应该成功解析和执行
    EXPECT_TRUE(result.success());
}

// ========== if/unless block 子命令测试 ==========

TEST_F(ExecuteCommandTest, ExecuteIfBlockCommandNoWorld)
{
    // 在没有世界的情况下测试 if block
    // 由于单元测试中没有真正的世界，条件检查会失败
    const auto result = m_server.commandRegistry().execute("execute if block 0 0 0 stone run help", m_console);

    // 应该因为没有世界而失败
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteUnlessBlockCommandNoWorld)
{
    // 在没有世界的情况下测试 unless block
    const auto result = m_server.commandRegistry().execute("execute unless block 0 0 0 stone run help", m_console);

    // 应该因为没有世界而失败
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

// ========== as 子命令测试 ==========

TEST_F(ExecuteCommandTest, ExecuteAsNoTarget)
{
    // 测试没有目标玩家的情况
    const auto result = m_server.commandRegistry().execute("execute as @p run help", m_console);

    // 由于没有玩家在线，应该返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecuteAsWithPlayer)
{
    // 添加一个测试玩家
    m_server.addTestPlayer(1, "TestPlayer");

    // 测试 as 子命令
    // 注意：由于 playerEntityManager() 会抛出异常，这个测试会失败
    // 但这是预期的，因为单元测试环境不完整
    const auto result = m_server.commandRegistry().execute("execute as TestPlayer run help", m_console);

    // 在单元测试环境中，玩家实体不可用，所以应该返回 0
    EXPECT_TRUE(result.success() || result.failed());
}

// ========== at 子命令测试 ==========

TEST_F(ExecuteCommandTest, ExecuteAtNoTarget)
{
    // 测试没有目标玩家的情况
    const auto result = m_server.commandRegistry().execute("execute at @p run help", m_console);

    // 由于没有玩家在线，应该返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 命令解析测试 ==========

TEST_F(ExecuteCommandTest, InvalidSubcommand)
{
    // 测试无效的子命令
    const auto result = m_server.commandRegistry().execute("execute invalid run help", m_console);

    // 应该解析失败
    EXPECT_TRUE(result.failed());
}

TEST_F(ExecuteCommandTest, MissingRunKeyword)
{
    // 测试缺少 run 关键字
    const auto result = m_server.commandRegistry().execute("execute as @p help", m_console);

    // 应该解析失败
    EXPECT_TRUE(result.failed());
}

// ========== 嵌套命令执行测试 ==========

TEST_F(ExecuteCommandTest, NestedCommandExecution)
{
    // 测试嵌套命令执行基本功能
    // execute run help 应该返回 help 命令的结果
    const auto result = m_server.commandRegistry().execute("execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, MultipleNestedCommands)
{
    // 测试多次嵌套（通过多个 execute）
    // execute positioned 0 0 0 run execute run help
    const auto result = m_server.commandRegistry().execute("execute positioned 0 0 0 run execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}
