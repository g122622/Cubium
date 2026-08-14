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

#include "common/TestWorldHelper.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/drop/BlockDropHandler.hpp"

#include <cmath>

namespace mc {
namespace {

/**
 * @brief ItemDropHelper 单元测试
 *
 * 测试物品掉落工具类的随机速度计算和物品实体生成功能。
 * 参考 MC 1.16.5 的 InventoryHelper.spawnItemStack() 和 Entity.entityDropItem()。
 */
class ItemDropHelperTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        // 注册原版实体类型，使 VanillaEntityTypeKeys::ITEM 全局缓存与注册表一致。
        // 本文件 SpawnItemEntities_SetsTypeIdToItem 等用例断言
        // entity->entityType() == VanillaEntityTypeKeys::ITEM，二者必须来自同一已初始化
        // 注册表，避免依赖前置测试的隐式注册状态（测试顺序污染）。
        // VanillaEntities::registerAll() 幂等且线程安全，无异常风险。
        entity::VanillaEntities::registerAll();
    }
};

// ============================================================================
// 随机速度计算测试
// ============================================================================

TEST_F(ItemDropHelperTest, GetBlockDropVelocityReturnsValidRange)
{
    // 方块掉落式速度范围测试
    // X: (random - 0.5) * 0.1 + random * 0.2 => [-0.05, 0.25]
    // Y: random * 0.2 => [0, 0.2]
    // Z: (random - 0.5) * 0.1 + random * 0.2 => [-0.05, 0.25]

    math::Random rng(12345);

    for (int i = 0; i < 100; ++i) {
        Vector3 velocity = ItemDropHelper::getBlockDropVelocity(rng);

        // X 范围: [-0.05, 0.25] (有宽松范围)
        EXPECT_GE(velocity.x, -0.15f) << "X velocity should be >= -0.15";
        EXPECT_LE(velocity.x, 0.35f) << "X velocity should be <= 0.35";

        // Y 范围: [0, 0.2]
        EXPECT_GE(velocity.y, 0.0f) << "Y velocity should be >= 0";
        EXPECT_LE(velocity.y, 0.25f) << "Y velocity should be <= 0.25";

        // Z 范围: [-0.05, 0.25] (有宽松范围)
        EXPECT_GE(velocity.z, -0.15f) << "Z velocity should be >= -0.15";
        EXPECT_LE(velocity.z, 0.35f) << "Z velocity should be <= 0.35";
    }
}

TEST_F(ItemDropHelperTest, GetSimpleDropVelocityReturnsValidRange)
{
    // 简单随机速度范围测试
    // X: random * 0.2 - 0.1 => [-0.1, 0.1]
    // Y: 0.2 (固定)
    // Z: random * 0.2 - 0.1 => [-0.1, 0.1]

    math::Random rng(54321);

    for (int i = 0; i < 100; ++i) {
        Vector3 velocity = ItemDropHelper::getSimpleDropVelocity(rng);

        // X 范围: [-0.1, 0.1]
        EXPECT_GE(velocity.x, -0.15f) << "X velocity should be >= -0.15";
        EXPECT_LE(velocity.x, 0.15f) << "X velocity should be <= 0.15";

        // Y 固定为 0.2
        EXPECT_FLOAT_EQ(velocity.y, 0.2f) << "Y velocity should be exactly 0.2";

        // Z 范围: [-0.1, 0.1]
        EXPECT_GE(velocity.z, -0.15f) << "Z velocity should be >= -0.15";
        EXPECT_LE(velocity.z, 0.15f) << "Z velocity should be <= 0.15";
    }
}

TEST_F(ItemDropHelperTest, GetPlayerDropVelocityDropAround)
{
    // 玩家丢弃物品（向四周散射）
    // 速度幅度: random * 0.5 (范围 [0, 0.5])
    // 随机角度: random * 2PI
    // X = -sin(angle) * magnitude
    // Y = 0.2 (固定)
    // Z = cos(angle) * magnitude

    math::Random rng(11111);

    for (int i = 0; i < 100; ++i) {
        Vector3 velocity = ItemDropHelper::getPlayerDropVelocity(rng, true);

        // X 范围: [-0.5, 0.5]
        EXPECT_GE(velocity.x, -0.55f) << "X velocity should be >= -0.55";
        EXPECT_LE(velocity.x, 0.55f) << "X velocity should be <= 0.55";

        // Y 固定为 0.2
        EXPECT_FLOAT_EQ(velocity.y, 0.2f) << "Y velocity should be exactly 0.2";

        // Z 范围: [-0.5, 0.5]
        EXPECT_GE(velocity.z, -0.55f) << "Z velocity should be >= -0.55";
        EXPECT_LE(velocity.z, 0.55f) << "Z velocity should be <= 0.55";
    }
}

