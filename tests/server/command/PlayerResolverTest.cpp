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

#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <vector>

// Forward declarations
namespace mc {
class ServerDimensionManager;
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
}

namespace mc::server {
class ServerWorld;
class ServerChunkManager;
class EntityTracker;
class ItemPickupManager;
class WeatherManager;
class ServerPlayerEntityManager;
}

namespace mc::server::sync {
class EntitySyncManager;
class ChunkSendManager;
class LightSyncManager;
}

namespace mc::server::interaction {
class BlockInteractionManager;
class MiningManager;
class ContainerManager;
}

namespace mc::command {
namespace {

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
        : m_playerManager(m_config)
        , m_inventoryManager(m_playerManager)
        , m_connectionManager(m_playerManager)
        , m_timeManager(0, 1000)
        , m_teleportManager(m_playerManager)
        , m_keepAliveManager(m_playerManager, m_config)
        , m_positionTracker(m_playerManager, m_config)
        , m_packetHandler(
            m_playerManager,
            m_connectionManager,
            m_teleportManager,
            m_keepAliveManager,
            m_positionTracker,
            m_timeManager,
            m_config)
        , m_gameModeManager(m_playerManager, m_connectionManager)
        , m_commandRegistry()
    {
    }

    // IServer 接口实现
    [[nodiscard]] Result<void> initialize() override { return Result<void>::ok(); }
    void shutdown() override { m_running = false; }
    void tick() override {}
    [[nodiscard]] bool isRunning() const override { return m_running; }

    [[nodiscard]] server::core::PlayerManager& playerManager() override { return m_playerManager; }
    [[nodiscard]] const server::core::PlayerManager& playerManager() const override { return m_playerManager; }
    [[nodiscard]] server::core::ConnectionManager& connectionManager() override { return m_connectionManager; }
    [[nodiscard]] const server::core::ConnectionManager& connectionManager() const override { return m_connectionManager; }
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

    [[nodiscard]] mc::ServerDimensionManager& dimensionManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const mc::ServerDimensionManager& dimensionManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::ServerWorld& world() override { throw std::logic_error("world not available in unit test"); }
    [[nodiscard]] const server::ServerWorld& world() const override { throw std::logic_error("world not available in unit test"); }
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
    [[nodiscard]] const server::ItemPickupManager& itemPickupManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override { throw std::logic_error("playerEntityManager not available in unit test"); }
    [[nodiscard]] const server::ServerPlayerEntityManager& playerEntityManager() const override { throw std::logic_error("playerEntityManager not available in unit test"); }
    [[nodiscard]] server::interaction::BlockInteractionManager& blockInteractionManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::BlockInteractionManager& blockInteractionManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::interaction::MiningManager& miningManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::MiningManager& miningManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::interaction::ContainerManager& containerManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::ContainerManager& containerManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::interaction::InventoryManager& inventoryManager() override { return m_inventoryManager; }
    [[nodiscard]] const server::interaction::InventoryManager& inventoryManager() const override { return m_inventoryManager; }
    [[nodiscard]] PlayerInventory* playerInventory(PlayerId playerId) override { return m_inventoryManager.getInventory(playerId); }
    [[nodiscard]] const PlayerInventory* playerInventory(PlayerId playerId) const override { return m_inventoryManager.getInventory(playerId); }
    [[nodiscard]] server::sync::EntitySyncManager& entitySyncManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::EntitySyncManager& entitySyncManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::sync::ChunkSendManager& chunkSendManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::ChunkSendManager& chunkSendManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::sync::LightSyncManager& lightSyncManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::LightSyncManager& lightSyncManager() const override { throw std::logic_error("unused"); }

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
    void requestStop() override { m_stopRequested = true; m_running = false; }

    void broadcastParticleInRange(u32, f64, f64, f64, f32, f32, f32, f32, f32, f32, u32, f32) override {}

    void sendSoundToPlayer(PlayerId,
                          const ResourceLocation&,
                          sound::SoundCategory,
                          const Vector3&,
                          f32,
                          f32) override {}

    /**
     * @brief 添加测试玩家（仅 PlayerManager 条目，无实体）。
     */
    [[nodiscard]] server::ServerPlayerData* addTestPlayer(PlayerId playerId, const std::string& username)
    {
        auto connection = std::make_shared<FakeConnection>();
        auto* player = m_playerManager.addPlayer(playerId, username, connection);
        if (player != nullptr) {
            m_connections.push_back(connection);
            m_inventoryManager.initializeInventory(playerId);
        }
        return player;
    }

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
    CommandRegistry m_commandRegistry;
    std::vector<std::shared_ptr<FakeConnection>> m_connections;
};

} // namespace
} // namespace mc::command

// 全局命名空间中引入需要的类型
using mc::command::IntRange;
using mc::command::FloatRange;
using mc::command::EntitySelector;
using mc::command::EntitySelectorType;
using mc::command::EntitySelectorSort;
using mc::command::ServerCommandSource;
using mc::command::CommandRegistry;
using mc::command::support::resolveSinglePlayerId;
using mc::command::support::resolvePlayerIds;
using mc::command::support::getGameModeCommandName;
using mc::command::support::getDifficultyCommandName;
using mc::PlayerId;
using mc::GameMode;
using mc::Difficulty;
using mc::Vector3d;
using mc::Vector2f;
using mc::Vector3;
using mc::ResourceLocation;
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
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

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

    ServerCommandSource source(
        &m_server, nullptr, nullptr,
        Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Alice");

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
    EXPECT_EQ(result, 2);  // Bob 最近
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
    EXPECT_EQ(result[0], 3);  // Charlie 最远
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
    EXPECT_EQ(result.size(), 1);  // 只有 Bob 在范围内
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
    selector.setGameMode("1");  // Creative = 1

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
    selector.setGameMode("creative", true);  // Negated: 不是创造模式

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 2);  // Alice 和 Charlie
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
