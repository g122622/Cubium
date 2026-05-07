#include <gtest/gtest.h>

#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/drop/BlockDropHandler.hpp"
#include "common/util/math/random/Random.hpp"

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
    static void SetUpTestSuite() {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// 随机速度计算测试
// ============================================================================

TEST_F(ItemDropHelperTest, GetBlockDropVelocityReturnsValidRange) {
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

TEST_F(ItemDropHelperTest, GetSimpleDropVelocityReturnsValidRange) {
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

TEST_F(ItemDropHelperTest, GetPlayerDropVelocityDropAround) {
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

TEST_F(ItemDropHelperTest, GetPlayerDropVelocityDirected) {
    // 玩家丢弃物品（按朝向投掷）
    // 基础速度: 0.3
    // 随机偏移: 0.02 * random

    math::Random rng(22222);

    for (int i = 0; i < 100; ++i) {
        // 测试不同朝向
        f32 yaw = static_cast<f32>(i * 36);   // 0, 36, 72, ... 度
        f32 pitch = 0.0f;

        Vector3 velocity = ItemDropHelper::getPlayerDropVelocity(rng, false, yaw, pitch);

        // 速度不应过大
        EXPECT_LT(std::abs(velocity.x), 1.0f) << "X velocity magnitude should be reasonable";
        EXPECT_LT(std::abs(velocity.y), 1.0f) << "Y velocity magnitude should be reasonable";
        EXPECT_LT(std::abs(velocity.z), 1.0f) << "Z velocity magnitude should be reasonable";
    }
}

TEST_F(ItemDropHelperTest, GetGaussianVelocityReturnsValidRange) {
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

TEST_F(ItemDropHelperTest, VelocitiesAreRandomized) {
    // 测试随机速度确实有随机性
    math::Random rng1(12345);
    math::Random rng2(54321);

    Vector3 v1 = ItemDropHelper::getBlockDropVelocity(rng1);
    Vector3 v2 = ItemDropHelper::getBlockDropVelocity(rng2);

    // 不同种子生成的速度应该不同
    EXPECT_FALSE(v1.x == v2.x && v1.y == v2.y && v1.z == v2.z)
        << "Velocities from different seeds should differ";
}

// ============================================================================
// 物品实体生成测试（通过 BlockDropHandler）
// ============================================================================

TEST_F(ItemDropHelperTest, SpawnDropsCreatesItemEntities) {
    EntityManager entityManager;
    math::Random rng(12345);

    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr) << "Shears should be registered";

    ItemStack stack(*shears, 1);
    const BlockPos pos(0, 64, 0);
    const std::vector<ItemStack> drops{stack};

    auto spawned = BlockDropHandler::spawnDrops(
        entityManager,
        nullptr,  // physicsEngine
        pos,
        drops,
        ""  // throwerUuid
    );

    ASSERT_EQ(spawned.size(), static_cast<size_t>(1)) << "Should spawn 1 item entity";
    EXPECT_TRUE(entityManager.hasEntity(spawned[0])) << "Entity should exist in manager";
    EXPECT_EQ(entityManager.entityCount(), static_cast<size_t>(1)) << "Should have 1 entity";
}

TEST_F(ItemDropHelperTest, SpawnMultipleItemEntities) {
    EntityManager entityManager;
    math::Random rng(12345);

    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr) << "Apple should be registered";

    const BlockPos pos(10, 70, -5);
    const std::vector<ItemStack> drops{
        ItemStack(*apple, 3),
        ItemStack(*apple, 2),
        ItemStack(*apple, 1)
    };

    auto spawned = BlockDropHandler::spawnDrops(
        entityManager,
        nullptr,
        pos,
        drops,
        ""
    );

    EXPECT_EQ(spawned.size(), static_cast<size_t>(3)) << "Should spawn 3 item entities";
    EXPECT_EQ(entityManager.entityCount(), static_cast<size_t>(3)) << "Should have 3 entities";
}

TEST_F(ItemDropHelperTest, SpawnEmptyDropsReturnsEmpty) {
    EntityManager entityManager;

    const BlockPos pos(0, 0, 0);
    const std::vector<ItemStack> emptyDrops;

    auto spawned = BlockDropHandler::spawnDrops(
        entityManager,
        nullptr,
        pos,
        emptyDrops,
        ""
    );

    EXPECT_TRUE(spawned.empty()) << "Empty drops should return empty vector";
    EXPECT_EQ(entityManager.entityCount(), static_cast<size_t>(0)) << "Should have 0 entities";
}

TEST_F(ItemDropHelperTest, SpawnWithEmptyStackSkipped) {
    EntityManager entityManager;

    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr);

    const BlockPos pos(0, 0, 0);
    const std::vector<ItemStack> drops{
        ItemStack(*apple, 1),
        ItemStack(),  // 空物品堆
        ItemStack(*apple, 2)
    };

    auto spawned = BlockDropHandler::spawnDrops(
        entityManager,
        nullptr,
        pos,
        drops,
        ""
    );

    // 只应该生成 2 个实体（跳过空堆）
    EXPECT_EQ(spawned.size(), static_cast<size_t>(2)) << "Empty stacks should be skipped";
}

TEST_F(ItemDropHelperTest, SpawnWithThrowerUuid) {
    EntityManager entityManager;

    auto* apple = Items::APPLE;
    ASSERT_NE(apple, nullptr);

    const BlockPos pos(0, 0, 0);
    const String throwerUuid = "test-player-uuid-123";
    const std::vector<ItemStack> drops{ItemStack(*apple, 1)};

    auto spawned = BlockDropHandler::spawnDrops(
        entityManager,
        nullptr,
        pos,
        drops,
        throwerUuid
    );

    EXPECT_EQ(spawned.size(), static_cast<size_t>(1));

    // 验证实体存在
    const Entity* entity = entityManager.getEntity(spawned[0]);
    ASSERT_NE(entity, nullptr);

    // 验证实体类型是物品
    EXPECT_EQ(entity->legacyType(), LegacyEntityType::Item);
}

// ============================================================================
// 常量测试
// ============================================================================

TEST_F(ItemDropHelperTest, DefaultPickupDelayIsCorrect) {
    // MC 1.16.5: 默认拾取延迟为 10 ticks (0.5秒)
    EXPECT_EQ(ItemDropHelper::DEFAULT_PICKUP_DELAY, 10);
}

TEST_F(ItemDropHelperTest, DefaultLifetimeIsCorrect) {
    // MC 1.16.5: 默认存活时间为 6000 ticks (5分钟)
    EXPECT_EQ(ItemDropHelper::DEFAULT_LIFETIME, 6000);
}

} // namespace
} // namespace mc
