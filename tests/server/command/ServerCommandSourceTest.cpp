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
 * The above copyright notice shall be included in all
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
 * @file ServerCommandSourceTest.cpp
 * @brief ServerCommandSource 单元测试
 *
 * 测试 entity()/entityOrException()/withEntity() 等实体相关方法，
 * 覆盖控制台源、玩家源、非玩家实体源等场景。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
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

using namespace mc;
using namespace mc::command;
using namespace mc::entity;
using namespace mc::server;

namespace {

// ============================================================================
// 测试服务器 — 扩展 BaseTestServer，提供维度管理器和世界支持
// ============================================================================

class ServerCommandSourceTestServer final : public mc::test::BaseTestServer {
public:
    ServerCommandSourceTestServer()
        : BaseTestServer()
        , m_playerEntityManager()
    {
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();

        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = 12345;

        auto worldRaw = createTestWorld(config);
        m_world = worldRaw.get();

        auto dimension = std::make_unique<ServerDimension>(0, DimensionType::overworld(), nullptr, 12345, 10);
        dimension->setWorld(std::move(worldRaw));
        m_dimension = dimension.get();
        bool registered = m_dimensionManager.registerDimension(std::move(dimension));
        (void)registered;
    }

    ~ServerCommandSourceTestServer() override = default;

    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return m_dimensionManager;
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return m_dimensionManager;
    }

    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

    [[nodiscard]] ServerWorld* getPlayerWorld(PlayerId) override { return m_world; }
    [[nodiscard]] ServerWorld* world() const { return m_world; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity)
    {
        if (!m_world) {
            return 0;
        }
        return m_world->spawnEntity(std::move(entity));
    }

private:
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

    // 真实 ServerDimensionManager（nullptr 构造：仅用于 getPlayerDimension 等 map 查询，不调
    // initialize 故不解引用内部 m_server；RelWithDebInfo 下构造断言 MC_ASSERT(server!=nullptr) 不生效）。
    // 替代旧 reinterpret_cast<ServerDimensionManager&>(基类DimensionManager) UB——派生类独有
    // m_playerDimensions 越界读基类内存致 TeleportCommand::teleportPlayers 调 getPlayerDimension 时 SEH。
    ServerDimensionManager m_dimensionManager{nullptr};
    ServerDimension* m_dimension = nullptr;
    ServerPlayerEntityManager m_playerEntityManager;
    ServerWorld* m_world = nullptr;
};

// ============================================================================
// 辅助函数
// ============================================================================

std::unique_ptr<Entity> createEntityByType(const char* typeId)
{
    const EntityType* type = EntityRegistry::instance().getType(typeId);
    if (type == nullptr) {
        return nullptr;
    }
    return type->create(nullptr, mc::test::testEcsRegistry());
}

} // namespace

// ============================================================================
// 测试固件
// ============================================================================

class ServerCommandSourceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
    }

    ServerCommandSourceTestServer m_server;
};

// ============================================================================
// 1. entity() 基本测试
// ============================================================================

TEST_F(ServerCommandSourceTest, EntityReturnsNullptrForConsoleSource)
{
    // 控制台命令源没有关联实体
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EXPECT_EQ(source.entity(), nullptr);
}

TEST_F(ServerCommandSourceTest, EntityReturnsPlayerPointerWhenConstructedWithPlayer)
{
    // 使用 ServerPlayer 构造命令源时，entity() 应返回指向该玩家的 Entity 指针
    auto serverPlayer =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource source(&m_server, playerPtr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 42, "TestPlayer");

    // entity() 应该与 player() 指向同一个对象（因为构造函数中 entity 默认为 nullptr，
    // 会回退到 static_cast<Entity*>(player)）
    EXPECT_EQ(source.entity(), static_cast<Entity*>(playerPtr));
    EXPECT_EQ(source.player(), playerPtr);
}

TEST_F(ServerCommandSourceTest, EntityReturnsExplicitEntityWhenProvided)
{
    // 使用显式 entity 参数构造命令源
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "", pigPtr);

    EXPECT_EQ(source.entity(), pigPtr);
    EXPECT_EQ(source.player(), nullptr);
}

// ============================================================================
// 2. entityOrException() 测试
// ============================================================================

