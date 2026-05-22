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
 * @file PlayerResolverTest.cpp
 * @brief PlayerResolver 单元测试
 *
 * 测试玩家选择器的解析和过滤功能，特别是经验等级过滤。
 *
 * 注意：等级过滤测试需要完整的 ServerWorld 和 Player 实体，
 * 这些测试应在集成测试环境中进行。此文件主要测试：
 * 1. IntRange 等级范围逻辑
 * 2. 不需要实体的基本选择器解析
 * 3. 游戏模式过滤逻辑
 */

#include <gtest/gtest.h>

#include "common/command/arguments/EntityArgument.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/PlayerResolver.hpp"
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
#include "server/scoreboard/ServerScoreboard.hpp"

#include <memory>
#include <vector>

// Forward declarations
namespace mc {
class ServerDimensionManager;
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
} // namespace mc

namespace mc::server {
class ServerWorld;
class ServerChunkManager;
class EntityTracker;
class ItemPickupManager;
class WeatherManager;
class ServerPlayerEntityManager;
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
 * @brief 测试用假连接。
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

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

/**
 * @brief 玩家解析测试服务器。
 *
 * 提供最小化的服务器接口实现，不提供 world() 和 playerEntityManager()，
 * 因为这些测试专注于不需要玩家实体的选择器逻辑。
 */
class PlayerResolverTestServer final : public mc::server::IServer {
public:
    PlayerResolverTestServer()
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

    [[nodiscard]] mc::ServerDimensionManager& dimensionManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const mc::ServerDimensionManager& dimensionManager() const override
    {
        throw std::logic_error("unused");
    }
    [[nodiscard]] server::ServerWorld* getPlayerWorld(PlayerId) override { return nullptr; }
    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override
    {
        throw std::logic_error("playerEntityManager not available in unit test");
    }
    [[nodiscard]] const server::ServerPlayerEntityManager& playerEntityManager() const override
    {
        throw std::logic_error("playerEntityManager not available in unit test");
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

    /**
     * @brief 添加测试玩家（仅 PlayerManager 条目，无实体）。
     */
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
    std::vector<std::shared_ptr<FakeConnection>> m_connections;
};

} // namespace mc::command

// 全局命名空间中引入需要的类型
using mc::Difficulty;
using mc::GameMode;
using mc::PlayerId;
using mc::ResourceLocation;
using mc::Vector2f;
using mc::Vector3;
using mc::Vector3d;
using mc::command::CommandRegistry;
using mc::command::EntitySelector;
using mc::command::EntitySelectorSort;
using mc::command::EntitySelectorType;
using mc::command::FloatRange;
using mc::command::IntRange;
using mc::command::ServerCommandSource;
using mc::command::support::getDifficultyCommandName;
using mc::command::support::getGameModeCommandName;
using mc::command::support::resolvePlayerIds;
using mc::command::support::resolveSinglePlayerId;
using mc::sound::SoundCategory;

// ========== IntRange 测试（等级过滤核心逻辑）==========

/**
 * @brief IntRange 等级范围测试
 *
 * 这些测试验证 EntitySelector 中的 level 参数过滤逻辑。
 * IntRange 用于 @a[level=..] 等选择器参数。
 */
class IntRangeTest : public ::testing::Test {
protected:
    mc::command::IntRange range;
};

TEST_F(IntRangeTest, UnboundedRangeAcceptsAnyValue)
{
    EXPECT_TRUE(range.isUnbounded());
    EXPECT_TRUE(range.test(0));
    EXPECT_TRUE(range.test(100));
    EXPECT_TRUE(range.test(-50));
}

TEST_F(IntRangeTest, MinBoundRejectsLowerValues)
{
    range.setMin(10);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_FALSE(range.test(5));
    EXPECT_FALSE(range.test(9));
    EXPECT_TRUE(range.test(10));
    EXPECT_TRUE(range.test(100));
}

TEST_F(IntRangeTest, MaxBoundRejectsHigherValues)
{
    range.setMax(20);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_TRUE(range.test(0));
    EXPECT_TRUE(range.test(20));
    EXPECT_FALSE(range.test(21));
    EXPECT_FALSE(range.test(100));
}

TEST_F(IntRangeTest, BoundedRangeOnlyAcceptsInRange)
{
    range.setMin(10);
    range.setMax(20);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_FALSE(range.test(5));
    EXPECT_FALSE(range.test(9));
    EXPECT_TRUE(range.test(10));
    EXPECT_TRUE(range.test(15));
    EXPECT_TRUE(range.test(20));
    EXPECT_FALSE(range.test(21));
    EXPECT_FALSE(range.test(100));
}

TEST_F(IntRangeTest, ExactValueRange)
{
    range.setMin(15);
    range.setMax(15);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_FALSE(range.test(14));
    EXPECT_TRUE(range.test(15));
    EXPECT_FALSE(range.test(16));
}

TEST_F(IntRangeTest, ZeroLevelHandling)
{
    // 等级 0 是有效等级
    range.setMin(0);
    range.setMax(5);
    EXPECT_TRUE(range.test(0));
    EXPECT_TRUE(range.test(3));
    EXPECT_TRUE(range.test(5));
    EXPECT_FALSE(range.test(6));
}

TEST_F(IntRangeTest, HighLevelHandling)
{
    // 高等级玩家
    range.setMin(100);
    range.setMax(200);
    EXPECT_FALSE(range.test(99));
    EXPECT_TRUE(range.test(100));
    EXPECT_TRUE(range.test(150));
    EXPECT_TRUE(range.test(200));
    EXPECT_FALSE(range.test(201));
}

// ========== PlayerResolver 基础测试 ==========

class PlayerResolverTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    mc::command::PlayerResolverTestServer m_server;
};

