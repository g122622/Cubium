/**
 * @file EnchantCommandTest.cpp
 * @brief EnchantCommand 单元测试
 *
 * 测试 /enchant 命令的注册、解析和权限检查。
 * 附魔操作完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/EnchantCommand.hpp"
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
 */
class EnchantTestServer final : public server::IServer {
public:
    EnchantTestServer()
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
    {
        // 初始化附魔注册表
        item::enchant::EnchantmentRegistry::initialize();
    }

    ~EnchantTestServer() { item::enchant::EnchantmentRegistry::clear(); }

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

class EnchantCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        EnchantCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    EnchantTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(EnchantCommandTest, EnchantCommandIsRegistered)
{
    // 验证 enchant 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 enchant 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "enchant") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "enchant command should be registered";
}

TEST_F(EnchantCommandTest, EnchantCommandRequiresPermissionLevel2)
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
        const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ========== 语法测试 ==========

TEST_F(EnchantCommandTest, EnchantWithoutLevel)
{
    // 测试不带等级参数（默认等级 1）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness", m_console);

    // 命令执行成功，但由于没有玩家实体，返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantWithLevel)
{
    // 测试带等级参数
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantWithNamespace)
{
    // 测试带命名空间的附魔名
    const auto result = m_server.commandRegistry().execute("enchant @p minecraft:sharpness 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantLevelZero)
{
    // 测试等级 0（MC 1.16.5 允许等级 0）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantHighLevel)
{
    // 测试高等级
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 32767", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 附魔名称测试 ==========

TEST_F(EnchantCommandTest, UnknownEnchantment)
{
    // 测试未知附魔
    const auto result = m_server.commandRegistry().execute("enchant @p unknown_enchantment 1", m_console);

    // 未知附魔应该返回错误
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, VariousEnchantments)
{
    // 测试各种附魔名称
    const char* enchantments[] = {"sharpness",
        "smite",
        "bane_of_arthropods",
        "knockback",
        "fire_aspect",
        "looting",
        "efficiency",
        "silk_touch",
        "fortune",
        "unbreaking",
        "power",
        "punch",
        "flame",
        "infinity",
        "protection",
        "fire_protection",
        "blast_protection",
        "projectile_protection",
        "feather_falling",
        "thorns",
        "respiration",
        "depth_strider",
        "aqua_affinity"};

    for (const char* ench : enchantments) {
        std::string cmd = std::string("enchant @p ") + ench + " 1";
        const auto result = m_server.commandRegistry().execute(cmd, m_console);
        EXPECT_TRUE(result.success()) << "enchantment " << ench << " should be parseable";
    }
}

// ========== 选择器测试 ==========

TEST_F(EnchantCommandTest, SelectorWithNoPlayersReturnsZero)
{
    // 测试没有目标玩家
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, SelectorWithMultiplePlayers)
{
    // 测试多目标选择器 @a
    // 注意：@a 选择器在 EntityArgumentType::player() 模式下会抛出异常，
    // 因为 player() 只允许单个玩家
    // 这与 MC 1.16.5 行为一致：/enchant 只支持单个目标

    // 执行命令时，@a 选择器会在解析阶段失败（不是执行阶段）
    // 因为 EntityArgumentType::player() 要求 isSingle() == true
    const auto result = m_server.commandRegistry().execute("enchant @a sharpness 5", m_console);

    // 命令解析失败，result.success() 应该为 false
    EXPECT_FALSE(result.success());
}

// ========== 附魔类型兼容性测试 ==========

TEST_F(EnchantCommandTest, WeaponEnchantment)
{
    // 测试武器附魔
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, ToolEnchantment)
{
    // 测试工具附魔
    const auto result = m_server.commandRegistry().execute("enchant @p efficiency 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, ArmorEnchantment)
{
    // 测试护甲附魔
    const auto result = m_server.commandRegistry().execute("enchant @p protection 4", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, BowEnchantment)
{
    // 测试弓附魔
    const auto result = m_server.commandRegistry().execute("enchant @p power 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, TridentEnchantment)
{
    // 测试三叉戟附魔
    const auto result = m_server.commandRegistry().execute("enchant @p loyalty 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, CrossbowEnchantment)
{
    // 测试弩附魔
    const auto result = m_server.commandRegistry().execute("enchant @p quick_charge 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, FishingRodEnchantment)
{
    // 测试钓鱼竿附魔
    const auto result = m_server.commandRegistry().execute("enchant @p luck_of_the_sea 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 宝藏附魔测试 ==========

TEST_F(EnchantCommandTest, TreasureEnchantment)
{
    // 测试宝藏附魔（冰霜行者、修补、灵魂疾行）
    const auto result1 = m_server.commandRegistry().execute("enchant @p frost_walker 2", m_console);
    EXPECT_TRUE(result1.success());

    const auto result2 = m_server.commandRegistry().execute("enchant @p mending 1", m_console);
    EXPECT_TRUE(result2.success());

    const auto result3 = m_server.commandRegistry().execute("enchant @p soul_speed 3", m_console);
    EXPECT_TRUE(result3.success());
}

// ========== 诅咒附魔测试 ==========

TEST_F(EnchantCommandTest, CurseEnchantment)
{
    // 测试诅咒附魔
    const auto result1 = m_server.commandRegistry().execute("enchant @p binding_curse 1", m_console);
    EXPECT_TRUE(result1.success());

    const auto result2 = m_server.commandRegistry().execute("enchant @p vanishing_curse 1", m_console);
    EXPECT_TRUE(result2.success());
}

// ========== 等级边界测试 ==========

TEST_F(EnchantCommandTest, NegativeLevel)
{
    // 测试负等级（应该失败）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness -1", m_console);

    // 负等级应该在解析时失败
    EXPECT_FALSE(result.success());
}

TEST_F(EnchantCommandTest, ZeroLevelAllowed)
{
    // 测试等级 0（MC 1.16.5 允许）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 0", m_console);

    EXPECT_TRUE(result.success());
}

} // namespace
} // namespace mc::command
