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

#include <gtest/gtest.h>

#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
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
#include "server/core/ServerCoreConfig.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"

#include <stdexcept>
#include <vector>

// Forward declaration for ServerPlayerEntityManager (only needed for interface declaration)
namespace mc::server {
class ServerPlayerEntityManager;
class ServerScoreboard;
}

namespace mc::command {
namespace {

/**
 * @brief 命令测试使用的假连接。
 *
 * 该连接只负责满足 `PlayerManager`、`ConnectionManager` 与 `TeleportManager`
 * 对连接接口的最小要求，并记录发送行为，避免测试依赖真实网络层。
 */
class FakeConnection final : public network::IServerConnection {
public:
    /**
     * @brief 记录一次发送。
     *
     * @param data 数据指针。
     * @param size 数据长度。
     */
    void send(const u8* data, size_t size) override
    {
        if (data != nullptr && size > 0) {
            m_sentData.insert(m_sentData.end(), data, data + size);
        }
    }

    /**
     * @brief 断开连接。
     *
     * @param reason 断开原因。
     */
    void disconnect(const std::string& reason = "") override
    {
        m_disconnectReason = reason;
        m_connected = false;
    }

    /**
     * @brief 查询连接是否仍有效。
     *
     * @return `true` 表示仍连接。
     */
    [[nodiscard]] bool isConnected() const override { return m_connected; }

    /**
     * @brief 获取连接标识字符串。
     */
    [[nodiscard]] std::string identifier() const override { return "FakeConnection"; }

    /**
     * @brief 获取连接类型。
     */
    [[nodiscard]] network::ConnectionType type() const override { return network::ConnectionType::Local; }

    /**
     * @brief 获取远程地址（IP 地址）。
     */
    [[nodiscard]] std::string getAddress() const override { return ""; }

    /**
     * @brief 获取最近一次断开原因。
     */
    [[nodiscard]] const std::string& disconnectReason() const noexcept { return m_disconnectReason; }

    [[nodiscard]] size_t sentBytes() const noexcept { return m_sentData.size(); }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

class FakeServer final : public server::IServer {
public:
    FakeServer()
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
        , m_scoreboard(*this)
    {
        Items::initialize();
    }

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
    [[nodiscard]] server::ServerWorld& world() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::ServerWorld& world() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::ServerChunkManager& chunkManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::ServerChunkManager& chunkManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] WorldLightManager* lightManager() override { return nullptr; }
    [[nodiscard]] const WorldLightManager* lightManager() const override { return nullptr; }
    [[nodiscard]] mc::EntityManager& entityManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const mc::EntityManager& entityManager() const override { throw std::logic_error("unused"); }
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
    [[nodiscard]] mc::PlayerInventory* playerInventory(PlayerId playerId) override
    {
        return m_inventoryManager.getInventory(playerId);
    }
    [[nodiscard]] const mc::PlayerInventory* playerInventory(PlayerId playerId) const override
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

    [[nodiscard]] mc::command::CommandRegistry& commandRegistry() override { return m_commandRegistry; }
    [[nodiscard]] const mc::command::CommandRegistry& commandRegistry() const override { return m_commandRegistry; }

    [[nodiscard]] server::ServerScoreboard& scoreboard() override { return m_scoreboard; }
    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override { return m_scoreboard; }
    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override { throw std::logic_error("unused"); }

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

    void broadcastParticleInRange(u32 type,
        f64 x,
        f64 y,
        f64 z,
        f32 velocityX,
        f32 velocityY,
        f32 velocityZ,
        f32 offsetX,
        f32 offsetY,
        f32 offsetZ,
        u32 count,
        f32 range) override
    {
        m_lastParticleType = type;
        m_lastParticleX = x;
        m_lastParticleY = y;
        m_lastParticleZ = z;
        m_lastParticleCount = count;
        m_particleBroadcastCalled = true;
        (void)velocityX;
        (void)velocityY;
        (void)velocityZ;
        (void)offsetX;
        (void)offsetY;
        (void)offsetZ;
        (void)range;
    }