TEST_F(PlayerResolverTest, ResolveSinglePlayerWithNoPlayersReturnsZero)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::SinglePlayer);

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 0);
}

TEST_F(PlayerResolverTest, ResolveAllPlayersWithNoPlayersReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(PlayerResolverTest, ResolveAllPlayersReturnsAll)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_EQ(result.size(), 3);
}

TEST_F(PlayerResolverTest, ResolveByUsername)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::byUsername("Bob");

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 2);
}

TEST_F(PlayerResolverTest, ResolveByUsernameNotFound)
{
    m_server.addTestPlayer(1, "Alice");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::byUsername("UnknownPlayer");

    auto result = resolvePlayerIds(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(PlayerResolverTest, ResolveByUsernameNegated)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setUsernameNegated("Bob");

    auto result = resolvePlayerIds(source, selector);
    EXPECT_EQ(result.size(), 2);
    // 结果不包含 Bob
    EXPECT_TRUE(std::find(result.begin(), result.end(), 2) == result.end());
}

TEST_F(PlayerResolverTest, ResolveSelfReturnsOwnPlayerId)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Alice");

    EntitySelector selector = EntitySelector::self();

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 1);
}

TEST_F(PlayerResolverTest, ResolveNearestPlayer)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    // 设置不同位置
    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->x = 100.0f;
    data2->x = 10.0f;
    data3->x = 200.0f;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::SinglePlayer);
    selector.setSort(EntitySelectorSort::Nearest);

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 2); // Bob 最近
}

TEST_F(PlayerResolverTest, ResolveFurthestPlayer)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    // 设置不同位置
    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->x = 100.0f;
    data2->x = 10.0f;
    data3->x = 200.0f;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setSort(EntitySelectorSort::Furthest);
    selector.setLimit(1);

    auto result = resolvePlayerIds(source, selector);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 3); // Charlie 最远
}

TEST_F(PlayerResolverTest, ResolveWithDistanceFilter)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    // 设置不同位置
    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->x = 5.0f;
    data2->x = 15.0f;
    data3->x = 50.0f;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.distance().setMin(10);
    selector.distance().setMax(30);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_EQ(result.size(), 1); // 只有 Bob 在范围内
    EXPECT_EQ(result[0], 2);
}

TEST_F(PlayerResolverTest, ResolveWithLimit)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");
    m_server.addTestPlayer(4, "Dave");
    m_server.addTestPlayer(5, "Eve");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setLimit(3);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_EQ(result.size(), 3);
}

// ========== 游戏模式过滤测试 ==========

TEST_F(PlayerResolverTest, GameModeFilterSurvival)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("survival");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 1);
}

TEST_F(PlayerResolverTest, GameModeFilterCreative)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;
    data3->gameMode = GameMode::Adventure;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("creative");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 2);
}

TEST_F(PlayerResolverTest, GameModeFilterByNumber)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("1"); // Creative = 1

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 2);
}

TEST_F(PlayerResolverTest, GameModeFilterNegated)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;
    data3->gameMode = GameMode::Survival;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("creative", true); // Negated: 不是创造模式

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 2); // Alice 和 Charlie
}

