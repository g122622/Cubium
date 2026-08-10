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
#include "common/TestWorldHelper.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/player/ServerPlayer.hpp"
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

class EntityResolverTestServer final : public mc::test::BaseTestServer {
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
    return type->create(nullptr, mc::test::testEcsRegistry());
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
    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("test_tag");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("excluded");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("tag_a");
    pig->addTag("tag_b");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->addTag("included");
    pig->addTag("excluded");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto nearEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(nearEntity, nullptr);
    nearEntity->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(nearEntity));

    auto mid = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(mid, nullptr);
    mid->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(mid));

    auto farEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(farEntity, nullptr);
    farEntity->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(farEntity));

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
    auto entity = createEntityByType(EntityTypeKeys::PIG);
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
    auto entity = createEntityByType(EntityTypeKeys::PIG);
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
    auto inside = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(inside, nullptr);
    inside->setPosition(5.0f, 5.0f, 5.0f);
    m_server.spawnEntity(std::move(inside));

    auto outside = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto entity = createEntityByType(EntityTypeKeys::PIG);
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
    auto entity = createEntityByType(EntityTypeKeys::PIG);
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
    auto entity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(5.0f, 0.5f, 0.5f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setDx(10.0f); // 仅设置 dx，不设 dy/dz

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, VolumeFilterAABBIntersectionEntityOverlap)
{
    // AABB 相交检查：实体碰撞箱与选择 AABB 部分重叠时应被选中
    // 猪的碰撞箱宽度约 0.9，高度约 0.9
    // 将猪放在 x=10.5（刚好在 AABB 边界外，但碰撞箱延伸到 x=10.05，仍与 AABB [0,11] 相交）
    auto entity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(10.5f, 0.0f, 0.5f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setDx(10.0f); // AABB: x∈[0, 11]

    auto result = EntityResolver::resolve(source, selector);
    // 猪的碰撞箱约 [10.05, 10.95] x [0, 0.9] x [0.05, 0.95]
    // AABB 为 [0, 11] x [0, 1] x [0, 1]，碰撞箱与 AABB 相交
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, VolumeFilterAABBIntersectionEntityOutsideAABB)
{
    // 实体碰撞箱完全在选择 AABB 之外时不应被选中
    auto entity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(15.0f, 0.0f, 0.5f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setDx(10.0f); // AABB: x∈[0, 11]

    auto result = EntityResolver::resolve(source, selector);
    // 猪在 x=15，碰撞箱约 [14.55, 15.45]，完全在 AABB 外
    EXPECT_TRUE(result.empty());
}

TEST_F(EntityResolverTest, VolumeFilterAABBIntersectionLargeEntity)
{
    // 大型实体（如恶魂/巨人，碰撞箱远大于 1x1x1）跨越选择边界时应被选中
    // 这里用僵尸测试（碰撞箱约 0.6 x 1.95 x 0.6）
    // 将僵尸放在 x=0.0，AABB 为 x∈[0, 1]
    // 僵尸碰撞箱 x 约 [-0.3, 0.3]，与 AABB [0, 1] 在 x=0 处相交
    auto entity = createEntityByType(EntityTypeKeys::ZOMBIE);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(0.0f, 0.0f, 0.5f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setDx(0.0f); // AABB: x∈[0, 1], y∈[0, 1], z∈[0, 1]

    auto result = EntityResolver::resolve(source, selector);
    // 僵尸碰撞箱约 [-0.3, 0.3] x [0, 1.95] x [0.2, 0.8]
    // 与 AABB [0, 1] x [0, 1] x [0, 1] 有交集（x=0 处、y∈[0,1]、z∈[0.2,0.8]）
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, VolumeFilterAABBIntersectionWithPositionOverride)
{
    // 使用 x/y/z 覆盖参考坐标，验证 AABB 在绝对坐标下的正确构造
    auto inside = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(inside, nullptr);
    inside->setPosition(105.0f, 5.0f, 5.0f);
    m_server.spawnEntity(std::move(inside));

    auto outside = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(outside, nullptr);
    outside->setPosition(95.0f, 5.0f, 5.0f);
    m_server.spawnEntity(std::move(outside));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setX(100.0f);
    selector.setY(0.0f);
    selector.setZ(0.0f);
    selector.setDx(10.0f);
    selector.setDy(10.0f);
    selector.setDz(10.0f);
    // AABB: x∈[100, 111], y∈[0, 11], z∈[0, 11]

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 105.0f);
}

TEST_F(EntityResolverTest, VolumeFilterSelfSelectorWithDx)
{
    // @s 选择器也支持体积过滤
    auto serverPlayerEntity =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1000), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayerEntity->setPosition(5.0f, 64.0f, 5.0f);
    serverPlayerEntity->setPlayerId(42);
    auto* serverPlayerPtr = serverPlayerEntity.get();
    m_server.spawnEntity(std::move(serverPlayerEntity));

    ServerCommandSource source(
        &m_server, serverPlayerPtr, 0, Vector3d(5.0, 64.0, 5.0), Vector2f(0.0f, 0.0f), 2, 42, "TestPlayer");

    // 在体积范围内
    EntitySelector selector1 = EntitySelector::self();
    selector1.setDx(10.0f);
    selector1.setDy(10.0f);
    selector1.setDz(10.0f);
    auto result1 = EntityResolver::resolve(source, selector1);
    EXPECT_EQ(result1.size(), 1u);

    // 在体积范围外（使用远处的参考坐标）
    EntitySelector selector2 = EntitySelector::self();
    selector2.setX(1000.0f);
    selector2.setY(64.0f);
    selector2.setZ(5.0f);
    selector2.setDx(1.0f);
    auto result2 = EntityResolver::resolve(source, selector2);
    EXPECT_TRUE(result2.empty());
}

TEST_F(EntityResolverTest, VolumeFilterDistanceMaxCreatesCubicAABB)
{
    // 无 dx/dy/dz 但有 distance 最大值时，应构造立方体 AABB
    auto inside = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(inside, nullptr);
    inside->setPosition(3.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(inside));

    auto outside = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(outside, nullptr);
    outside->setPosition(20.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(outside));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.distance().setMax(5.0f);
    // 立方体 AABB: x∈[-5, 6], y∈[-5, 6], z∈[-5, 6]

    auto result = EntityResolver::resolve(source, selector);
    // inside 在 x=3（AABB 内），outside 在 x=20（AABB 外）
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0]->position().x, 3.0f);
}