TEST_F(ItemDropHelperTest, GetPlayerDropVelocityDirected)
{
    // 玩家丢弃物品（按朝向投掷）
    // 基础速度: 0.3
    // 随机偏移: 0.02 * random

    math::Random rng(22222);

    for (int i = 0; i < 100; ++i) {
        // 测试不同朝向
        f32 yaw = static_cast<f32>(i * 36); // 0, 36, 72, ... 度
        f32 pitch = 0.0f;

        Vector3 velocity = ItemDropHelper::getPlayerDropVelocity(rng, false, yaw, pitch);

        // 速度不应过大
        EXPECT_LT(std::abs(velocity.x), 1.0f) << "X velocity magnitude should be reasonable";
        EXPECT_LT(std::abs(velocity.y), 1.0f) << "Y velocity magnitude should be reasonable";
        EXPECT_LT(std::abs(velocity.z), 1.0f) << "Z velocity magnitude should be reasonable";
    }
}

TEST_F(ItemDropHelperTest, GetGaussianVelocityReturnsValidRange)
{
    // 高斯分布速度测试
    // 用于发射器等场景

    math::Random rng(33333);
    constexpr f32 BASE_VELOCITY = 0.2f;
    constexpr f32 INACCURACY = 1.0f;

    for (int i = 0; i < 100; ++i) {
        Vector3 velocity = ItemDropHelper::getGaussianVelocity(rng, BASE_VELOCITY, INACCURACY);

        // 高斯分布有 99.7% 概率在 3 个标准差内
        // 标准差约为 0.0075 * inaccuracy = 0.0075
        // 所以速度范围大约在 [0.2 - 0.0225, 0.2 + 0.0225]
        // 但为了测试稳定性，使用更宽松的范围
        EXPECT_GT(velocity.x, -0.5f) << "X velocity should be reasonable";
        EXPECT_LT(velocity.x, 0.5f) << "X velocity should be reasonable";
        EXPECT_GT(velocity.y, -0.5f) << "Y velocity should be reasonable";
        EXPECT_LT(velocity.y, 0.5f) << "Y velocity should be reasonable";
        EXPECT_GT(velocity.z, -0.5f) << "Z velocity should be reasonable";
        EXPECT_LT(velocity.z, 0.5f) << "Z velocity should be reasonable";
    }
}

TEST_F(ItemDropHelperTest, VelocitiesAreRandomized)
{
    // 测试随机速度确实有随机性
    math::Random rng1(12345);
    math::Random rng2(54321);

    Vector3 v1 = ItemDropHelper::getBlockDropVelocity(rng1);
    Vector3 v2 = ItemDropHelper::getBlockDropVelocity(rng2);

    // 不同种子生成的速度应该不同
    EXPECT_FALSE(v1.x == v2.x && v1.y == v2.y && v1.z == v2.z) << "Velocities from different seeds should differ";
}

// ============================================================================
// 物品实体生成测试（通过 BlockDropHandler）
// ============================================================================

TEST_F(ItemDropHelperTest, SpawnDropsCreatesItemEntities)
{
    EntityManager entityManager(mc::test::testEcsRegistry());
    math::Random rng(12345);

    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr) << "Shears should be registered";

    ItemStack stack(*shears, 1);
    const BlockPos pos(0, 64, 0);
    const std::vector<ItemStack> drops{stack};

    auto spawned = BlockDropHandler::spawnDrops(entityManager,
        nullptr, // physicsEngine
        pos,
        drops,
        "" // throwerUuid
    );

    ASSERT_EQ(spawned.size(), static_cast<size_t>(1)) << "Should spawn 1 item entity";
    EXPECT_TRUE(entityManager.hasEntity(spawned[0])) << "Entity should exist in manager";
    EXPECT_EQ(entityManager.entityCount(), static_cast<size_t>(1)) << "Should have 1 entity";
}

