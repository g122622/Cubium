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
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file EntityResolverTest.cpp
 * @brief EntityResolver 单元测试
 *
 * 测试实体选择器的解析和过滤功能，覆盖所有选择器类型和过滤条件。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

using namespace mc;
using namespace mc::command;
using namespace mc::command::support;
using namespace mc::entity;
using namespace mc::server;

namespace {

// ============================================================================
// 测试服务器 — 扩展 BaseTestServer，提供维度管理器和世界支持
// ============================================================================

class EntityResolverTestServer final : public test::BaseTestServer {
public:
    EntityResolverTestServer()
        : BaseTestServer()
        , m_playerEntityManager()
    {
        // 初始化方块和实体注册表
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();

        // 创建测试世界
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = 12345;

        auto worldRaw = createTestWorld(config);
        m_world = worldRaw.get(); // 保存裸指针（在 move 之前）

        // 创建维度并关联世界
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

    ~EntityResolverTestServer() override = default;

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

    // 覆盖 playerEntityManager
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }

    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

    // 覆盖 getPlayerWorld，返回测试世界
    [[nodiscard]] ServerWorld* getPlayerWorld(PlayerId) override { return m_world; }

    // 获取测试世界
    [[nodiscard]] ServerWorld* world() const { return m_world; }

    // 在测试世界中生成实体
    EntityId spawnEntity(std::unique_ptr<Entity> entity)
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
        auto generator = std::make_unique<NoiseChunkGenerator>(config.seed,
            DimensionSettings::overworld(),
            world::biome::source::MultiNoiseBiomeSource::createOverworld(config.seed, false));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));
        return world;
    }

    DimensionManager m_dimensionManager;
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
    return type->create(nullptr);
}

} // namespace

// ============================================================================
// 测试固件
// ============================================================================

class EntityResolverTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
    }

    EntityResolverTestServer m_server;
};

// ============================================================================
// 1. 选择器类型基本解析测试
// ============================================================================

TEST_F(EntityResolverTest, SelfSelectorWithNoPlayerReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::self();

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, SinglePlayerSelectorWithNoPlayersReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::SinglePlayer);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, AllPlayersSelectorWithNoPlayersReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, AllEntitiesSelectorWithNoEntitiesReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, RandomPlayerSelectorWithNoPlayersReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::randomPlayer();

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, NullServerReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(nullptr);
    EntitySelector selector = EntitySelector::allEntities();

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, ResolveSingleWithNoMatchReturnsNullptr)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();

    auto* entity = EntityResolver::resolveSingle(source, selector);
    EXPECT_EQ(entity, nullptr);
}

TEST_F(EntityResolverTest, NearestPlayerSelectorWithNoPlayersReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::nearestPlayer();

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// 2. type= 过滤测试（正向、反向、命名空间兼容）
// ============================================================================

TEST_F(EntityResolverTest, TypeFilterMatchesExactType)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setEntityType("minecraft:zombie");

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:zombie"));
}

TEST_F(EntityResolverTest, TypeFilterMatchesWithoutNamespace)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setEntityType("zombie"); // 不带命名空间前缀

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, TypeFilterNegatedExcludesType)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setEntityType("minecraft:zombie", true); // type=!minecraft:zombie

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:pig"));
}

TEST_F(EntityResolverTest, TypeFilterNegatedWithoutNamespace)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setEntityType("zombie", true); // type=!zombie

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:pig"));
}

TEST_F(EntityResolverTest, TypeFilterNoMatchReturnsEmpty)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setEntityType("minecraft:creeper"); // 不存在的类型

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// 3. tag= 过滤测试（正向、反向）
// ============================================================================

TEST_F(EntityResolverTest, TagFilterMatchesEntityWithTag)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("test_tag");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.addTag("test_tag");

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:pig"));
}

TEST_F(EntityResolverTest, TagFilterNegatedExcludesEntityWithTag)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("excluded");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.addTag("excluded", true); // tag=!excluded

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:zombie"));
}

TEST_F(EntityResolverTest, TagFilterMultipleTagsAllRequired)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("tag_a");
    pig->addTag("tag_b");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    zombie->addTag("tag_a");
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.addTag("tag_a");
    selector.addTag("tag_b");

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:pig"));
}