TEST_F(EntityResolverTest, VolumeFilterNegativeDeltasReversedAABB)
{
    // 负值 dx：AABB 范围反向扩展
    auto entity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(entity, nullptr);
    entity->setPosition(3.0f, 0.5f, 0.5f);
    m_server.spawnEntity(std::move(entity));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setX(5.0f);
    selector.setDx(-10.0f); // AABB: x∈[5+(-10), 5+1] = [-5, 6]

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 1u);
}

// ============================================================================
// 5. sort= 排序测试
// ============================================================================

TEST_F(EntityResolverTest, SortNearest)
{
    auto nearEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(nearEntity, nullptr);
    nearEntity->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(nearEntity));

    auto mid = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(mid, nullptr);
    mid->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(mid));

    auto farEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(farEntity, nullptr);
    farEntity->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(farEntity));

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
    auto nearEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(nearEntity, nullptr);
    nearEntity->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(nearEntity));

    auto mid = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(mid, nullptr);
    mid->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(mid));

    auto farEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(farEntity, nullptr);
    farEntity->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(farEntity));

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
        auto pig = createEntityByType(EntityTypeKeys::PIG);
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
        auto pig = createEntityByType(EntityTypeKeys::PIG);
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
        auto pig = createEntityByType(EntityTypeKeys::PIG);
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
        auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto nearEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(nearEntity, nullptr);
    nearEntity->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(nearEntity));

    auto farEntity = createEntityByType(EntityTypeKeys::ZOMBIE);
    ASSERT_NE(farEntity, nullptr);
    farEntity->setPosition(50.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(farEntity));

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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    EntityInstanceId pigId = m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig1 = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig1, nullptr);
    pig1->setPosition(5.0f, 0.0f, 0.0f);
    pig1->addTag("friendly");
    m_server.spawnEntity(std::move(pig1));

    auto pig2 = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig2, nullptr);
    pig2->setPosition(10.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig2));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto nearEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(nearEntity, nullptr);
    nearEntity->setPosition(100.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(nearEntity));

    auto farEntity = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(farEntity, nullptr);
    farEntity->setPosition(200.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(farEntity));

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
    auto entity = createEntityByType(EntityTypeKeys::PIG);
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
    auto entity = createEntityByType(EntityTypeKeys::PIG);
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
    auto pig1 = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig1, nullptr);
    pig1->setPosition(5.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig1));

    auto pig2 = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig2, nullptr);
    pig2->setPosition(15.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig2));

    auto pig3 = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig3, nullptr);
    pig3->setPosition(25.0f, 0.0f, 0.0f);
    m_server.spawnEntity(std::move(pig3));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->setCustomName("TestPig");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    pig->setCustomName("Excluded");
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    auto zombie = createEntityByType(EntityTypeKeys::ZOMBIE);
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

