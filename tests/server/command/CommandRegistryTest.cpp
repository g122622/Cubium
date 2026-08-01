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
#include "common/entity/core/Entity.hpp"
#include "common/item/Items.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

namespace mc::command {
namespace {

// 测试服务器所需的服务端类型位于 mc::server 命名空间，此处引入以便在
// mc::command 命名空间内直接引用（与 EntityResolverTest.cpp 的 using namespace
// mc::server 等价，但作用域更窄）。
using mc::ServerDimension;
using mc::ServerDimensionManager;
using mc::server::ServerChunkManager;
using mc::server::ServerPlayerEntityManager;
using mc::server::ServerWorld;
using mc::server::ServerWorldConfig;

class FakeServer final : public test::BaseTestServer {
public:
    FakeServer()
        : BaseTestServer()
        , m_playerEntityManager()
    {
        // 初始化方块和物品注册表（createTestWorld 依赖方块注册表）
        VanillaBlocks::initialize();
        Items::initialize();

        // 创建测试世界
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = 12345;

        auto worldRaw = createTestWorld(config);
        m_world = worldRaw.get(); // 保存裸指针（在 move 之前）

        // 创建维度并关联测试世界
        auto dimension = std::make_unique<ServerDimension>(0, // DimensionId::OVERWORLD
            DimensionType::overworld(),
            nullptr, // 无区块生成器（维度仅作为世界容器）
            12345,   // seed
            10       // viewDistance
        );
        dimension->setWorld(std::move(worldRaw));
        m_dimension = dimension.get();
        bool registered = m_dimensionManager.registerDimension(std::move(dimension));
        (void)registered;
    }

    ~FakeServer() override = default;

    // 覆盖 dimensionManager，返回包含测试世界的维度管理器
    // 注意：DimensionManager 是 ServerDimensionManager 的基类，
    // 我们将 DimensionManager reinterpret_cast 为 ServerDimensionManager，
    // 因为 ServerDimensionManager::getDimension() 仅调用基类 DimensionManager::getDimension()
    // 然后做 static_cast，在我们的测试场景中是安全的。
    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return reinterpret_cast<ServerDimensionManager&>(m_dimensionManager);
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return reinterpret_cast<const ServerDimensionManager&>(m_dimensionManager);
    }

    // 覆盖 playerEntityManager，返回测试用实体管理器
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }

    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

    // 覆盖 getPlayerWorld，返回测试世界
    [[nodiscard]] ServerWorld* getPlayerWorld(PlayerId) override { return m_world; }

    // 获取测试世界（供测试用例生成实体等使用）
    [[nodiscard]] ServerWorld* world() const { return m_world; }

    /**
     * @brief 在测试世界中生成一个 id 等于 playerId 的 ServerPlayer 实体。
     *
     * SpectateCommand 会通过 `world->getEntity(static_cast<EntityInstanceId>(spectatorId))`
     * 查找旁观者实体，并要求该实体能 dynamic_cast 为 ServerPlayer。addTestPlayer
     * 仅在 PlayerManager 注册玩家数据，不会在世界中创建实体，因此旁观相关测试需要
     * 额外调用本方法补足实体。
     *
     * @param playerId 玩家 ID，同时作为实体 ID（与 SpectateCommand 的查找键一致）。
     * @param username 玩家名。
     * @return 生成的 ServerPlayer 指针；生成失败返回 nullptr。
     */
    ServerPlayer* spawnTestPlayerEntity(PlayerId playerId, const std::string& username)
    {
        auto player = std::make_unique<ServerPlayer>(static_cast<EntityInstanceId>(playerId), username);
        player->setPlayerId(playerId);
        auto* raw = player.get();
        m_world->spawnEntity(std::move(player));
        return raw;
    }

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
    // 构造测试世界：创建带噪声区块生成器的 ServerWorld
    static std::unique_ptr<ServerWorld> createTestWorld(const ServerWorldConfig& config)
    {
        auto world = std::make_unique<ServerWorld>(config);
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));
        return world;
    }

    DimensionManager m_dimensionManager;
    ServerDimension* m_dimension = nullptr;
    ServerPlayerEntityManager m_playerEntityManager;
    ServerWorld* m_world = nullptr;
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
    // 批5b：broadcastServerMessage 纯虚已删，SayCommand 改走 spdlog::info 直接打日志，
    // 不再经 IServer 出站。此处仅验证命令执行成功（/say hello world 解析通过）。
    const auto result = m_server.commandRegistry().execute("say hello world", m_console);

    ASSERT_TRUE(result.success());
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

    // 批5b：粒子改经 connectionManager().broadcast(buildLevelParticlesIr(...)) 投递，
    // 原 IServer 弱类型 broadcastParticleInRange 纯虚已删。FakeServerConnection::send
    // 仅累积字节、不还原包内容，故此处断言"向在线玩家 Steve 发了出站包"即可，不再
    // 校验粒子类型/位置/数量（包构造正确性由真客户端验证覆盖）。
    const auto connection = m_server.lastConnection();
    ASSERT_NE(connection, nullptr);
    EXPECT_GT(connection->sentBytes(), static_cast<size_t>(0));
}