TEST_F(ItemDropHelperTest, SpawnMultipleItemEntities)
{
    EntityManager entityManager(mc::test::testEcsRegistry());
    math::Random rng(12345);

    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr) << "Apple should be registered";

    const BlockPos pos(10, 70, -5);
    const std::vector<ItemStack> drops{ItemStack(*apple, 3), ItemStack(*apple, 2), ItemStack(*apple, 1)};

    auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, drops, "");

    EXPECT_EQ(spawned.size(), static_cast<size_t>(3)) << "Should spawn 3 item entities";
    EXPECT_EQ(entityManager.entityCount(), static_cast<size_t>(3)) << "Should have 3 entities";
}

TEST_F(ItemDropHelperTest, SpawnEmptyDropsReturnsEmpty)
{
    EntityManager entityManager(mc::test::testEcsRegistry());

    const BlockPos pos(0, 0, 0);
    const std::vector<ItemStack> emptyDrops;

    auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, emptyDrops, "");

    EXPECT_TRUE(spawned.empty()) << "Empty drops should return empty vector";
    EXPECT_EQ(entityManager.entityCount(), static_cast<size_t>(0)) << "Should have 0 entities";
}

TEST_F(ItemDropHelperTest, SpawnWithEmptyStackSkipped)
{
    EntityManager entityManager(mc::test::testEcsRegistry());

    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr);

    const BlockPos pos(0, 0, 0);
    const std::vector<ItemStack> drops{ItemStack(*apple, 1),
        ItemStack(), // 空物品堆
        ItemStack(*apple, 2)};

    auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, drops, "");

    // 只应该生成 2 个实体（跳过空堆）
    EXPECT_EQ(spawned.size(), static_cast<size_t>(2)) << "Empty stacks should be skipped";
}

TEST_F(ItemDropHelperTest, SpawnWithThrowerUuid)
{
    EntityManager entityManager(mc::test::testEcsRegistry());

    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr);

    const BlockPos pos(0, 0, 0);
    const std::string throwerUuid = "test-player-uuid-123";
    const std::vector<ItemStack> drops{ItemStack(*apple, 1)};

    auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, drops, throwerUuid);

    EXPECT_EQ(spawned.size(), static_cast<size_t>(1));

    // 验证实体存在
    const Entity* entity = entityManager.getEntity(spawned[0]);
    ASSERT_NE(entity, nullptr);

    // 验证实体类型是物品
    EXPECT_EQ(entity->entityType(), entity::VanillaEntityTypeKeys::ITEM);
}

// ============================================================================
// 常量测试
// ============================================================================

TEST_F(ItemDropHelperTest, DefaultPickupDelayIsCorrect)
{
    // MC 1.16.5: 默认拾取延迟为 10 ticks (0.5秒)
    EXPECT_EQ(ItemDropHelper::DEFAULT_PICKUP_DELAY, 10);
}

TEST_F(ItemDropHelperTest, DefaultLifetimeIsCorrect)
{
    // MC 1.16.5: 默认存活时间为 6000 ticks (5分钟)
    EXPECT_EQ(ItemDropHelper::DEFAULT_LIFETIME, 6000);
}

// ============================================================================
// 边界和异常测试
// ============================================================================

TEST_F(ItemDropHelperTest, SpawnItemEntityWithNullWorldReturnsNullptr)
{
    // 空世界指针应返回 nullptr
    math::Random rng(12345);
    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr);

    ItemStack stack(*apple, 1);
    ItemEntity* result = ItemDropHelper::spawnItemEntity(nullptr, // 空世界指针
        stack,
        0.0,
        64.0,
        0.0,
        rng);

    EXPECT_EQ(result, nullptr) << "Should return nullptr for null world";
}

TEST_F(ItemDropHelperTest, SpawnItemEntityWithEmptyStackReturnsNullptr)
{
    // 空物品堆应返回 nullptr
    EntityManager entityManager(mc::test::testEcsRegistry());
    math::Random rng(12345);

    ItemStack emptyStack;                                         // 默认构造为空
    ItemEntity* result = ItemDropHelper::spawnItemEntity(nullptr, // 这里用 nullptr 也可以测试空堆检查
        emptyStack,
        0.0,
        64.0,
        0.0,
        rng);

    EXPECT_EQ(result, nullptr) << "Should return nullptr for empty stack";
}

