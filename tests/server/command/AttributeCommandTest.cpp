/**
 * @file AttributeCommandTest.cpp
 * @brief AttributeCommand 单元测试
 *
 * 测试 /attribute 命令的注册、解析和权限检查。
 * 属性操作完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/AttributeCommand.hpp"
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
#include "common/entity/inventory/PlayerInventory.hpp"
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

    [[nodiscard]] size_t sentBytes() const noexcept { return m_sentData.size(); }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

/**
 * @brief 测试服务器，用于命令测试。
 */
class AttributeTestServer final : public server::IServer {
public:
    AttributeTestServer()
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
    CommandRegistry m_commandRegistry;
    std::vector<std::shared_ptr<FakeConnection>> m_connections;
};

class AttributeCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        AttributeCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    AttributeTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(AttributeCommandTest, AttributeCommandIsRegistered)
{
    // 验证 attribute 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 attribute 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "attribute") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "attribute command should be registered";
}

TEST_F(AttributeCommandTest, AttributeCommandRequiresPermissionLevel2)
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
        const auto result = m_server.commandRegistry().execute("attribute @p minecraft:generic.max_health get", lowPermSource);
        permissionDenied = (result.value() == 0);
    } catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ========== get 子命令测试 ==========

TEST_F(AttributeCommandTest, GetAttributeSyntax)
{
    // 测试 get 语法
    const auto result = m_server.commandRegistry().execute("attribute @p minecraft:generic.max_health get", m_console);

    // 命令执行成功，但由于没有玩家实体，返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, GetAttributeWithoutNamespace)
{
    // 测试不带命名空间的属性名
    const auto result = m_server.commandRegistry().execute("attribute @p generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, GetAttributeWithShortName)
{
    // 测试短属性名（需要自动添加 generic. 前缀）
    const auto result = m_server.commandRegistry().execute("attribute @p max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, GetMultipleAttributes)
{
    // 测试获取各种属性
    const char* attributes[] = {
        "max_health",
        "follow_range",
        "knockback_resistance",
        "movement_speed",
        "attack_damage",
        "attack_speed",
        "armor",
        "luck"
    };

    for (const char* attr : attributes) {
        std::string cmd = std::string("attribute @p ") + attr + " get";
        const auto result = m_server.commandRegistry().execute(cmd, m_console);
        EXPECT_TRUE(result.success()) << "attribute " << attr << " should be parseable";
    }
}

// ========== set 子命令测试 ==========

TEST_F(AttributeCommandTest, SetAttributeSyntax)
{
    // 测试 set 语法
    const auto result = m_server.commandRegistry().execute("attribute @p minecraft:generic.max_health set 20.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetAttributeWithFloatValue)
{
    // 测试浮点数值
    const auto result = m_server.commandRegistry().execute("attribute @p generic.movement_speed set 0.15", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetAttributeWithIntValue)
{
    // 测试整数值（会隐式转换为浮点）
    const auto result = m_server.commandRegistry().execute("attribute @p generic.max_health set 30", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetMovementSpeed)
{
    // 测试设置移动速度
    const auto result = m_server.commandRegistry().execute("attribute @p movement_speed set 0.1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetAttackDamage)
{
    // 测试设置攻击伤害
    const auto result = m_server.commandRegistry().execute("attribute @p attack_damage set 5.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetKnockbackResistance)
{
    // 测试设置击退抗性（0-1 范围）
    const auto result = m_server.commandRegistry().execute("attribute @p knockback_resistance set 0.5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetArmor)
{
    // 测试设置护甲值（0-30 范围）
    const auto result = m_server.commandRegistry().execute("attribute @p armor set 20.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetLuck)
{
    // 测试设置幸运值
    const auto result = m_server.commandRegistry().execute("attribute @p luck set 1024.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 选择器测试 ==========

TEST_F(AttributeCommandTest, SelectorWithNoPlayersReturnsZero)
{
    // 测试没有目标玩家
    const auto result = m_server.commandRegistry().execute("attribute @p generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SelectorWithMultiplePlayersFails)
{
    // 测试多目标选择器 @a - 应该在解析时失败
    // MC 1.16.5 只允许单个实体，@a 应该在解析阶段失败
    const auto result = m_server.commandRegistry().execute("attribute @a generic.max_health get", m_console);

    // @a 选择器在解析时应该抛出错误（只允许单个实体）
    EXPECT_FALSE(result.success());
}

// ========== 未知属性测试 ==========

TEST_F(AttributeCommandTest, UnknownAttributeReturnsZero)
{
    // 测试未知属性
    const auto result = m_server.commandRegistry().execute("attribute @p unknown_attribute get", m_console);

    // 命令执行成功，但由于没有玩家实体，无法验证属性是否有效
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 马属性测试 ==========

TEST_F(AttributeCommandTest, HorseJumpStrengthAttribute)
{
    // 测试马专属属性
    const auto result = m_server.commandRegistry().execute("attribute @p horse.jump_strength get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

} // namespace
} // namespace mc::command