TEST_F(PlayerResolverTest, GameModeFilterAdventure)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Adventure;
    data2->gameMode = GameMode::Spectator;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("adventure");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 1);
}

TEST_F(PlayerResolverTest, GameModeFilterSpectator)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Spectator;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("spectator");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 2);
}

// ========== 工具函数测试 ==========

TEST(PlayerResolverUtilTest, GetGameModeCommandName)
{
    using namespace mc::command::support;

    EXPECT_STREQ(getGameModeCommandName(GameMode::Survival), "survival");
    EXPECT_STREQ(getGameModeCommandName(GameMode::Creative), "creative");
    EXPECT_STREQ(getGameModeCommandName(GameMode::Adventure), "adventure");
    EXPECT_STREQ(getGameModeCommandName(GameMode::Spectator), "spectator");
    EXPECT_STREQ(getGameModeCommandName(GameMode::NotSet), "not_set");
}

TEST(PlayerResolverUtilTest, GetDifficultyCommandName)
{
    using namespace mc::command::support;

    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Peaceful), "peaceful");
    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Easy), "easy");
    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Normal), "normal");
    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Hard), "hard");
}

// ========== FloatRange 角度测试（x_rotation/y_rotation 过滤核心逻辑）==========

/**
 * @brief FloatRange 角度范围测试
 *
 * 这些测试验证 EntitySelector 中的 x_rotation 和 y_rotation 参数过滤逻辑。
 * 角度范围需要特殊处理，因为角度在 -180 到 180 度之间环绕。
 *
 * 参考 MC 1.16.5 EntitySelector.createRotationPredicate 实现逻辑：
 * - 如果 min > max，说明范围跨越了 -180/180 边界，需要使用 OR 逻辑
 * - 例如 [170..-170] 表示从 170 度到 -170 度（跨越正北方向）
 */
class FloatRangeAngleTest : public ::testing::Test {
protected:
    mc::command::FloatRange range;
};

TEST_F(FloatRangeAngleTest, UnboundedRangeAcceptsAnyAngle)
{
    EXPECT_TRUE(range.isUnbounded());
    EXPECT_TRUE(range.testAngle(0.0f));
    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(-90.0f));
    EXPECT_TRUE(range.testAngle(180.0f));
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(270.0f));  // 会被规范化为 -90
    EXPECT_TRUE(range.testAngle(-270.0f)); // 会被规范化为 90
}

TEST_F(FloatRangeAngleTest, NormalRangeNoWraparound)
{
    // 普通范围：[10, 30]
    range.setMin(10.0f);
    range.setMax(30.0f);

    EXPECT_TRUE(range.testAngle(10.0f));
    EXPECT_TRUE(range.testAngle(20.0f));
    EXPECT_TRUE(range.testAngle(30.0f));
    EXPECT_FALSE(range.testAngle(9.0f));
    EXPECT_FALSE(range.testAngle(31.0f));
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(-10.0f));
}

TEST_F(FloatRangeAngleTest, WraparoundRange)
{
    // 跨越边界的范围：[170, -170]（接近正北方向）
    // 这表示角度在 [170, 180) 或 [-180, -170] 范围内
    range.setMin(170.0f);
    range.setMax(-170.0f);

    // 在范围内（接近正北）
    EXPECT_TRUE(range.testAngle(175.0f));
    EXPECT_TRUE(range.testAngle(180.0f)); // 180 会被规范化为 -180
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(-175.0f));
    EXPECT_TRUE(range.testAngle(-170.0f));
    EXPECT_TRUE(range.testAngle(170.0f));

    // 不在范围内（远离正北）
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(90.0f));
    EXPECT_FALSE(range.testAngle(-90.0f));
    EXPECT_FALSE(range.testAngle(169.0f));
    EXPECT_FALSE(range.testAngle(-169.0f));
}

TEST_F(FloatRangeAngleTest, WraparoundRangeLarge)
{
    // 大范围跨越：[90, -90]（覆盖整个后半球）
    range.setMin(90.0f);
    range.setMax(-90.0f);

    // 在范围内（后半球：东 -> 南 -> 西 -> 北 -> 东）
    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(180.0f));
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(-90.0f));
    EXPECT_TRUE(range.testAngle(120.0f));
    EXPECT_TRUE(range.testAngle(-120.0f));

    // 不在范围内（前半球）
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(45.0f));
    EXPECT_FALSE(range.testAngle(-45.0f));
    EXPECT_FALSE(range.testAngle(89.0f));
    EXPECT_FALSE(range.testAngle(-89.0f));
}

