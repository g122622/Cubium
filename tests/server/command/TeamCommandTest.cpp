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
 * @file TeamCommandTest.cpp
 * @brief TeamCommand 单元测试
 *
 * 测试 /team 命令的所有子命令：
 * - add: 创建队伍
 * - remove: 移除队伍
 * - list: 列出队伍
 * - empty: 清空队伍
 * - join: 加入队伍
 * - leave: 离开队伍
 * - modify: 修改队伍属性
 */

#include <gtest/gtest.h>

#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/TeamCommand.hpp"
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
 */
class TeamTestServer final : public server::IServer {
public:
    TeamTestServer()
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

    [[nodiscard]] CommandRegistry& commandRegistry() override { return m_commandRegistry; }
    [[nodiscard]] const CommandRegistry& commandRegistry() const override { return m_commandRegistry; }

    [[nodiscard]] server::ServerScoreboard& scoreboard() override { return m_scoreboard; }
    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override { return m_scoreboard; }
    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override { throw std::logic_error("unused"); }
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override
    {
        throw std::logic_error("unused");
    }

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
    server::ServerScoreboard m_scoreboard;
    std::vector<std::shared_ptr<FakeConnection>> m_connections;
};

class TeamCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        TeamCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    TeamTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(TeamCommandTest, TeamCommandIsRegistered)
{
    // 验证 team 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 team 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "team") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "team command should be registered";
}

// ========== addTeam 测试 ==========

TEST_F(TeamCommandTest, AddTeamCreatesTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 初始应该没有队伍
    EXPECT_EQ(scoreboard.getTeams().size(), 0u);

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    EXPECT_EQ(team->getName(), "red");

    // 验证队伍已创建
    EXPECT_EQ(scoreboard.getTeams().size(), 1u);
    EXPECT_TRUE(scoreboard.hasTeam("red"));
}

TEST_F(TeamCommandTest, AddTeamWithDisplayName)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("blue");
    ASSERT_NE(team, nullptr);

    // 设置显示名称
    team->setDisplayName(std::make_unique<text::StringTextComponent>("Blue Team"));

    EXPECT_NE(team->getDisplayName(), nullptr);
}

TEST_F(TeamCommandTest, AddTeamDuplicateNameFails)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建第一个队伍
    auto* team1 = scoreboard.createTeam("red");
    ASSERT_NE(team1, nullptr);

    // 尝试创建同名队伍
    auto* team2 = scoreboard.createTeam("red");
    EXPECT_EQ(team2, nullptr);

    // 验证只有一个队伍
    EXPECT_EQ(scoreboard.getTeams().size(), 1u);
}

// ========== removeTeam 测试 ==========

TEST_F(TeamCommandTest, RemoveTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    EXPECT_EQ(scoreboard.getTeams().size(), 1u);

    // 移除队伍
    scoreboard.removeTeam(*team);

    // 验证队伍已移除
    EXPECT_EQ(scoreboard.getTeams().size(), 0u);
    EXPECT_FALSE(scoreboard.hasTeam("red"));
}

TEST_F(TeamCommandTest, RemoveNonExistentTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 尝试移除不存在的队伍
    auto* team = scoreboard.getTeam("nonexistent");
    EXPECT_EQ(team, nullptr);
}

// ========== listTeams 测试 ==========

TEST_F(TeamCommandTest, ListEmptyTeams)
{
    auto& scoreboard = m_server.scoreboard();

    // 初始应该没有队伍
    EXPECT_EQ(scoreboard.getTeams().size(), 0u);
}

TEST_F(TeamCommandTest, ListMultipleTeams)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建多个队伍
    scoreboard.createTeam("red");
    scoreboard.createTeam("blue");
    scoreboard.createTeam("green");

    // 验证队伍数量
    EXPECT_EQ(scoreboard.getTeams().size(), 3u);
}

// ========== joinTeam/leaveTeam 测试 ==========