    void sendSoundToPlayer(PlayerId, const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 空实现，用于测试
    }

    [[nodiscard]] bool particleBroadcastCalled() const noexcept { return m_particleBroadcastCalled; }
    [[nodiscard]] u32 lastParticleType() const noexcept { return m_lastParticleType; }
    [[nodiscard]] f64 lastParticleX() const noexcept { return m_lastParticleX; }
    [[nodiscard]] f64 lastParticleY() const noexcept { return m_lastParticleY; }
    [[nodiscard]] f64 lastParticleZ() const noexcept { return m_lastParticleZ; }
    [[nodiscard]] u32 lastParticleCount() const noexcept { return m_lastParticleCount; }

    /**
     * @brief 向测试服务器添加一个在线玩家。
     *
     * @param playerId 玩家 ID。
     * @param username 玩家名。
     * @return 新增玩家数据指针。
     *
     * @warning 测试用例应保证 `playerId` 唯一，否则 `PlayerManager` 会拒绝插入。
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

    /**
     * @brief 查询最近插入的测试连接。
     *
     * @return 最近一个测试连接；若不存在则返回空指针。
     */
    [[nodiscard]] std::shared_ptr<FakeConnection> lastConnection() const
    {
        return m_connections.empty() ? nullptr : m_connections.back();
    }

    [[nodiscard]] const std::string& lastBroadcastMessage() const noexcept { return m_lastBroadcastMessage; }
    [[nodiscard]] bool stopRequested() const noexcept { return m_stopRequested; }

private:
    server::ServerCoreConfig m_config{};
    bool m_running = true;
    Difficulty m_difficulty = Difficulty::Normal;
    GameMode m_defaultGameMode = GameMode::Survival;
    i32 m_idleTimeoutMinutes = 0;
    bool m_stopRequested = false;
    std::string m_lastBroadcastMessage;

    // 粒子广播记录
    bool m_particleBroadcastCalled = false;
    u32 m_lastParticleType = 0;
    f64 m_lastParticleX = 0.0;
    f64 m_lastParticleY = 0.0;
    f64 m_lastParticleZ = 0.0;
    u32 m_lastParticleCount = 0;

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
    mc::command::CommandRegistry m_commandRegistry;
    server::ServerScoreboard m_scoreboard;
    std::vector<std::shared_ptr<FakeConnection>> m_connections;
};

class CommandRegistryServerTest : public ::testing::Test {
protected:
    /**
     * @brief 构造一个玩家命令源。
     *
     * @param playerId 玩家 ID。
     * @param username 玩家名。
     * @return 绑定到该玩家的命令源。
     *
     * @note 该辅助函数不会重复建玩家，调用前应先通过 `addTestPlayer()` 完成注册。
     */
    [[nodiscard]] ServerCommandSource makePlayerSource(PlayerId playerId, const std::string& username)
    {
        auto* playerData = m_server.playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            ADD_FAILURE() << "Player must exist before creating a player source";
            return ServerCommandSource::forConsole(&m_server);
        }

        return ServerCommandSource(&m_server,
            nullptr,
            nullptr,
            Vector3d(playerData->x, playerData->y, playerData->z),
            Vector2f(playerData->yaw, playerData->pitch),
            2,
            playerId,
            username);
    }

    FakeServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(CommandRegistryServerTest, HelpListsDynamicCommands)
{
    const auto result = m_server.commandRegistry().execute("help", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(m_server.lastBroadcastMessage().empty());
}

TEST_F(CommandRegistryServerTest, DifficultyCommandUpdatesServerDifficulty)
{
    const auto result = m_server.commandRegistry().execute("difficulty hard", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(m_server.difficulty(), Difficulty::Hard);
}

TEST_F(CommandRegistryServerTest, DefaultGameModeCommandUpdatesServerDefault)
{
    const auto result = m_server.commandRegistry().execute("defaultgamemode creative", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(m_server.defaultGameMode(), GameMode::Creative);
}

TEST_F(CommandRegistryServerTest, SayCommandBroadcastsServerMessage)
{
    const auto result = m_server.commandRegistry().execute("say hello world", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(m_server.lastBroadcastMessage(), "[Console] hello world");
}

TEST_F(CommandRegistryServerTest, StopCommandRequestsServerStop)
{
    const auto result = m_server.commandRegistry().execute("stop", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(m_server.stopRequested());
}

TEST_F(CommandRegistryServerTest, SetIdleTimeoutCommandUpdatesServerTimeout)
{
    const auto result = m_server.commandRegistry().execute("setidletimeout 15", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(m_server.playerIdleTimeoutMinutes(), 15);
}

TEST_F(CommandRegistryServerTest, KickCommandDisconnectsSelectedPlayers)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    const auto connection = m_server.lastConnection();
    ASSERT_NE(connection, nullptr);

    const auto result = m_server.commandRegistry().execute("kick Steve", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_FALSE(connection->isConnected());
    EXPECT_EQ(connection->disconnectReason(), "Kicked by an operator");
    EXPECT_EQ(m_server.playerManager().getPlayer(1), nullptr);
}

TEST_F(CommandRegistryServerTest, KickCommandUsesCustomReason)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    const auto connection = m_server.lastConnection();
    ASSERT_NE(connection, nullptr);

    const auto result = m_server.commandRegistry().execute("kick Steve excessive ping", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_EQ(connection->disconnectReason(), "excessive ping");
}

TEST_F(CommandRegistryServerTest, GameModeCommandUpdatesSelectedPlayers)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    auto* alex = m_server.addTestPlayer(2, "Alex");
    ASSERT_NE(steve, nullptr);
    ASSERT_NE(alex, nullptr);

    const auto result = m_server.commandRegistry().execute("gamemode creative @a", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 2);
    EXPECT_EQ(m_server.playerManager().getPlayer(1)->gameMode, GameMode::Creative);
    EXPECT_EQ(m_server.playerManager().getPlayer(2)->gameMode, GameMode::Creative);
}

TEST_F(CommandRegistryServerTest, TeleportCommandMovesSelectedPlayersToCoordinates)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    auto* alex = m_server.addTestPlayer(2, "Alex");
    ASSERT_NE(steve, nullptr);
    ASSERT_NE(alex, nullptr);

    const auto result = m_server.commandRegistry().execute("tp @a 10 80 -5", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 2);

    const auto* updatedSteve = m_server.playerManager().getPlayer(1);
    const auto* updatedAlex = m_server.playerManager().getPlayer(2);
    ASSERT_NE(updatedSteve, nullptr);
    ASSERT_NE(updatedAlex, nullptr);
    EXPECT_FLOAT_EQ(updatedSteve->x, 10.0f);
    EXPECT_FLOAT_EQ(updatedSteve->y, 80.0f);
    EXPECT_FLOAT_EQ(updatedSteve->z, -5.0f);
    EXPECT_FLOAT_EQ(updatedAlex->x, 10.0f);
    EXPECT_FLOAT_EQ(updatedAlex->y, 80.0f);
    EXPECT_FLOAT_EQ(updatedAlex->z, -5.0f);
}

TEST_F(CommandRegistryServerTest, TeleportCommandMovesSelfToCoordinatesWithSlashPrefix)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);

    auto playerSource = makePlayerSource(1, "Steve");
    const auto result = m_server.commandRegistry().execute("/tp 100 100 100", playerSource);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);

    const auto* updatedSteve = m_server.playerManager().getPlayer(1);
    ASSERT_NE(updatedSteve, nullptr);
    EXPECT_FLOAT_EQ(updatedSteve->x, 100.0f);
    EXPECT_FLOAT_EQ(updatedSteve->y, 100.0f);
    EXPECT_FLOAT_EQ(updatedSteve->z, 100.0f);
}

TEST_F(CommandRegistryServerTest, TeleportCommandMovesSelfToNamedPlayer)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    auto* alex = m_server.addTestPlayer(2, "Alex");
    ASSERT_NE(steve, nullptr);
    ASSERT_NE(alex, nullptr);

    alex->x = 30.0f;
    alex->y = 90.0f;
    alex->z = -40.0f;
    alex->yaw = 45.0f;
    alex->pitch = -15.0f;

    auto playerSource = makePlayerSource(1, "Steve");
    const auto result = m_server.commandRegistry().execute("tp Alex", playerSource);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);