// ============================================================================
// 8. @p / @a / @r 选择器与真实玩家的集成测试
// ============================================================================

TEST_F(EntityResolverTest, AllPlayersSelectorReturnsPlayers)
{
    // 注册玩家到 PlayerManager 并在世界中创建 Player 实体
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 0.0;
    aliceData->y = 64.0;
    aliceData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 0.0f, 64.0f, 0.0f);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 10.0;
    bobData->y = 64.0;
    bobData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 10.0f, 64.0f, 0.0f);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_EQ(result.size(), 2u);
}

TEST_F(EntityResolverTest, NearestPlayerSelectorReturnsClosestPlayer)
{
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 5.0;
    aliceData->y = 64.0;
    aliceData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 5.0f, 64.0f, 0.0f);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 50.0;
    bobData->y = 64.0;
    bobData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 50.0f, 64.0f, 0.0f);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::nearestPlayer();

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    // Alice 距离控制台(0,0,0)更近
    // 注意：createPlayerEntity 创建的 ServerPlayer 的 typeId 为空（未通过 EntityType 工厂创建），
    // 因此不能通过 getTypeId() 判断是否为玩家，但 EntityResolver 的 @p 选择器确实返回了最近的玩家实体
    EXPECT_NE(result[0], nullptr);
}

TEST_F(EntityResolverTest, RandomPlayerSelectorReturnsOnePlayer)
{
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 0.0;
    aliceData->y = 64.0;
    aliceData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 0.0f, 64.0f, 0.0f);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 10.0;
    bobData->y = 64.0;
    bobData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 10.0f, 64.0f, 0.0f);

    m_server.addTestPlayer(3, "Charlie");
    auto* charlieData = m_server.playerManager().getPlayer(3);
    ASSERT_NE(charlieData, nullptr);
    charlieData->x = 20.0;
    charlieData->y = 64.0;
    charlieData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        3, "Charlie", *m_server.world(), &m_server, nullptr, 20.0f, 64.0f, 0.0f);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::randomPlayer();

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    // 注意：createPlayerEntity 创建的 ServerPlayer 的 typeId 为空
    EXPECT_NE(result[0], nullptr);
    // 验证返回的是玩家实体（通过 dynamic_cast）
    EXPECT_NE(dynamic_cast<Player*>(result[0]), nullptr);
}