TEST_F(ServerCommandSourceTest, EntityOrExceptionThrowsWhenEntityIsNullptr)
{
    // 控制台命令源调用 entityOrException() 应抛出 CommandException
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);

    EXPECT_THROW({ source.entityOrException(); }, CommandException);
}

TEST_F(ServerCommandSourceTest, EntityOrExceptionThrowsCorrectErrorType)
{
    // 验证抛出的异常类型和消息
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);

    try {
        source.entityOrException();
        FAIL() << "Expected CommandException";
    }
    catch (const CommandException& e) {
        EXPECT_EQ(e.type(), CommandErrorType::PermissionDenied);
        EXPECT_EQ(e.message(), "commands.requires.entity");
    }
}

TEST_F(ServerCommandSourceTest, EntityOrExceptionReturnsReferenceWhenEntityIsSet)
{
    // 玩家命令源调用 entityOrException() 应返回实体引用
    auto serverPlayer =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource source(&m_server, playerPtr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 42, "TestPlayer");

    Entity& entity = source.entityOrException();
    EXPECT_EQ(&entity, static_cast<Entity*>(playerPtr));
}

TEST_F(ServerCommandSourceTest, EntityOrExceptionReturnsNonPlayerEntityReference)
{
    // 非玩家实体命令源调用 entityOrException() 应返回该实体引用
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "", pigPtr);

    Entity& entity = source.entityOrException();
    EXPECT_EQ(&entity, pigPtr);
}

// ============================================================================
// 3. withEntity() 测试
// ============================================================================

TEST_F(ServerCommandSourceTest, WithEntitySetsEntityPointer)
{
    // withEntity() 应更新 m_entity
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EXPECT_EQ(source.entity(), nullptr);

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource modified = source.withEntity(*pigPtr);
    EXPECT_EQ(modified.entity(), pigPtr);
}

TEST_F(ServerCommandSourceTest, WithEntityWithServerPlayerUpdatesPlayerAndPlayerId)
{
    // withEntity() 传入 ServerPlayer 时，应同时更新 m_player 和 m_playerId
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EXPECT_EQ(source.player(), nullptr);
    EXPECT_EQ(source.playerId(), 0);

    auto serverPlayer = std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "Alice", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource modified = source.withEntity(*playerPtr);
    EXPECT_EQ(modified.entity(), static_cast<Entity*>(playerPtr));
    EXPECT_EQ(modified.player(), playerPtr);
    EXPECT_EQ(modified.playerId(), 42);
}

TEST_F(ServerCommandSourceTest, WithEntityWithServerPlayerUpdatesName)
{
    // withEntity() 传入 ServerPlayer 时，名称应为玩家用户名
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EXPECT_EQ(source.name(), "Console");

    auto serverPlayer = std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "Bob", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(7);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource modified = source.withEntity(*playerPtr);
    EXPECT_EQ(modified.name(), "Bob");
}

TEST_F(ServerCommandSourceTest, WithEntityWithNonPlayerEntityPreservesPlayer)
{
    // withEntity() 传入非玩家实体时，m_player 和 m_playerId 应保持不变
    auto serverPlayer = std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "Alice", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource source(&m_server, playerPtr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 42, "Alice");
    EXPECT_EQ(source.player(), playerPtr);
    EXPECT_EQ(source.playerId(), 42);

    // 使用非玩家实体调用 withEntity()
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource modified = source.withEntity(*pigPtr);
    // entity 应该指向猪，但 player 和 playerId 保持不变
    EXPECT_EQ(modified.entity(), pigPtr);
    EXPECT_EQ(modified.player(), playerPtr);
    EXPECT_EQ(modified.playerId(), 42);
}

TEST_F(ServerCommandSourceTest, WithEntityWithCustomNamedEntityUpdatesName)
{
    // withEntity() 传入有自定义名称的非玩家实体时，名称应为自定义名称
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setCustomName("Piggy");
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource modified = source.withEntity(*pigPtr);
    EXPECT_EQ(modified.name(), "Piggy");
}

