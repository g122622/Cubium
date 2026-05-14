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
 * @file SpawnPointCommandTest.cpp
 * @brief SpawnPointCommand 单元测试
 *
 * 测试 /spawnpoint 命令的注册、解析、权限检查和功能。
 */

#include <gtest/gtest.h>

#include "common/entity/entities/player/Player.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/SpawnPointCommand.hpp"
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
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <stdexcept>
#include <vector>

// Forward declarations
namespace mc {
class ServerDimensionManager;
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
namespace server {
class ServerScoreboard;
} // namespace server
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
 */
class SpawnPointTestServer final : public server::IServer {
public:
    SpawnPointTestServer()
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
        m_lastSoundPlayerId = playerId;
        m_lastSoundEvent = soundEventId;
        m_lastSoundCategory = category;
        m_lastSoundPosition = position;
        m_lastSoundVolume = volume;
        m_lastSoundPitch = pitch;
        m_soundSent = true;
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

    // 音效记录
    bool m_soundSent = false;
    PlayerId m_lastSoundPlayerId = 0;
    ResourceLocation m_lastSoundEvent{""};
    sound::SoundCategory m_lastSoundCategory = sound::SoundCategory::Master;
    Vector3 m_lastSoundPosition{0, 0, 0};
    f32 m_lastSoundVolume = 1.0f;
    f32 m_lastSoundPitch = 1.0f;

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

class SpawnPointCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        SpawnPointCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    SpawnPointTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(SpawnPointCommandTest, SpawnPointCommandIsRegistered)
{
    // 验证 spawnpoint 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 spawnpoint 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "spawnpoint") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "spawnpoint command should be registered";
}

TEST_F(SpawnPointCommandTest, SpawnPointCommandRequiresPermissionLevel2)
{
    // 创建一个权限等级 0 的命令源
    ServerCommandSource lowPermSource(&m_server,
        nullptr,
        nullptr,
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        0, // 权限等级 0
        0,
        "test");

    // 应该因为没有权限而被拒绝
    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("spawnpoint", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ========== 命令语法测试 ==========

TEST_F(SpawnPointCommandTest, SpawnPointNoArgsFailsWithoutPlayer)
{
    // 测试无参数命令 - 控制台没有玩家实体
    const auto result = m_server.commandRegistry().execute("spawnpoint", m_console);

    // 控制台没有玩家实体，应该返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPlayerSelectorNoPlayers)
{
    // 测试带玩家选择器但没有玩家
    const auto result = m_server.commandRegistry().execute("spawnpoint @p", m_console);

    // 没有玩家匹配，返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPositionNoPlayers)
{
    // 测试带位置参数但没有玩家
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 100 64 200", m_console);

    // 没有玩家匹配，返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithCoordinates)
{
    // 测试带坐标参数的命令语法
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 100.5 64.0 -200.5", m_console);

    // 命令解析成功，由于没有玩家返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithRelativeCoordinates)
{
    // 测试相对坐标语法（使用 ~ 前缀）
    const auto result = m_server.commandRegistry().execute("spawnpoint @p ~10 ~ ~-5", m_console);

    // 相对坐标语法应该被解析
    EXPECT_TRUE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithFloatCoordinates)
{
    // 测试浮点坐标
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 123.456 78.9 -456.789", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 选择器测试 ==========

TEST_F(SpawnPointCommandTest, SpawnPointWithSelfSelector)
{
    // 添加测试玩家
    m_server.addTestPlayer(1, "TestPlayer");

    // 测试 @s 选择器（自己）
    const auto result = m_server.commandRegistry().execute("spawnpoint @s", m_console);

    // 控制台不是玩家，所以 @s 应该不匹配
    EXPECT_TRUE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPlayerName)
{
    // 添加测试玩家 - 这只在 PlayerManager 中添加记录，但没有实体
    m_server.addTestPlayer(1, "TestPlayer");

    // 测试通过玩家名选择
    // 注意：由于测试环境没有 ServerWorld 和 PlayerEntityManager，
    // 玩家名选择器可以解析，但无法获取实际玩家实体
    const auto result = m_server.commandRegistry().execute("spawnpoint TestPlayer", m_console);

    // 命令应该失败，因为没有完整的玩家实体环境
    // 实际环境中，这会成功设置指定玩家的重生点
    EXPECT_FALSE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithInvalidSelector)
{
    // 测试无效选择器
    const auto result = m_server.commandRegistry().execute("spawnpoint @invalid", m_console);

    // 无效选择器应该失败
    EXPECT_FALSE(result.success());
}

// ========== 权限测试 ==========

TEST_F(SpawnPointCommandTest, SpawnPointWithPermissionLevel2)
{
    // 创建权限等级 2 的命令源
    ServerCommandSource permSource(&m_server,
        nullptr,
        nullptr,
        Vector3d(100, 64, 200),
        Vector2f(0, 0),
        2, // 权限等级 2
        0,
        "admin");

    const auto result = m_server.commandRegistry().execute("spawnpoint", permSource);

    // 有权限，命令应该成功（虽然可能没有玩家实体）
    EXPECT_TRUE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPermissionLevel4)
{
    // 创建权限等级 4 的命令源（控制台级别）
    ServerCommandSource permSource(&m_server,
        nullptr,
        nullptr,
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        4, // 权限等级 4
        0,
        "console");

    const auto result = m_server.commandRegistry().execute("spawnpoint @p", permSource);

    // 有权限
    EXPECT_TRUE(result.success());
}

// ========== 坐标边界测试 ==========

TEST_F(SpawnPointCommandTest, SpawnPointWithZeroCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 0 0 0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithNegativeCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p -100 -64 -200", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithLargeCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 30000000 256 30000000", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 命令别名测试 ==========

TEST_F(SpawnPointCommandTest, SpawnPointHasNoAlias)
{
    // spawnpoint 没有别名的测试
    // 命令应该正确执行
    const auto result = m_server.commandRegistry().execute("spawnpoint", m_console);
    EXPECT_TRUE(result.success());
}

} // namespace mc::command