    const auto* updatedSteve = m_server.playerManager().getPlayer(1);
    ASSERT_NE(updatedSteve, nullptr);
    EXPECT_FLOAT_EQ(updatedSteve->x, 30.0f);
    EXPECT_FLOAT_EQ(updatedSteve->y, 90.0f);
    EXPECT_FLOAT_EQ(updatedSteve->z, -40.0f);
    EXPECT_FLOAT_EQ(updatedSteve->yaw, 45.0f);
    EXPECT_FLOAT_EQ(updatedSteve->pitch, -15.0f);
}

TEST_F(CommandRegistryServerTest, TimeCommandSupportsNamedSetValues)
{
    const auto result = m_server.commandRegistry().execute("time set noon", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 6000);
    EXPECT_EQ(m_server.timeManager().dayTime(), 6000);
}

TEST_F(CommandRegistryServerTest, TimeCommandReturnsQueriedDaytime)
{
    m_server.timeManager().setDayTime(13000);

    const auto result = m_server.commandRegistry().execute("time query daytime", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 13000);
}

TEST_F(CommandRegistryServerTest, ClearCommandClearsSelfInventory)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);

    auto* inventory = m_server.inventoryManager().getInventory(1);
    ASSERT_NE(inventory, nullptr);

    inventory->setItem(0, ItemStack(*Items::STONE, 32));
    inventory->setItem(10, ItemStack(*Items::IRON_INGOT, 7));
    inventory->setItem(InventorySlots::ARMOR_HEAD, ItemStack(*Items::IRON_HELMET, 1));
    inventory->setItem(InventorySlots::OFFHAND, ItemStack(*Items::COBBLESTONE, 4));

    auto playerSource = makePlayerSource(1, "Steve");
    const auto result = m_server.commandRegistry().execute("clear", playerSource);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 44);
    EXPECT_TRUE(inventory->isEmpty());

    const auto connection = m_server.lastConnection();
    ASSERT_NE(connection, nullptr);
    EXPECT_TRUE(connection->isConnected());
    EXPECT_GT(connection->sentBytes(), 0u);
}

TEST_F(CommandRegistryServerTest, ClearCommandClearsNamedPlayerInventory)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    auto* alex = m_server.addTestPlayer(2, "Alex");
    ASSERT_NE(steve, nullptr);
    ASSERT_NE(alex, nullptr);

    auto* inventory = m_server.inventoryManager().getInventory(2);
    ASSERT_NE(inventory, nullptr);

    inventory->setItem(0, ItemStack(*Items::STONE, 16));
    inventory->setItem(1, ItemStack(*Items::IRON_INGOT, 8));
    inventory->setItem(InventorySlots::OFFHAND, ItemStack(*Items::COBBLESTONE, 4));

    const auto result = m_server.commandRegistry().execute("clear Alex", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 28);
    EXPECT_TRUE(inventory->isEmpty());

    const auto connection = m_server.lastConnection();
    ASSERT_NE(connection, nullptr);
    EXPECT_TRUE(connection->isConnected());
    EXPECT_GT(connection->sentBytes(), 0u);
}