TEST_F(CommandRegistryServerTest, ParticleCommandBroadcastsParticleAtSpecifiedPosition)
{
    const auto result = m_server.commandRegistry().execute("particle smoke 50.5 70.0 -100.5", m_console);

    // console 源无在线玩家，connectionManager().broadcast 为 no-op，仅验证命令成功。
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(CommandRegistryServerTest, ParticleCommandRejectsUnknownParticleType)
{
    const auto result = m_server.commandRegistry().execute("particle unknown_particle", m_console);

    // 未知粒子类型返回 0（失败）但命令本身执行成功
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CommandRegistryServerTest, ParticleCommandAcceptsMinecraftNamespace)
{
    const auto result = m_server.commandRegistry().execute("particle minecraft:lava 0 0 0", m_console);

    // console 源无在线玩家，仅验证命令成功且 minecraft: 前缀被正确剥离解析。
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
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

    // SpectateCommand._startSpectating 通过 world->getEntity(playerId) 查找旁观者实体
    // 并 dynamic_cast 为 ServerPlayer，因此需要生成 id==playerId 的 ServerPlayer 实体。
    // 注意：不能同时用 playerEntityManager().createPlayerEntity 注册第二个玩家作为目标——
    // EntityManager::allocateId 会从 1 开始自增，与手动指定的 id==1 实体冲突并覆盖。
    // 因此旁观目标也以显式 EntityInstanceId 生成（此处使用 1000 避免冲突）。
    auto* steveEntity = m_server.spawnTestPlayerEntity(1, "Steve");
    ASSERT_NE(steveEntity, nullptr);

    // 生成旁观目标实体（普通 Player 即可，SpectateCommand 仅要求目标是 Entity*）。
    // 使用 @e[name=Alex,limit=1] 按名称选中目标，避免 “Cannot spectate yourself”。
    auto targetEntity = std::make_unique<Player>(EntityInstanceId(1000), "Alex");
    targetEntity->setPosition(5.0f, 64.0f, 0.0f);
    m_server.world()->spawnEntity(std::move(targetEntity));

    auto playerSource = makePlayerSource(1, "Steve");

    // 执行 spectate 命令，目标为按名称选中的 Alex 实体
    const auto result = m_server.commandRegistry().execute("spectate @e[name=Alex,limit=1]", playerSource);

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

    // SpectateCommand._stopSpectating 会通过 world->getEntity(playerId) 查找实体并
    // dynamic_cast 为 ServerPlayer，因此需要在测试世界中生成对应实体。
    auto* steveEntity = m_server.spawnTestPlayerEntity(1, "Steve");
    ASSERT_NE(steveEntity, nullptr);

    auto playerSource = makePlayerSource(1, "Steve");

    // 执行 spectate stop 命令
    const auto result = m_server.commandRegistry().execute("spectate stop", playerSource);

    // 命令执行成功
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

} // namespace
} // namespace mc::command