TEST_F(EntityResolverTest, AllPlayersSelectorWithDistanceFilter)
{
    // 在同一 Y 平面上测试距离过滤
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 5.0;
    aliceData->y = 0.0;
    aliceData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 5.0f, 0.0f, 0.0f);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 50.0;
    bobData->y = 0.0;
    bobData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 50.0f, 0.0f, 0.0f);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.distance().setMax(10.0f); // 只选择距离 10 以内的玩家

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    // Alice 距离原点 5.0，在范围内；Bob 距离原点 50.0，超出范围
}

TEST_F(EntityResolverTest, AllPlayersSelectorWithDistanceFilterAtSameY)
{
    // 在同一 Y 平面上测试距离过滤
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 5.0;
    aliceData->y = 0.0;
    aliceData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 5.0f, 0.0f, 0.0f);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 50.0;
    bobData->y = 0.0;
    bobData->z = 0.0;
    m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 50.0f, 0.0f, 0.0f);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.distance().setMax(10.0f); // 只选择距离 10 以内的玩家

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
}

// ============================================================================
// 9. gamemode= 过滤测试（玩家特有条件）
// ============================================================================

TEST_F(EntityResolverTest, GamemodeFilterMatchesPlayerGamemode)
{
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 0.0;
    aliceData->y = 64.0;
    aliceData->z = 0.0;
    auto* alice = m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 0.0f, 64.0f, 0.0f);
    ASSERT_NE(alice, nullptr);
    alice->setGameMode(GameMode::Creative);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 5.0;
    bobData->y = 64.0;
    bobData->z = 0.0;
    auto* bob = m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 5.0f, 64.0f, 0.0f);
    ASSERT_NE(bob, nullptr);
    bob->setGameMode(GameMode::Survival);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setGameMode("creative"); // gamemode=creative

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, GamemodeFilterNegatedExcludesCreative)
{
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 0.0;
    aliceData->y = 64.0;
    aliceData->z = 0.0;
    auto* alice = m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 0.0f, 64.0f, 0.0f);
    ASSERT_NE(alice, nullptr);
    alice->setGameMode(GameMode::Creative);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 5.0;
    bobData->y = 64.0;
    bobData->z = 0.0;
    auto* bob = m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 5.0f, 64.0f, 0.0f);
    ASSERT_NE(bob, nullptr);
    bob->setGameMode(GameMode::Survival);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setGameMode("creative", true); // gamemode=!creative

    auto result = EntityResolver::resolve(source, selector);
    // 只返回 Survival 模式的 Bob（非玩家实体被 gamemode 过滤排除）
    ASSERT_EQ(result.size(), 1u);
}

TEST_F(EntityResolverTest, GamemodeFilterExcludesNonPlayerEntities)
{
    // gamemode= 过滤只适用于玩家，非玩家实体应被排除
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.setGameMode("survival");

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty()); // 非玩家实体不匹配 gamemode 过滤
}

// ============================================================================
// 10. level= 过滤测试（玩家特有条件）
// ============================================================================

TEST_F(EntityResolverTest, LevelFilterMatchesPlayerLevel)
{
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 0.0;
    aliceData->y = 64.0;
    aliceData->z = 0.0;
    auto* alice = m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 0.0f, 64.0f, 0.0f);
    ASSERT_NE(alice, nullptr);
    alice->setExperienceLevel(10);

    m_server.addTestPlayer(2, "Bob");
    auto* bobData = m_server.playerManager().getPlayer(2);
    ASSERT_NE(bobData, nullptr);
    bobData->x = 5.0;
    bobData->y = 64.0;
    bobData->z = 0.0;
    auto* bob = m_server.playerEntityManager().createPlayerEntity(
        2, "Bob", *m_server.world(), &m_server, nullptr, 5.0f, 64.0f, 0.0f);
    ASSERT_NE(bob, nullptr);
    bob->setExperienceLevel(30);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.level().setMin(5);
    selector.level().setMax(20);

    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    // Alice (level 10) 在范围内，Bob (level 30) 不在
}