TEST_F(CommandRegistryServerTest, ClearCommandRespectsItemAndMaxCount)
{
    auto* alex = m_server.addTestPlayer(2, "Alex");
    ASSERT_NE(alex, nullptr);

    auto* inventory = m_server.inventoryManager().getInventory(2);
    ASSERT_NE(inventory, nullptr);

    inventory->setItem(0, ItemStack(*Items::STONE, 5));
    inventory->setItem(1, ItemStack(*Items::STONE, 4));
    inventory->setItem(2, ItemStack(*Items::IRON_INGOT, 2));

    const auto result = m_server.commandRegistry().execute("clear Alex minecraft:stone 6", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 6);

    EXPECT_TRUE(inventory->getItem(0).isEmpty());

    const ItemStack slot1 = inventory->getItem(1);
    ASSERT_FALSE(slot1.isEmpty());
    EXPECT_EQ(slot1.getItem(), Items::STONE);
    EXPECT_EQ(slot1.getCount(), 3);

    const ItemStack slot2 = inventory->getItem(2);
    ASSERT_FALSE(slot2.isEmpty());
    EXPECT_EQ(slot2.getItem(), Items::IRON_INGOT);
    EXPECT_EQ(slot2.getCount(), 2);

    const auto connection = m_server.lastConnection();
    ASSERT_NE(connection, nullptr);
    EXPECT_TRUE(connection->isConnected());
    EXPECT_GT(connection->sentBytes(), 0u);
}
TEST_F(CommandRegistryServerTest, CommandTreeSnapshotContainsMetadata)
{
    const auto snapshot = m_server.commandRegistry().getCommandTreeSnapshot();

    auto findNodeByName = [&snapshot](std::string_view name) -> const CommandTreeNodeSnapshot* {
        for (const auto& node : snapshot.nodes) {
            if (node.name == name) {
                return &node;
            }
        }
        return nullptr;
    };

    const auto* helpNode = findNodeByName("help");
    ASSERT_NE(helpNode, nullptr);
    ASSERT_TRUE(helpNode->metadata.contains("description"));
    EXPECT_EQ(helpNode->metadata.at("description").get<std::string>(), "Show command help.");

    const auto* experienceNode = findNodeByName("experience");
    ASSERT_NE(experienceNode, nullptr);
    ASSERT_TRUE(experienceNode->metadata.contains("aliases"));
    EXPECT_EQ(experienceNode->metadata.at("aliases").at(0).get<std::string>(), "xp");

    const auto* kickNode = findNodeByName("kick");
    ASSERT_NE(kickNode, nullptr);
    ASSERT_TRUE(kickNode->metadata.contains("implemented"));
    EXPECT_TRUE(kickNode->metadata.at("implemented").get<bool>());

    const auto* tpNode = findNodeByName("tp");
    ASSERT_NE(tpNode, nullptr);
    ASSERT_TRUE(tpNode->metadata.contains("implemented"));
    EXPECT_TRUE(tpNode->metadata.at("implemented").get<bool>());
}

TEST_F(CommandRegistryServerTest, ParticleCommandBroadcastsParticleAtCurrentPosition)
{
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    steve->x = 100.0f;
    steve->y = 64.0f;
    steve->z = -200.0f;

    auto playerSource = makePlayerSource(1, "Steve");
    const auto result = m_server.commandRegistry().execute("particle flame", playerSource);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_TRUE(m_server.particleBroadcastCalled());

    // 验证粒子类型 - flame = 20 (ParticleTypeId::Flame)
    EXPECT_EQ(m_server.lastParticleType(), 20u);

    // 验证位置使用命令源的位置
    EXPECT_DOUBLE_EQ(m_server.lastParticleX(), 100.0);
    EXPECT_DOUBLE_EQ(m_server.lastParticleY(), 64.0);
    EXPECT_DOUBLE_EQ(m_server.lastParticleZ(), -200.0);
    EXPECT_EQ(m_server.lastParticleCount(), 1u);
}