TEST_F(FloatRangeAngleTest, PitchRangeNegative90To90)
{
    // 俯仰角范围（x_rotation）：[-45, 45]
    // 表示从向下看 45 度到向上看 45 度
    range.setMin(-45.0f);
    range.setMax(45.0f);

    EXPECT_TRUE(range.testAngle(0.0f));   // 正视前方
    EXPECT_TRUE(range.testAngle(30.0f));  // 向上看 30 度
    EXPECT_TRUE(range.testAngle(-30.0f)); // 向下看 30 度
    EXPECT_TRUE(range.testAngle(45.0f));  // 边界
    EXPECT_TRUE(range.testAngle(-45.0f)); // 边界

    EXPECT_FALSE(range.testAngle(60.0f));  // 太高
    EXPECT_FALSE(range.testAngle(-60.0f)); // 太低
    EXPECT_FALSE(range.testAngle(90.0f));  // 直视上方
    EXPECT_FALSE(range.testAngle(-90.0f)); // 直视下方
}

TEST_F(FloatRangeAngleTest, OnlyMinBound)
{
    // 只有最小值：[45, ...]
    // MC 1.16.5 行为：min=45, max 默认为 null
    // testAngle() 中，max 为 null 时使用默认值 359（规范后为 -1）
    // 范围 [45, -1]，min > max，所以是跨越边界的范围
    // 跨越边界匹配：value >= 45 || value <= -1
    // 范围 = [45, 180) ∪ [-180, -1]
    range.setMin(45.0f);

    // 在跨越边界范围内 [45, 180) ∪ [-180, -1]
    EXPECT_TRUE(range.testAngle(45.0f));
    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(180.0f));  // 规范化为 -180，在 [-180, -1] 范围内
    EXPECT_TRUE(range.testAngle(-180.0f)); // 在范围内
    EXPECT_TRUE(range.testAngle(-1.0f));   // 在范围内边界
    EXPECT_TRUE(range.testAngle(170.0f));
    EXPECT_TRUE(range.testAngle(-170.0f)); // 在 [-180, -1] 范围内
    EXPECT_TRUE(range.testAngle(-2.0f));   // 在 [-180, -1] 范围内
    EXPECT_TRUE(range.testAngle(-90.0f));  // 在 [-180, -1] 范围内
    EXPECT_TRUE(range.testAngle(-179.0f)); // 在范围内

    // 不在范围内：[-0.999, 44.999]
    // 这个范围是 "缺口"，即不包含的角度
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(-0.5f));
    EXPECT_FALSE(range.testAngle(44.0f));
    EXPECT_FALSE(range.testAngle(1.0f));
    EXPECT_FALSE(range.testAngle(44.999f));
}

TEST_F(FloatRangeAngleTest, OnlyMaxBound)
{
    // 只有最大值：[..., 90]
    range.setMax(90.0f);

    // MC 行为：min 默认为 0，max=90
    // 范围是 [0, 90]
    EXPECT_TRUE(range.testAngle(0.0f));
    EXPECT_TRUE(range.testAngle(45.0f));
    EXPECT_TRUE(range.testAngle(90.0f));

    EXPECT_FALSE(range.testAngle(-1.0f));
    EXPECT_FALSE(range.testAngle(91.0f));
    EXPECT_FALSE(range.testAngle(-45.0f));
    EXPECT_FALSE(range.testAngle(180.0f));
}

TEST_F(FloatRangeAngleTest, AngleNormalization)
{
    // 测试角度规范化
    range.setMin(0.0f);
    range.setMax(90.0f);

    // 270 度会被规范化为 -90 度
    EXPECT_FALSE(range.testAngle(270.0f));
    // -270 度会被规范化为 90 度
    EXPECT_TRUE(range.testAngle(-270.0f));
    // 360 度会被规范化为 0 度
    EXPECT_TRUE(range.testAngle(360.0f));
    // -360 度会被规范化为 0 度
    EXPECT_TRUE(range.testAngle(-360.0f));
    // 450 度会被规范化为 90 度
    EXPECT_TRUE(range.testAngle(450.0f));
}

TEST_F(FloatRangeAngleTest, ExactAngleMatch)
{
    // 精确角度匹配：[45, 45]
    range.setMin(45.0f);
    range.setMax(45.0f);

    EXPECT_TRUE(range.testAngle(45.0f));
    EXPECT_FALSE(range.testAngle(44.9f));
    EXPECT_FALSE(range.testAngle(45.1f));
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(-45.0f));
}