TEST_F(ServerCommandSourceTest, WithEntityWithUnnamedEntityUsesTypeId)
{
    // withEntity() 传入没有自定义名称的非玩家实体时，名称应为类型ID
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource modified = source.withEntity(*pigPtr);
    EXPECT_EQ(modified.name(), "minecraft:pig");
}

TEST_F(ServerCommandSourceTest, WithEntityPreservesOtherFields)
{
    // withEntity() 不应改变位置、旋转、维度、权限等级等字段
    auto serverPlayer =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(1);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource source(
        &m_server, playerPtr, 0, Vector3d(10.0, 64.0, 20.0), Vector2f(90.0f, 45.0f), 3, 1, "TestPlayer");

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource modified = source.withEntity(*pigPtr);

    // 位置、旋转、维度、权限应保持不变
    EXPECT_EQ(modified.dimensionId(), 0);
    EXPECT_DOUBLE_EQ(modified.position().x, 10.0);
    EXPECT_DOUBLE_EQ(modified.position().y, 64.0);
    EXPECT_DOUBLE_EQ(modified.position().z, 20.0);
    EXPECT_FLOAT_EQ(modified.rotation().x, 90.0f);
    EXPECT_FLOAT_EQ(modified.rotation().y, 45.0f);
    EXPECT_EQ(modified.permissionLevel(), 3);
    EXPECT_EQ(modified.server(), &m_server);
}

TEST_F(ServerCommandSourceTest, WithEntityDoesNotModifyOriginalSource)
{
    // withEntity() 应返回新对象，不修改原始 source
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EXPECT_EQ(source.entity(), nullptr);
    EXPECT_EQ(source.name(), "Console");

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource modified = source.withEntity(*pigPtr);

    // 原始 source 应不受影响
    EXPECT_EQ(source.entity(), nullptr);
    EXPECT_EQ(source.name(), "Console");
    // 新 source 应已更新
    EXPECT_EQ(modified.entity(), pigPtr);
    EXPECT_EQ(modified.name(), "minecraft:pig");
}

// ============================================================================
// 4. withPlayer() 同时更新 entity 测试
// ============================================================================

TEST_F(ServerCommandSourceTest, WithPlayerUpdatesEntityPointer)
{
    // withPlayer() 应同时更新 m_entity 指针
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EXPECT_EQ(source.entity(), nullptr);
    EXPECT_EQ(source.player(), nullptr);

    auto serverPlayer =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource modified = source.withPlayer(playerPtr);
    EXPECT_EQ(modified.player(), playerPtr);
    EXPECT_EQ(modified.entity(), static_cast<Entity*>(playerPtr));
}

TEST_F(ServerCommandSourceTest, WithPlayerWithNullptrClearsEntity)
{
    // withPlayer(nullptr) 应同时清除 m_entity
    auto serverPlayer =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource source(&m_server, playerPtr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 42, "TestPlayer");
    EXPECT_EQ(source.entity(), static_cast<Entity*>(playerPtr));

    ServerCommandSource modified = source.withPlayer(nullptr);
    EXPECT_EQ(modified.player(), nullptr);
    EXPECT_EQ(modified.entity(), nullptr);
}

// ============================================================================
// 5. 构造函数 entity 参数默认行为测试
// ============================================================================

TEST_F(ServerCommandSourceTest, ConstructorDefaultsEntityToPlayerWhenEntityIsNullptr)
{
    // 当 entity 参数为 nullptr 且 player 非 nullptr 时，entity 应默认为 static_cast<Entity*>(player)
    auto serverPlayer =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    // 不传 entity 参数（默认 nullptr）
    ServerCommandSource source(&m_server, playerPtr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 42, "TestPlayer");

    EXPECT_EQ(source.entity(), static_cast<Entity*>(playerPtr));
    EXPECT_EQ(source.player(), playerPtr);
}

TEST_F(ServerCommandSourceTest, ConstructorWithExplicitEntityOverridesPlayerDefault)
{
    // 当显式传入 entity 参数时，应使用传入的 entity 而非 player
    auto serverPlayer =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    // 传入 player 同时传入一个不同的 entity
    ServerCommandSource source(&m_server, playerPtr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 42, "TestPlayer", pigPtr);

    EXPECT_EQ(source.entity(), pigPtr);
    EXPECT_EQ(source.player(), playerPtr);
}

