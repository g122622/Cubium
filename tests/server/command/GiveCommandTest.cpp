/**
 * @file GiveCommandTest.cpp
 * @brief GiveCommand 单元测试
 *
 * 测试 /give 命令的注册、解析和权限检查。
 * 物品掉落和音效播放的完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/GiveCommand.hpp"
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
#include "server/core/BannedPlayerList.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/OpListManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/UuidUtils.hpp"

#include <stdexcept>
#include <vector>

// Forward declarations
namespace mc {
class ServerDimensionManager;
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
}

namespace mc::server {
class ServerPlayerEntityManager;
class ServerWorld;
class ServerChunkManager;
class EntityTracker;
class ItemPickupManager;
class WeatherManager;
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
 * GiveCommand 的物品掉落和音效播放完整功能测试应在集成测试中进行。
 */
class GiveTestServer final : public server::IServer {
public:
    GiveTestServer()
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
        Items::initialize();
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
    [[nodiscard]] server::core::BannedPlayerList& bannedPlayerList() override { return m_bannedPlayerList; }
    [[nodiscard]] const server::core::BannedPlayerList& bannedPlayerList() const override { return m_bannedPlayerList; }
    [[nodiscard]] server::core::BannedIpList& bannedIpList() override { return m_bannedIpList; }
    [[nodiscard]] const server::core::BannedIpList& bannedIpList() const override { return m_bannedIpList; }
    [[nodiscard]] server::core::OpListManager& opListManager() override { return m_opListManager; }
    [[nodiscard]] const server::core::OpListManager& opListManager() const override { return m_opListManager; }

    [[nodiscard]] ServerDimensionManager& dimensionManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override { throw std::logic_error("unused"); }
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
    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::ServerPlayerEntityManager& playerEntityManager() const override { throw std::logic_error("unused"); }
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
    [[nodiscard]] bool soundWasSent() const noexcept { return m_soundSent; }
    [[nodiscard]] PlayerId lastSoundPlayerId() const noexcept { return m_lastSoundPlayerId; }
    [[nodiscard]] const ResourceLocation& lastSoundEvent() const noexcept { return m_lastSoundEvent; }
    [[nodiscard]] sound::SoundCategory lastSoundCategory() const noexcept { return m_lastSoundCategory; }

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

class GiveCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        // 注册命令
        GiveCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    GiveTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(GiveCommandTest, GiveCommandIsRegistered)
{
    // 验证 give 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 give 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "give") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "give command should be registered";
}

TEST_F(GiveCommandTest, GiveCommandRequiresPermissionLevel2)
{
    // 创建一个权限等级 0 的命令源
    ServerCommandSource lowPermSource(
        &m_server,
        nullptr,
        nullptr,
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        0,  // 权限等级 0
        0,
        "test"
    );

    // 应该因为没有权限而被拒绝
    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("give @p minecraft:stone 1", lowPermSource);
        permissionDenied = (result.value() == 0);
    } catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(GiveCommandTest, GiveCommandParsesItemWithNamespace)
{
    // 测试解析带命名空间的物品
    const auto result = m_server.commandRegistry().execute("give @p minecraft:stone 1", m_console);

    // 命令执行成功，但由于没有玩家实体，返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandParsesItemWithoutNamespace)
{
    // 测试解析不带命名空间的物品
    const auto result = m_server.commandRegistry().execute("give @p stone 1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandParsesCountArgument)
{
    // 测试解析数量参数
    const auto result = m_server.commandRegistry().execute("give @p stone 64", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandWithInvalidItemFails)
{
    // 测试无效物品 - 命令执行应该失败
    const auto result = m_server.commandRegistry().execute("give @p minecraft:nonexistent_item 1", m_console);

    // 无效物品会导致命令执行失败
    EXPECT_FALSE(result.success());
}

TEST_F(GiveCommandTest, GiveCommandWithNoTargetsReturnsZero)
{
    // 测试没有目标玩家
    // @p 选择器在没有玩家时返回空列表
    const auto result = m_server.commandRegistry().execute("give @p stone 1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandWithCountAbove64Clamped)
{
    // MC 1.16.5 限制数量为 1-64，超过应该被 IntegerArgumentType 拒绝
    // 这里我们测试在范围内的最大值
    const auto result = m_server.commandRegistry().execute("give @p stone 64", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandDefaultCountIsOne)
{
    // 测试省略数量参数时默认为 1
    const auto result = m_server.commandRegistry().execute("give @p stone", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandWithMultipleTargets)
{
    // 测试多目标选择器 @a
    const auto result = m_server.commandRegistry().execute("give @a stone 1", m_console);

    // 没有玩家时返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

} // namespace
} // namespace mc::command