TEST_F(EntityResolverTest, TagFilterMixedPositiveAndNegative)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("included");
    pig->addTag("excluded");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    zombie->addTag("included");
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.addTag("included", false); // tag=included
    selector.addTag("excluded", true);  // tag=!excluded

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:zombie"));
}

// ============================================================================
// 4. distance/dx/dy/dz 空间过滤测试（含边界值和 MC 原版 +1.0 行为）
// ============================================================================

TEST_F(EntityResolverTest, DistanceFilterBasicRange)
{
    auto near = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(near, nullptr);
    near->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(near));

    auto mid = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(mid, nullptr);
    mid->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(mid));

    auto far = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(far, nullptr);
    far->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(far));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.distance().setMin(10.0f);
    selector.distance().setMax(20.0f);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 15.0f);
}

TEST_F(EntityResolverTest, DistanceFilterExactBoundary)
{
    auto entity = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(10.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.distance().setMin(10.0f);
    selector.distance().setMax(10.0f);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, DistanceFilterOutsideRange)
{
    auto entity = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.distance().setMin(10.0f);
    selector.distance().setMax(20.0f);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, VolumeFilterDxDyDz)
{
    auto inside = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(inside, nullptr);
    inside->setPosition(5.0f, 5.0f, 5.0f);
    m_server.spawnEntity(std::move(inside));

    auto outside = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(outside, nullptr);
    outside->setPosition(15.0f, 5.0f, 5.0f);
    m_server.spawnEntity(std::move(outside));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setDx(10.0f);
    selector.setDy(10.0f);
    selector.setDz(10.0f);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 5.0f);
}

TEST_F(EntityResolverTest, VolumeFilterNegativeDx)
{
    auto entity = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(3.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setX(5.0f);
    selector.setY(0.0f);
    selector.setZ(0.0f);
    selector.setDx(-10.0f); // 体积范围 x∈[5+(-10), 5+1] = [-5, 6]

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, VolumeFilterBoundaryInclusive)
{
    auto entity = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(10.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setDx(10.0f);
    // MC 原版行为：仅设置 dx 时，dy/dz 默认为 0，但 max 侧加 1.0
    // 因此 Y 范围为 [0, 1]，Z 范围为 [0, 1]，实体在 Y=0 应在范围内

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, VolumeFilterOnlyDxFindsEntityAtY0)
{
    // MC 原版行为验证：仅 dx 时，dy/dz 默认 delta=0
    // AABB Y 范围为 [refY, refY+1]，Z 范围为 [refZ, refZ+1]
    // 实体在原点 Y=0 应被选中
    auto entity = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(5.0f, 0.5f, 0.5f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setDx(10.0f); // 仅设置 dx，不设 dy/dz

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

// ============================================================================
// 5. sort= 排序测试
// ============================================================================

TEST_F(EntityResolverTest, SortNearest)
{
    auto near = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(near, nullptr);
    near->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(near));

    auto mid = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(mid, nullptr);
    mid->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(mid));

    auto far = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(far, nullptr);
    far->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(far));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setSort(EntitySelectorSort::Nearest);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 5.0f);
    EXPECT_FLOAT_EQ(result[1]->position().x, 15.0f);
    EXPECT_FLOAT_EQ(result[2]->position().x, 50.0f);
}

TEST_F(EntityResolverTest, SortFurthest)
{
    auto near = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(near, nullptr);
    near->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(near));

    auto mid = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(mid, nullptr);
    mid->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(mid));

    auto far = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(far, nullptr);
    far->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(far));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setSort(EntitySelectorSort::Furthest);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 50.0f);
    EXPECT_FLOAT_EQ(result[1]->position().x, 15.0f);
    EXPECT_FLOAT_EQ(result[2]->position().x, 5.0f);
}

TEST_F(EntityResolverTest, SortRandomReturnsAllEntities)
{
    for (int i = 0; i < 5; ++i) {
        auto pig = createEntityByType(EntityTypes::PIG);
        ASSERT_NE(pig, nullptr);
        pig->setPosition(static_cast<f32>(i * 10), 0.0f, 0.0f);
        m_server.spawnEntity(std::move(pig));
    }

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setSort(EntitySelectorSort::Random);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 5u);
}

