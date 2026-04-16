#include <gtest/gtest.h>

#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
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
#include "common/network/connection/IServerConnection.hpp"

#include <stdexcept>
#include <vector>

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
    void disconnect(const String& reason = "") override
    {
        m_disconnectReason = reason;
        m_connected = false;
    }

    /**
     * @brief 查询连接是否仍有效。
     *
     * @return `true` 表示仍连接。
     */
    [[nodiscard]] bool isConnected() const override
    {
        return m_connected;
    }

    /**
     * @brief 获取连接标识字符串。
     */
    [[nodiscard]] String identifier() const override
    {
        return "FakeConnection";
    }

    /**
     * @brief 获取连接类型。
     */
    [[nodiscard]] network::ConnectionType type() const override
    {
        return network::ConnectionType::Local;
    }

    /**
     * @brief 获取最近一次断开原因。
     */
    [[nodiscard]] const String& disconnectReason() const noexcept
    {
        return m_disconnectReason;
    }

private:
    bool m_connected = true;
    String m_disconnectReason;
    std::vector<u8> m_sentData;
};

class FakeServer final : public server::IServer {
public:
    FakeServer()
        : m_playerManager(m_config)
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
    [[nodiscard]] const server::ItemPickupManager& itemPickupManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::interaction::BlockInteractionManager& blockInteractionManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::BlockInteractionManager& blockInteractionManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::interaction::MiningManager& miningManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::MiningManager& miningManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::interaction::ContainerManager& containerManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::ContainerManager& containerManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::interaction::InventoryManager& inventoryManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::interaction::InventoryManager& inventoryManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::sync::EntitySyncManager& entitySyncManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::EntitySyncManager& entitySyncManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::sync::ChunkSendManager& chunkSendManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::ChunkSendManager& chunkSendManager() const override { throw std::logic_error("unused"); }
    [[nodiscard]] server::sync::LightSyncManager& lightSyncManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::sync::LightSyncManager& lightSyncManager() const override { throw std::logic_error("unused"); }

    [[nodiscard]] mc::command::CommandRegistry& commandRegistry() override { return m_commandRegistry; }
    [[nodiscard]] const mc::command::CommandRegistry& commandRegistry() const override { return m_commandRegistry; }

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
    void broadcastServerMessage(StringView message) override { m_lastBroadcastMessage = String(message); }
    void requestStop() override { m_stopRequested = true; m_running = false; }

    /**
     * @brief 向测试服务器添加一个在线玩家。
     *
     * @param playerId 玩家 ID。
     * @param username 玩家名。
     * @return 新增玩家数据指针。
     *
     * @warning 测试用例应保证 `playerId` 唯一，否则 `PlayerManager` 会拒绝插入。
     */
    [[nodiscard]] server::ServerPlayerData* addTestPlayer(PlayerId playerId, const String& username)
    {
        auto connection = std::make_shared<FakeConnection>();
        auto* player = m_playerManager.addPlayer(playerId, username, connection);
        if (player != nullptr) {
            m_connections.push_back(connection);
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

    [[nodiscard]] const String& lastBroadcastMessage() const noexcept { return m_lastBroadcastMessage; }
    [[nodiscard]] bool stopRequested() const noexcept { return m_stopRequested; }

private:
    server::ServerCoreConfig m_config{};
    bool m_running = true;
    Difficulty m_difficulty = Difficulty::Normal;
    GameMode m_defaultGameMode = GameMode::Survival;
    i32 m_idleTimeoutMinutes = 0;
    bool m_stopRequested = false;
    String m_lastBroadcastMessage;

    server::core::PlayerManager m_playerManager;
    server::core::ConnectionManager m_connectionManager;
    server::core::TimeManager m_timeManager;
    server::core::TeleportManager m_teleportManager;
    server::core::KeepAliveManager m_keepAliveManager;
    server::core::PositionTracker m_positionTracker;
    server::core::PacketHandler m_packetHandler;
    server::core::GameModeManager m_gameModeManager;
    mc::command::CommandRegistry m_commandRegistry;
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
    [[nodiscard]] ServerCommandSource makePlayerSource(PlayerId playerId, const String& username)
    {
        auto* playerData = m_server.playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            ADD_FAILURE() << "Player must exist before creating a player source";
            return ServerCommandSource::forConsole(&m_server);
        }

        return ServerCommandSource(
            &m_server,
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
    EXPECT_EQ(helpNode->metadata.at("description").get<String>(), "Show command help.");

    const auto* experienceNode = findNodeByName("experience");
    ASSERT_NE(experienceNode, nullptr);
    ASSERT_TRUE(experienceNode->metadata.contains("aliases"));
    EXPECT_EQ(experienceNode->metadata.at("aliases").at(0).get<String>(), "xp");

    const auto* kickNode = findNodeByName("kick");
    ASSERT_NE(kickNode, nullptr);
    ASSERT_TRUE(kickNode->metadata.contains("implemented"));
    EXPECT_TRUE(kickNode->metadata.at("implemented").get<bool>());

    const auto* tpNode = findNodeByName("tp");
    ASSERT_NE(tpNode, nullptr);
    ASSERT_TRUE(tpNode->metadata.contains("implemented"));
    EXPECT_TRUE(tpNode->metadata.at("implemented").get<bool>());
}

} // namespace
} // namespace mc::command