TEST_F(TeamCommandTest, JoinTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 添加玩家到队伍
    EXPECT_TRUE(scoreboard.addPlayerToTeam("Steve", *team));

    // 验证玩家在队伍中
    EXPECT_TRUE(team->hasMember("Steve"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), team);
}

TEST_F(TeamCommandTest, JoinTeamSwitchesTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建两个队伍
    auto* redTeam = scoreboard.createTeam("red");
    auto* blueTeam = scoreboard.createTeam("blue");
    ASSERT_NE(redTeam, nullptr);
    ASSERT_NE(blueTeam, nullptr);

    // 添加玩家到 red 队伍
    scoreboard.addPlayerToTeam("Steve", *redTeam);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), redTeam);

    // 切换到 blue 队伍
    EXPECT_TRUE(scoreboard.addPlayerToTeam("Steve", *blueTeam));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), blueTeam);
    EXPECT_FALSE(redTeam->hasMember("Steve"));
}

TEST_F(TeamCommandTest, LeaveTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍并添加玩家
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    scoreboard.addPlayerToTeam("Steve", *team);

    // 移除玩家
    EXPECT_TRUE(scoreboard.removePlayerFromTeam("Steve", *team));
    EXPECT_FALSE(team->hasMember("Steve"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
}

// ========== emptyTeam 测试 ==========

TEST_F(TeamCommandTest, EmptyTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍并添加多个玩家
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    scoreboard.addPlayerToTeam("Steve", *team);
    scoreboard.addPlayerToTeam("Alex", *team);
    scoreboard.addPlayerToTeam("Bob", *team);

    EXPECT_EQ(team->getMembers().size(), 3u);

    // 清空队伍
    for (const auto& member : team->getMembers()) {
        scoreboard.removePlayerFromTeam(member, *team);
    }

    EXPECT_EQ(team->getMembers().size(), 0u);
}

// ========== modifyTeam 测试 ==========

TEST_F(TeamCommandTest, ModifyTeamColor)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置颜色
    team->setColor(text::TextFormatting::Red);
    EXPECT_EQ(team->getColor(), text::TextFormatting::Red);
}

TEST_F(TeamCommandTest, ModifyTeamFriendlyFire)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认允许友军伤害
    EXPECT_TRUE(team->getAllowFriendlyFire());

    // 禁用友军伤害
    team->setAllowFriendlyFire(false);
    EXPECT_FALSE(team->getAllowFriendlyFire());
}

TEST_F(TeamCommandTest, ModifyTeamSeeFriendlyInvisibles)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认可以看到隐身队友
    EXPECT_TRUE(team->canSeeFriendlyInvisibles());

    // 禁用看到隐身队友
    team->setSeeFriendlyInvisibles(false);
    EXPECT_FALSE(team->canSeeFriendlyInvisibles());
}

TEST_F(TeamCommandTest, ModifyTeamPrefixSuffix)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置前缀和后缀
    team->setPrefix(std::make_unique<text::StringTextComponent>("[Red] "));
    team->setSuffix(std::make_unique<text::StringTextComponent>(" [R]"));

    EXPECT_NE(team->getPrefix(), nullptr);
    EXPECT_NE(team->getSuffix(), nullptr);
}

TEST_F(TeamCommandTest, ModifyTeamVisibility)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置名称标签可见性
    team->setNameTagVisibility(scoreboard::TeamVisibility::HideForOtherTeams);
    EXPECT_EQ(team->getNameTagVisibility(), scoreboard::TeamVisibility::HideForOtherTeams);

    // 设置死亡消息可见性
    team->setDeathMessageVisibility(scoreboard::TeamVisibility::Never);
    EXPECT_EQ(team->getDeathMessageVisibility(), scoreboard::TeamVisibility::Never);
}

TEST_F(TeamCommandTest, ModifyTeamCollisionRule)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置碰撞规则
    team->setCollisionRule(scoreboard::TeamCollisionRule::PushOwnTeam);
    EXPECT_EQ(team->getCollisionRule(), scoreboard::TeamCollisionRule::PushOwnTeam);
}

} // namespace mc::command