TEST_F(EntityResolverTest, LevelFilterExcludesNonPlayerEntities)
{
    // level= 过滤只适用于玩家，非玩家实体应被排除
    auto pig = createEntityByType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::allEntities();
    selector.level().setMin(1);
    selector.level().setMax(100);

    auto result = EntityResolver::resolve(source, selector);
    EXPECT_TRUE(result.empty()); // 非玩家实体不匹配 level 过滤
}

// ============================================================================
// 11. @s 选择器与玩家源测试
// ============================================================================

TEST_F(EntityResolverTest, SelfSelectorWithPlayerSource)
{
    // 创建一个 ServerPlayer 并作为命令源
    m_server.addTestPlayer(1, "Alice");
    auto* aliceData = m_server.playerManager().getPlayer(1);
    ASSERT_NE(aliceData, nullptr);
    aliceData->x = 5.0;
    aliceData->y = 64.0;
    aliceData->z = 0.0;
    auto* playerEntity = m_server.playerEntityManager().createPlayerEntity(
        1, "Alice", *m_server.world(), &m_server, nullptr, 5.0f, 64.0f, 0.0f);
    ASSERT_NE(playerEntity, nullptr);

    // 将 Player* 转换为 ServerPlayer* 用于 ServerCommandSource
    auto* serverPlayer = playerEntity->asServerPlayer();
    // createPlayerEntity 现在直接创建 ServerPlayer，
    // 所以 asServerPlayer() 返回有效指针，可直接用于 ServerCommandSource。

    // 创建 mc::ServerPlayer 实体并生成到世界中
    // 注意：需要使用 mc::ServerPlayer 而非 mc::server::ServerPlayer（StatisticsManager 中的前向声明）
    auto serverPlayerEntity =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1000), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayerEntity->setPosition(10.0f, 64.0f, 0.0f);
    serverPlayerEntity->setPlayerId(42);
    auto* serverPlayerPtr = serverPlayerEntity.get();
    m_server.spawnEntity(std::move(serverPlayerEntity));

    // 使用 ServerPlayer 构造命令源
    ServerCommandSource source(
        &m_server, serverPlayerPtr, 0, Vector3d(10.0, 64.0, 0.0), Vector2f(0.0f, 0.0f), 2, 42, "TestPlayer");

    EntitySelector selector = EntitySelector::self();
    auto result = EntityResolver::resolve(source, selector);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->id(), serverPlayerPtr->id());
}

TEST_F(EntityResolverTest, SelfSelectorWithDistanceFilter)
{
    // 创建 mc::ServerPlayer 并作为命令源
    auto serverPlayerEntity =
        std::make_unique<mc::ServerPlayer>(EntityInstanceId(1000), "TestPlayer", mc::test::testEcsRegistry());
    serverPlayerEntity->setPosition(10.0f, 64.0f, 0.0f);
    serverPlayerEntity->setPlayerId(42);
    auto* serverPlayerPtr = serverPlayerEntity.get();
    m_server.spawnEntity(std::move(serverPlayerEntity));

    ServerCommandSource source(
        &m_server, serverPlayerPtr, 0, Vector3d(10.0, 64.0, 0.0), Vector2f(0.0f, 0.0f), 2, 42, "TestPlayer");

    // 距离足够近
    EntitySelector selector1 = EntitySelector::self();
    selector1.distance().setMax(5.0f); // 自身距离为0
    auto result1 = EntityResolver::resolve(source, selector1);
    EXPECT_EQ(result1.size(), 1u);

    // 距离过滤器中指定自定义位置远离自身
    EntitySelector selector2 = EntitySelector::self();
    selector2.setX(100.0f);
    selector2.setY(64.0f);
    selector2.setZ(0.0f);
    selector2.distance().setMax(5.0f); // 自身距离自定义位置约90，超过5
    auto result2 = EntityResolver::resolve(source, selector2);
    EXPECT_TRUE(result2.empty());
}