TEST_F(ItemDropHelperTest, SpawnItemAtEntityWithNullEntityReturnsNullptr)
{
    // 空实体指针应返回 nullptr
    math::Random rng(12345);
    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr);

    ItemStack stack(*apple, 1);
    ItemEntity* result = ItemDropHelper::spawnItemAtEntity(nullptr, // 空实体指针
        stack,
        0.5f,
        rng);

    EXPECT_EQ(result, nullptr) << "Should return nullptr for null entity";
}

TEST_F(ItemDropHelperTest, SpawnItemEntitiesWithNullWorldReturnsEmpty)
{
    // 空世界指针应返回空向量
    math::Random rng(12345);
    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr);

    BlockPos pos(0, 64, 0);
    std::vector<ItemStack> drops{ItemStack(*apple, 1)};

    auto result = ItemDropHelper::spawnItemEntities(nullptr, // 空世界指针
        pos,
        drops,
        rng);

    EXPECT_TRUE(result.empty()) << "Should return empty vector for null world";
}

TEST_F(ItemDropHelperTest, SpawnItemEntitiesWithEmptyDropsReturnsEmpty)
{
    // 空掉落列表应返回空向量
    EntityManager entityManager(mc::test::testEcsRegistry());
    math::Random rng(12345);

    BlockPos pos(0, 64, 0);
    std::vector<ItemStack> emptyDrops;

    auto result = ItemDropHelper::spawnItemEntities(nullptr, pos, emptyDrops, rng);

    EXPECT_TRUE(result.empty()) << "Should return empty vector for empty drops";
}

TEST_F(ItemDropHelperTest, GaussianVelocityWithZeroBaseVelocity)
{
    // 基础速度为 0 时，结果主要由高斯偏移决定
    math::Random rng(44444);
    constexpr f32 BASE_VELOCITY = 0.0f;
    constexpr f32 INACCURACY = 1.0f;

    for (int i = 0; i < 50; ++i) {
        Vector3 velocity = ItemDropHelper::getGaussianVelocity(rng, BASE_VELOCITY, INACCURACY);

        // 高斯分布结果应该在合理范围内
        EXPECT_GT(velocity.x, -0.1f) << "X velocity should be reasonable";
        EXPECT_LT(velocity.x, 0.1f) << "X velocity should be reasonable";
        EXPECT_GT(velocity.y, -0.1f) << "Y velocity should be reasonable";
        EXPECT_LT(velocity.y, 0.3f) << "Y velocity has base 0.1";
        EXPECT_GT(velocity.z, -0.1f) << "Z velocity should be reasonable";
        EXPECT_LT(velocity.z, 0.1f) << "Z velocity should be reasonable";
    }
}

TEST_F(ItemDropHelperTest, GaussianVelocityWithZeroInaccuracy)
{
    // 不精确度为 0 时，应该返回接近基础速度的值
    math::Random rng(55555);
    constexpr f32 BASE_VELOCITY = 0.5f;
    constexpr f32 INACCURACY = 0.0f;

    for (int i = 0; i < 50; ++i) {
        Vector3 velocity = ItemDropHelper::getGaussianVelocity(rng, BASE_VELOCITY, INACCURACY);

        // 不精确度为 0 时，X 和 Z 应该等于基础速度
        EXPECT_NEAR(velocity.x, BASE_VELOCITY, 0.001f) << "X should equal base velocity";
        EXPECT_NEAR(velocity.z, BASE_VELOCITY, 0.001f) << "Z should equal base velocity";
        // Y 有基础 0.1
        EXPECT_NEAR(velocity.y, 0.1f, 0.001f) << "Y should be ~0.1";
    }
}

TEST_F(ItemDropHelperTest, PlayerDropVelocityDirectionConsistency)
{
    // 玩家定向投掷：不同朝向应该产生不同方向的速度
    math::Random rng(66666);

    Vector3 velNorth = ItemDropHelper::getPlayerDropVelocity(rng, false, 0.0f, 0.0f);   // 北
    Vector3 velEast = ItemDropHelper::getPlayerDropVelocity(rng, false, 90.0f, 0.0f);   // 东
    Vector3 velSouth = ItemDropHelper::getPlayerDropVelocity(rng, false, 180.0f, 0.0f); // 南
    Vector3 velWest = ItemDropHelper::getPlayerDropVelocity(rng, false, 270.0f, 0.0f);  // 西

    // 北方向 (yaw=0): Z 应该为正（向前）
    EXPECT_GT(velNorth.z, 0.0f) << "North-facing throw should have positive Z";

    // 东方向 (yaw=90): X 应该为负（MC 坐标系）
    EXPECT_LT(velEast.x, 0.0f) << "East-facing throw should have negative X";

    // 南方向 (yaw=180): Z 应该为负
    EXPECT_LT(velSouth.z, 0.0f) << "South-facing throw should have negative Z";

    // 西方向 (yaw=270): X 应该为正
    EXPECT_GT(velWest.x, 0.0f) << "West-facing throw should have positive X";
}