TEST_F(EntityResolverTest, SortArbitraryReturnsAllEntities)
{
    for (int i = 0; i < 3; ++i) {
        auto pig = createEntityByType(EntityTypes::PIG);
        ASSERT_NE(pig, nullptr);
        pig->setPosition(static_cast<f32>(i * 10), 0.0f, 0.0f);
        m_server.spawnEntity(std::move(pig));
    }

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setSort(EntitySelectorSort::Arbitrary);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 3u);
}

// ============================================================================
// 6. limit= 数量限制测试
// ============================================================================

TEST_F(EntityResolverTest, LimitRestrictsResultCount)
{
    for (int i = 0; i < 10; ++i) {
        auto pig = createEntityByType(EntityTypes::PIG);
        ASSERT_NE(pig, nullptr);
        pig->setPosition(static_cast<f32>(i), 0.0f, 0.0f);
        m_server.spawnEntity(std::move(pig));
    }

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setLimit(3);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 3u);
}

TEST_F(EntityResolverTest, LimitWithSortNearest)
{
    for (int i = 0; i < 10; ++i) {
        auto pig = createEntityByType(EntityTypes::PIG);
        ASSERT_NE(pig, nullptr);
        pig->setPosition(static_cast<f32>(i * 10), 0.0f, 0.0f);
        m_server.spawnEntity(std::move(pig));
    }

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setSort(EntitySelectorSort::Nearest);
    selector.setLimit(3);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 0.0f);
    EXPECT_FLOAT_EQ(result[1]->position().x, 10.0f);
    EXPECT_FLOAT_EQ(result[2]->position().x, 20.0f);
}

TEST_F(EntityResolverTest, LimitZeroReturnsAll)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setLimit(0); // limit=0 表示不限制

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, LimitLargerThanResultReturnsAll)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setLimit(100);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, LimitOneWithSortFurthest)
{
    auto near = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(near, nullptr);
    near->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(near));

    auto far = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(far, nullptr);
    far->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(far));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setSort(EntitySelectorSort::Furthest);
    selector.setLimit(1);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:zombie"));
}

// ============================================================================
// 7. 空结果和边界场景
// ============================================================================

TEST_F(EntityResolverTest, EmptyWorldReturnsEmptyForAllSelectors)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);

    EXPECT_TRUE(EntityResolver::resolve(source, EntitySelector::allEntities()).empty());
    EXPECT_TRUE(EntityResolver::resolve(source, EntitySelector(EntitySelectorType::AllPlayers)).empty());
    EXPECT_TRUE(EntityResolver::resolve(source, EntitySelector::nearestPlayer()).empty());
    EXPECT_TRUE(EntityResolver::resolve(source, EntitySelector::randomPlayer()).empty());
    EXPECT_TRUE(EntityResolver::resolve(source, EntitySelector::self()).empty());
}

TEST_F(EntityResolverTest, ResolveSingleReturnsFirstEntity)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    EntityId pigId = m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(10.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setSort(EntitySelectorSort::Nearest);

    auto* entity = EntityResolver::resolveSingle(source, selector);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->id(), pigId);
}

TEST_F(EntityResolverTest, ResolveSingleReturnsNullptrWhenNoEntities)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();

    auto* entity = EntityResolver::resolveSingle(source, selector);
    EXPECT_EQ(entity, nullptr);
}

TEST_F(EntityResolverTest, MultipleEntityTypesWithCombinedFilters)
{
    auto pig1 = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig1, nullptr);
    pig1->setPosition(5.0f, 0.0f, 0.0f);
    pig1->addTag("friendly");
    m_server.spawnEntity(std::move(pig1));

    auto pig2 = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig2, nullptr);
    pig2->setPosition(10.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig2));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(15.0f, 0.0f, 0.0f);
    zombie->addTag("friendly");
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);

    // 查询 type=pig, tag=friendly
    EntitySelector selector1 = EntitySelector::allEntities();
    selector1.setEntityType("minecraft:pig");
    selector1.addTag("friendly");
    auto result1 = EntityResolver::resolve(source, selector1);
    ASSERT_EQ(result1.size(), 1u);
    EXPECT_EQ(result1[0]->getTypeId(), std::string("minecraft:pig"));

    // 查询 type=!zombie, tag=friendly
    EntitySelector selector2 = EntitySelector::allEntities();
    selector2.setEntityType("minecraft:zombie", true);
    selector2.addTag("friendly");
    auto result2 = EntityResolver::resolve(source, selector2);
    ASSERT_EQ(result2.size(), 1u);
    EXPECT_EQ(result2[0]->getTypeId(), std::string("minecraft:pig"));

    // 查询 distance=..12
    EntitySelector selector3 = EntitySelector::allEntities();
    selector3.distance().setMax(12.0f);
    selector3.setSort(EntitySelectorSort::Nearest);
    auto result3 = EntityResolver::resolve(source, selector3);
    EXPECT_EQ(result3.size(), 2u);
}

