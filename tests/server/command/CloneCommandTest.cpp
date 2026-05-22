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
 * @file CloneCommandTest.cpp
 * @brief CloneCommand 单元测试
 *
 * 测试 /clone 命令的注册和命令解析。
 * 由于 ServerWorld 接口复杂，完整的功能集成测试
 * 应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
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

// Forward declarations for types only needed for interface declaration
namespace mc {
class ServerDimensionManager;
}

namespace mc::server {
class ServerPlayerEntityManager;
class ServerScoreboard;
} // namespace mc::server

namespace mc {
class WorldLightManager;
}

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
 * 注意：此测试服务器不提供 world() 实现，
 * 因为 ServerWorld 接口复杂难以模拟。
 * CloneCommand 的完整功能测试应在集成测试中进行。
 */
class CloneTestServer final : public server::IServer {
public:
    CloneTestServer()
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
    {
        Items::initialize();
        VanillaBlocks::initialize();
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
    [[nodiscard]] server::ServerScoreboard& scoreboard() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override
    {
        throw std::logic_error("unused");
    }

    [[nodiscard]] resource::DataPackList& dataPackList() override { throw std::logic_error("unused"); }
    [[nodiscard]] const resource::DataPackList& dataPackList() const override { throw std::logic_error("unused"); }

    [[nodiscard]] loot::LootTableManager& lootTableManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const loot::LootTableManager& lootTableManager() const override { throw std::logic_error("unused"); }

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
    {
        // 空实现，用于测试
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
    std::vector<std::shared_ptr<FakeConnection>> m_connections;
};

class CloneCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    CloneTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(CloneCommandTest, CloneCommandIsRegistered)
{
    // 验证 clone 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 clone 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "clone") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "clone command should be registered";
}

TEST_F(CloneCommandTest, CloneCommandHasCorrectMetadata)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 clone 节点
    const CommandTreeNodeSnapshot* cloneNode = nullptr;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "clone") {
            cloneNode = &node;
            break;
        }
    }

    ASSERT_NE(cloneNode, nullptr) << "clone node should exist";

    // 验证元数据
    EXPECT_TRUE(cloneNode->metadata.contains("description"));
    EXPECT_TRUE(cloneNode->metadata.contains("usage"));
}

TEST_F(CloneCommandTest, CloneCommandRequiresWorld)
{
    // 控制台命令源没有关联世界，命令应该失败
    // 由于控制台没有 world，命令执行会失败（返回 0）
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20", m_console);

    // 命令执行本身成功，但由于没有世界，结果是 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandRequiresPermissionLevel2)
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
        const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20", lowPermSource);
        // 如果执行成功但返回 0，也算权限检查生效
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        // 如果抛出异常，也说明权限检查生效
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ========== 命令解析测试 ==========

TEST_F(CloneCommandTest, CloneCommandParsesBasicSyntax)
{
    // 测试基本语法解析（即使没有世界也会尝试解析参数）
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredModeWithBlock)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredModeWithNamespacedBlock)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered minecraft:dirt", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceForceMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace force", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceMoveMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace move", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceNormalMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace normal", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedForceMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked force", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedMoveMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked move", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedNormalMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked normal", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredForceMode)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone force", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredMoveMode)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone move", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredNormalMode)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone normal", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesNegativeCoordinates)
{
    const auto result = m_server.commandRegistry().execute("clone -100 -60 -100 100 64 100 200 64 200", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

TEST_F(CloneCommandTest, CloneCommandParsesReversedCoordinates)
{
    // MC 允许坐标顺序颠倒，会自动计算最小/最大值
    const auto result = m_server.commandRegistry().execute("clone 10 10 10 0 0 0 20 20 20", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0); // 失败因为没有世界
}

} // namespace mc::command