TEST_F(FloatRangeAngleTest, FullCircleRange)
{
    // 注意：[-180, 180] 的行为有些特殊
    // 因为 wrapDegrees(180) = -180，所以这个范围实际上变成了 [-180, -180]
    // 这是一个精确匹配范围

    // 如果想要匹配所有角度，应该使用无界范围（不设置 min/max）
    // 或者使用跨越边界的范围如 [-180, 179.999] 或 [-179, 180]

    // 范围 [-180, 180] 变成精确匹配 -180
    range.setMin(-180.0f);
    range.setMax(180.0f);

    // 只有 -180（或规范化为 -180 的值，如 180）匹配
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(180.0f)); // 180 规范化为 -180
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(540.0f)); // 540 = 180 + 360，规范化为 -180

    // 其他值不匹配（因为 180 规范化后 min == max == -180）
    EXPECT_FALSE(range.testAngle(-90.0f));
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(90.0f));
}

TEST_F(FloatRangeAngleTest, FullCircleRangeWraparound)
{
    // 如果要匹配几乎整个圆，可以使用跨越边界的范围
    // 例如：[-179, 179] 表示除了正北方向（±180）以外的所有方向
    range.setMin(-179.0f);
    range.setMax(179.0f);

    // min < max，所以使用 AND 逻辑
    // 范围：[-179, 179]
    EXPECT_TRUE(range.testAngle(-179.0f));
    EXPECT_TRUE(range.testAngle(-90.0f));
    EXPECT_TRUE(range.testAngle(0.0f));
    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(179.0f));

    // 不在范围内
    EXPECT_FALSE(range.testAngle(180.0f)); // 规范化为 -180
    EXPECT_FALSE(range.testAngle(-180.0f));
}

// ========== EntitySelector 角度范围解析测试 ==========

class EntitySelectorAngleTest : public ::testing::Test {
protected:
    mc::command::EntitySelector selector;
};

TEST_F(EntitySelectorAngleTest, DefaultRotationRangesAreUnbounded)
{
    EXPECT_TRUE(selector.xRotation().isUnbounded());
    EXPECT_TRUE(selector.yRotation().isUnbounded());
}

TEST_F(EntitySelectorAngleTest, SetXRotation)
{
    selector.xRotation().setMin(-45.0f);
    selector.xRotation().setMax(45.0f);

    EXPECT_FALSE(selector.xRotation().isUnbounded());
    EXPECT_TRUE(selector.xRotation().hasMin());
    EXPECT_TRUE(selector.xRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.xRotation().getMin(), -45.0f);
    EXPECT_FLOAT_EQ(selector.xRotation().getMax(), 45.0f);
}

TEST_F(EntitySelectorAngleTest, SetYRotation)
{
    selector.yRotation().setMin(170.0f);
    selector.yRotation().setMax(-170.0f);

    EXPECT_FALSE(selector.yRotation().isUnbounded());
    EXPECT_TRUE(selector.yRotation().hasMin());
    EXPECT_TRUE(selector.yRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.yRotation().getMin(), 170.0f);
    EXPECT_FLOAT_EQ(selector.yRotation().getMax(), -170.0f);
}

TEST_F(EntitySelectorAngleTest, XRotationFilterMatchesPitch)
{
    // 设置俯仰角范围：-30 到 30 度
    selector.xRotation().setMin(-30.0f);
    selector.xRotation().setMax(30.0f);

    // 测试实体俯仰角
    EXPECT_TRUE(selector.xRotation().testAngle(0.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(15.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(-15.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(30.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(-30.0f));

    EXPECT_FALSE(selector.xRotation().testAngle(45.0f));
    EXPECT_FALSE(selector.xRotation().testAngle(-45.0f));
    EXPECT_FALSE(selector.xRotation().testAngle(90.0f));
    EXPECT_FALSE(selector.xRotation().testAngle(-90.0f));
}

TEST_F(EntitySelectorAngleTest, YRotationFilterHandlesWraparound)
{
    // 设置偏航角范围：170 到 -170（接近正北）
    selector.yRotation().setMin(170.0f);
    selector.yRotation().setMax(-170.0f);

    // 测试实体偏航角（跨越边界）
    EXPECT_TRUE(selector.yRotation().testAngle(175.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(180.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(-180.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(-175.0f));

    EXPECT_FALSE(selector.yRotation().testAngle(0.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(90.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(-90.0f));
}
