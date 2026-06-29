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

#include "common/BaseTestServer.hpp"
#include "common/item/Items.hpp"
#include "common/sound/SoundCategory.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {
namespace {
class FakeServer final : public test::BaseTestServer {
public:
    FakeServer() { Items::initialize(); }

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
        MC_UNUSED(velocityX);
        MC_UNUSED(velocityY);
        MC_UNUSED(velocityZ);
        MC_UNUSED(offsetX);
        MC_UNUSED(offsetY);
        MC_UNUSED(offsetZ);
        MC_UNUSED(range);
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
    using test::BaseTestServer::lastConnection;
    using test::BaseTestServer::stopRequested;

private:
    bool m_particleBroadcastCalled = false;
    u32 m_lastParticleType = 0;
    f64 m_lastParticleX = 0.0;
    f64 m_lastParticleY = 0.0;
    f64 m_lastParticleZ = 0.0;
    u32 m_lastParticleCount = 0;
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
            0,
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

    // 验证粒子类型 - flame = 32 (ParticleTypeId::Flame, MC 1.21.11 协议 ID)
    EXPECT_EQ(m_server.lastParticleType(), 32u);

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

    // 验证粒子类型 - smoke = 60 (ParticleTypeId::Smoke, MC 1.21.11 协议 ID)
    EXPECT_EQ(m_server.lastParticleType(), 60u);

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

    // 验证粒子类型 - lava = 54 (ParticleTypeId::Lava, MC 1.21.11 协议 ID)
    EXPECT_EQ(m_server.lastParticleType(), 54u);
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