TEST_F(CommandRegistryServerTest, ParticleCommandBroadcastsParticleAtSpecifiedPosition)
{
    const auto result = m_server.commandRegistry().execute("particle smoke 50.5 70.0 -100.5", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_TRUE(m_server.particleBroadcastCalled());

    // 验证粒子类型 - smoke = 21 (ParticleTypeId::Smoke)
    EXPECT_EQ(m_server.lastParticleType(), 21u);

    // 验证位置使用指定位置
    EXPECT_DOUBLE_EQ(m_server.lastParticleX(), 50.5);
    EXPECT_DOUBLE_EQ(m_server.lastParticleY(), 70.0);
    EXPECT_DOUBLE_EQ(m_server.lastParticleZ(), -100.5);
    EXPECT_EQ(m_server.lastParticleCount(), 1u);
}

TEST_F(CommandRegistryServerTest, ParticleCommandRejectsUnknownParticleType)
{
    const auto result = m_server.commandRegistry().execute("particle unknown_particle", m_console);

    // 未知粒子类型返回 0（失败）但命令本身执行成功
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
    EXPECT_FALSE(m_server.particleBroadcastCalled());
}

TEST_F(CommandRegistryServerTest, ParticleCommandAcceptsMinecraftNamespace)
{
    const auto result = m_server.commandRegistry().execute("particle minecraft:lava 0 0 0", m_console);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_TRUE(m_server.particleBroadcastCalled());

    // 验证粒子类型 - lava = 23 (ParticleTypeId::Lava)
    EXPECT_EQ(m_server.lastParticleType(), 23u);
}

// ========== SpectateCommand 测试 ==========

TEST_F(CommandRegistryServerTest, SpectateCommandRequiresSpectatorGameMode)
{
    // 创建一个非观察者模式的玩家
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    steve->gameMode = GameMode::Survival; // 设置为生存模式

    auto playerSource = makePlayerSource(1, "Steve");

    // 尝试执行 spectate 命令，应该失败（玩家不在观察者模式）
    const auto result = m_server.commandRegistry().execute("spectate @e[type=player,limit=1]", playerSource);

    // 命令执行成功但返回 0（没有玩家被影响）
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CommandRegistryServerTest, SpectateCommandAcceptsSpectatorGameMode)
{
    // 创建一个观察者模式的玩家
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    steve->gameMode = GameMode::Spectator; // 设置为观察者模式

    auto playerSource = makePlayerSource(1, "Steve");

    // 执行 spectate 命令（目标为自己作为测试）
    const auto result = m_server.commandRegistry().execute("spectate @p", playerSource);

    // 命令执行成功
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(CommandRegistryServerTest, SpectateCommandRejectsCreativeGameMode)
{
    // 创建一个创造模式的玩家
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    steve->gameMode = GameMode::Creative;

    auto playerSource = makePlayerSource(1, "Steve");

    // 执行 spectate 命令，应该失败
    const auto result = m_server.commandRegistry().execute("spectate @p", playerSource);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CommandRegistryServerTest, SpectateCommandRejectsAdventureGameMode)
{
    // 创建一个冒险模式的玩家
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    steve->gameMode = GameMode::Adventure;

    auto playerSource = makePlayerSource(1, "Steve");

    // 执行 spectate 命令，应该失败
    const auto result = m_server.commandRegistry().execute("spectate @p", playerSource);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CommandRegistryServerTest, SpectateCommandStopWorksForAnyGameMode)
{
    // /spectate stop 命令不需要观察者模式
    auto* steve = m_server.addTestPlayer(1, "Steve");
    ASSERT_NE(steve, nullptr);
    steve->gameMode = GameMode::Survival;

    auto playerSource = makePlayerSource(1, "Steve");

    // 执行 spectate stop 命令
    const auto result = m_server.commandRegistry().execute("spectate stop", playerSource);

    // 命令执行成功
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

} // namespace
} // namespace mc::command