TEST_F(EntityResolverTest, ItemEntityType)
{
    // 通过 EntityRegistry 创建 ItemEntity，确保 typeId 被正确设置
    auto item = createEntityByType("minecraft:item");
    ASSERT_NE(item, nullptr);
    item->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(item));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setEntityType("minecraft:item");

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:item"));
}

TEST_F(EntityResolverTest, AllEntitiesIncludesAllTypes)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(10.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto item = createEntityByType("minecraft:item");
    ASSERT_NE(item, nullptr);
    item->setPosition(20.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(item));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 3u);
}

// ============================================================================
// 自定义位置测试
// ============================================================================

TEST_F(EntityResolverTest, CustomPositionOverridesSourcePosition)
{
    auto near = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(near, nullptr);
    near->setPosition(100.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(near));

    auto far = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(far, nullptr);
    far->setPosition(200.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(far));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setX(90.0f);
    selector.setY(0.0f);
    selector.setZ(0.0f);
    selector.setSort(EntitySelectorSort::Nearest);
    selector.setLimit(1);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 100.0f);
}

// ============================================================================
// 3D 距离测试
// ============================================================================

TEST_F(EntityResolverTest, DistanceFilter3D)
{
    auto entity = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(3.0f, 4.0f, 0.0f); // 距离原点 5.0 (3²+4²=25, sqrt=5)
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.distance().setMin(4.0f);
    selector.distance().setMax(6.0f);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, DistanceFilter3DOutside)
{
    auto entity = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(3.0f, 4.0f, 0.0f); // 距离原点 5.0
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.distance().setMin(6.0f);
    selector.distance().setMax(10.0f);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// 综合过滤测试
// ============================================================================

TEST_F(EntityResolverTest, CombinedTypeAndDistanceAndSortAndLimit)
{
    auto pig1 = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig1, nullptr);
    pig1->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig1));

    auto pig2 = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig2, nullptr);
    pig2->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig2));

    auto pig3 = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig3, nullptr);
    pig3->setPosition(25.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig3));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(3.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setEntityType("minecraft:pig");       // 只选择猪
    selector.distance().setMax(20.0f);             // 距离20以内
    selector.setSort(EntitySelectorSort::Nearest); // 按距离排序
    selector.setLimit(2);                          // 最多2个

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 5.0f);
    EXPECT_FLOAT_EQ(result[1]->position().x, 15.0f);
}

// ============================================================================
// name= 过滤测试
// ============================================================================

TEST_F(EntityResolverTest, NameFilterMatchesCustomName)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->setCustomName("TestPig");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setUsername("TestPig");

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:pig"));
}

TEST_F(EntityResolverTest, NameFilterNegatedExcludesNamedEntity)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->setCustomName("Excluded");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setUsernameNegated("Excluded");

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->getTypeId(), std::string("minecraft:zombie"));
}

TEST_F(EntityResolverTest, NameFilterNoMatchReturnsEmpty)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setUsername("NonExistentName");

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// team= 过滤测试
// ============================================================================

TEST_F(EntityResolverTest, TeamFilterNoTeamExcludedByTeamFilter)
{
    // 非玩家实体默认不在任何队伍中（getTeam() 返回 nullptr）
    // team=red 应该排除不在 red 队伍中的实体
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setTeam("red"); // team=red — 只选中 red 队伍中的实体

    auto result = EntityResolver::resolve(source, selector);
    // 非玩家实体不在任何队伍中，应该返回空
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, TeamFilterNegatedIncludesEntitiesWithoutTeam)
{
    // team=!red — 排除在 red 队伍中的实体
    // 没有队伍的实体不在 red 中，因此不应被排除
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setTeam("red", true); // team=!red

    auto result = EntityResolver::resolve(source, selector);
    // 所有实体都不在 red 队伍中，应全部返回
    EXPECT_EQ(result.size(), 2u);
}