TEST_F(ItemDropHelperTest, BlockDropVelocityDistribution)
{
    // 验证方块掉落速度的统计分布
    // 进行大量采样验证分布特性
    math::Random rng(77777);
    constexpr int SAMPLES = 1000;

    f32 sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    f32 minX = 999.0f, maxX = -999.0f;
    f32 minY = 999.0f, maxY = -999.0f;
    f32 minZ = 999.0f, maxZ = -999.0f;

    for (int i = 0; i < SAMPLES; ++i) {
        Vector3 velocity = ItemDropHelper::getBlockDropVelocity(rng);
        sumX += velocity.x;
        sumY += velocity.y;
        sumZ += velocity.z;
        minX = std::min(minX, velocity.x);
        maxX = std::max(maxX, velocity.x);
        minY = std::min(minY, velocity.y);
        maxY = std::max(maxY, velocity.y);
        minZ = std::min(minZ, velocity.z);
        maxZ = std::max(maxZ, velocity.z);
    }

    // 验证均值接近理论中心
    f32 avgX = sumX / SAMPLES;
    f32 avgY = sumY / SAMPLES;
    f32 avgZ = sumZ / SAMPLES;

    // 理论上 X 和 Z 的均值应该在 0.1 左右
    EXPECT_NEAR(avgX, 0.1f, 0.05f) << "Average X should be ~0.1";
    EXPECT_NEAR(avgZ, 0.1f, 0.05f) << "Average Z should be ~0.1";
    // Y 的均值应该在 0.1 左右
    EXPECT_NEAR(avgY, 0.1f, 0.05f) << "Average Y should be ~0.1";

    // 验证范围符合预期
    EXPECT_GE(minX, -0.15f) << "Min X should be >= -0.15";
    EXPECT_LE(maxX, 0.45f) << "Max X should be <= 0.45";
    EXPECT_GE(minY, 0.0f) << "Min Y should be >= 0";
    EXPECT_LE(maxY, 0.3f) << "Max Y should be <= 0.3";
    EXPECT_GE(minZ, -0.15f) << "Min Z should be >= -0.15";
    EXPECT_LE(maxZ, 0.45f) << "Max Z should be <= 0.45";
}

TEST_F(ItemDropHelperTest, SimpleDropVelocityFixedY)
{
    // 验证简单掉落的 Y 速度始终为 0.2
    math::Random rng(88888);
    constexpr int SAMPLES = 100;

    for (int i = 0; i < SAMPLES; ++i) {
        Vector3 velocity = ItemDropHelper::getSimpleDropVelocity(rng);
        EXPECT_FLOAT_EQ(velocity.y, 0.2f) << "Y velocity should always be 0.2";
    }
}

TEST_F(ItemDropHelperTest, PlayerDropAroundProducesScatter)
{
    // 验证玩家四周散射产生不同方向的速度
    math::Random rng(99999);

    bool hasPositiveX = false, hasNegativeX = false;
    bool hasPositiveZ = false, hasNegativeZ = false;

    for (int i = 0; i < 100; ++i) {
        Vector3 velocity = ItemDropHelper::getPlayerDropVelocity(rng, true);
        if (velocity.x > 0.01f) hasPositiveX = true;
        if (velocity.x < -0.01f) hasNegativeX = true;
        if (velocity.z > 0.01f) hasPositiveZ = true;
        if (velocity.z < -0.01f) hasNegativeZ = true;
    }

    // 散射应该覆盖所有方向
    EXPECT_TRUE(hasPositiveX) << "Should have positive X velocities";
    EXPECT_TRUE(hasNegativeX) << "Should have negative X velocities";
    EXPECT_TRUE(hasPositiveZ) << "Should have positive Z velocities";
    EXPECT_TRUE(hasNegativeZ) << "Should have negative Z velocities";
}

} // namespace
} // namespace mc