TEST_F(ServerCommandSourceTest, ConstructorWithNoPlayerAndNoEntityHasNullptrEntity)
{
    // forConsole() 创建的命令源 entity 和 player 都为 nullptr
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EXPECT_EQ(source.entity(), nullptr);
    EXPECT_EQ(source.player(), nullptr);
}

// ============================================================================
// 6. 综合场景测试
// ============================================================================

TEST_F(ServerCommandSourceTest, ExecuteAsNonPlayerEntityScenario)
{
    // 模拟 /execute as @e[type=pig] 场景：
    // 原始源是玩家，withEntity() 后 entity 指向猪，但 player 仍指向原始玩家
    auto serverPlayer = std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "Alice", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource playerSource(&m_server, playerPtr, 0, Vector3d(10, 64, 10), Vector2f(0, 0), 2, 42, "Alice");

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(50.0f, 64.0f, 50.0f);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    // /execute as @e[type=pig] run ...
    ServerCommandSource asSource = playerSource.withEntity(*pigPtr);

    // entity 指向猪
    EXPECT_EQ(asSource.entity(), pigPtr);
    // player 保持原始玩家
    EXPECT_EQ(asSource.player(), playerPtr);
    // 名称应为猪的类型 ID（无自定义名称时）
    EXPECT_EQ(asSource.name(), "minecraft:pig");
    // 位置、旋转、维度保持不变（withEntity 不修改位置）
    EXPECT_DOUBLE_EQ(asSource.position().x, 10.0);
    EXPECT_DOUBLE_EQ(asSource.position().y, 64.0);
    EXPECT_DOUBLE_EQ(asSource.position().z, 10.0);
}

TEST_F(ServerCommandSourceTest, ChainedWithEntityAndWithPosition)
{
    // 模拟 /execute as @e[type=pig] at @s run ...
    // 先 withEntity()，再 withPosition()
    auto serverPlayer = std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "Alice", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    ServerCommandSource playerSource(&m_server, playerPtr, 0, Vector3d(10, 64, 10), Vector2f(0, 0), 2, 42, "Alice");

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource asSource = playerSource.withEntity(*pigPtr);
    ServerCommandSource atSource = asSource.withPosition(Vector3d(50.0, 64.0, 50.0));

    // entity 仍指向猪
    EXPECT_EQ(atSource.entity(), pigPtr);
    // player 仍指向原始玩家
    EXPECT_EQ(atSource.player(), playerPtr);
    // 位置已更新
    EXPECT_DOUBLE_EQ(atSource.position().x, 50.0);
    EXPECT_DOUBLE_EQ(atSource.position().y, 64.0);
    EXPECT_DOUBLE_EQ(atSource.position().z, 50.0);
}

TEST_F(ServerCommandSourceTest, WithEntityThenWithPlayerBackToPlayerEntity)
{
    // 先 withEntity(非玩家实体)，然后 withPlayer(玩家) 切回玩家
    auto serverPlayer = std::make_unique<mc::ServerPlayer>(EntityInstanceId(1), "Alice", mc::test::testEcsRegistry());
    serverPlayer->setPlayerId(42);
    auto* playerPtr = serverPlayer.get();
    m_server.spawnEntity(std::move(serverPlayer));

    // 验证玩家用户名设置正确
    EXPECT_EQ(playerPtr->username(), "Alice");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);

    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    Entity* pigPtr = pig.get();
    m_server.spawnEntity(std::move(pig));

    // 先设置为猪
    ServerCommandSource withPig = source.withEntity(*pigPtr);
    EXPECT_EQ(withPig.entity(), pigPtr);
    EXPECT_EQ(withPig.player(), nullptr);

    // 再切回玩家
    ServerCommandSource withPlayer = withPig.withPlayer(playerPtr);
    EXPECT_EQ(withPlayer.entity(), static_cast<Entity*>(playerPtr));
    EXPECT_EQ(withPlayer.player(), playerPtr);
    EXPECT_EQ(withPlayer.playerId(), 42);
    // withPlayer(nullptr) 时不清空名称，但 withPlayer(non-null) 应更新为玩家用户名
    EXPECT_EQ(withPlayer.name(), playerPtr->username());
}
