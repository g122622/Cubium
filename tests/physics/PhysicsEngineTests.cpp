/**
 * @file PhysicsEngineTests.cpp
 * @brief PhysicsEngine 核心测试
 *
 * 测试覆盖：
 * - 物理常量验证
 * - 碰撞缓存测试
 */

#include <gtest/gtest.h>
#include "physics/PhysicsConstants.hpp"
#include "physics/CollisionCache.hpp"
#include <thread>
#include <vector>

using namespace mc;
using namespace mc::physics;

namespace {

// =====================================================
// 物理常量测试
// =====================================================

TEST(PhysicsConstantsTest, GravityValue_Correct) {
    EXPECT_FLOAT_EQ(GRAVITY, 0.08f);
}

TEST(PhysicsConstantsTest, JumpVelocity_Correct) {
    EXPECT_FLOAT_EQ(JUMP_VELOCITY, 0.42f);
}

TEST(PhysicsConstantsTest, StepHeight_Correct) {
    EXPECT_FLOAT_EQ(STEP_HEIGHT, 0.6f);
}

TEST(PhysicsConstantsTest, DragValues_Correct) {
    EXPECT_FLOAT_EQ(DRAG_AIR, 0.98f);
    EXPECT_FLOAT_EQ(DRAG_GROUND, 0.91f);
    EXPECT_FLOAT_EQ(DRAG_WATER, 0.8f);
    EXPECT_FLOAT_EQ(DRAG_LAVA, 0.5f);
}

TEST(PhysicsConstantsTest, SlipperinessValues_Correct) {
    EXPECT_FLOAT_EQ(SLIPPERINESS_DEFAULT, 0.6f);
    EXPECT_FLOAT_EQ(SLIPPERINESS_ICE, 0.98f);
    EXPECT_FLOAT_EQ(SLIPPERINESS_BLUE_ICE, 0.989f);
}

TEST(PhysicsConstantsTest, GroundMoveFactor_Correct) {
    // 默认滑度0.6，移动因子 = speed * 0.216 / (0.6^3) = speed * 0.216 / 0.216 = speed
    f32 factor = getGroundMoveFactor(0.1f, 0.6f);
    EXPECT_NEAR(factor, 0.1f, 0.001f);

    // 冰滑度0.98，摩擦力更小，但移动因子公式使速度减小
    // factor = speed * 0.216 / (0.98^3) = speed * 0.216 / 0.941192 = speed * 0.229
    f32 iceFactor = getGroundMoveFactor(0.1f, 0.98f);
    EXPECT_NEAR(iceFactor, 0.0229f, 0.001f);  // 冰上起步更慢但滑行更远
}

TEST(PhysicsConstantsTest, SpecialBlockConstants_Correct) {
    // 蜘蛛网
    EXPECT_FLOAT_EQ(COBWEB_SLOWDOWN_XZ, 0.25f);
    EXPECT_FLOAT_EQ(COBWEB_SLOWDOWN_Y, 0.05f);

    // 蜂蜜块
    EXPECT_FLOAT_EQ(HONEY_BLOCK_MAX_SLIDE_VELOCITY, 0.05f);
    EXPECT_FLOAT_EQ(HONEY_BLOCK_SLIDE_THRESHOLD, 0.08f);
    EXPECT_FLOAT_EQ(HONEY_BLOCK_JUMP_FACTOR, 0.5f);

    // 史莱姆块
    EXPECT_FLOAT_EQ(SLIME_BLOCK_BOUNCE_FACTOR_LIVING, 1.0f);
    EXPECT_FLOAT_EQ(SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING, 0.8f);

    // 甜浆果丛
    EXPECT_FLOAT_EQ(SWEET_BERRY_BUSH_SLOWDOWN_XZ, 0.8f);
    EXPECT_FLOAT_EQ(SWEET_BERRY_BUSH_SLOWDOWN_Y, 0.75f);
}

TEST(PhysicsConstantsTest, SwimConstants_Correct) {
    EXPECT_FLOAT_EQ(WATER_BUOYANCY, 0.005f);
    EXPECT_FLOAT_EQ(SWIM_SPEED_BASE, 0.02f);
    EXPECT_FLOAT_EQ(WATER_DRAG, 0.8f);
    EXPECT_FLOAT_EQ(WATER_DRAG_SPRINT, 0.9f);
    EXPECT_FLOAT_EQ(DOLPHINS_GRACE_WATER_DRAG, 0.96f);
}

TEST(PhysicsConstantsTest, FlyConstants_Correct) {
    EXPECT_FLOAT_EQ(FLY_SPEED, 0.05f);
    EXPECT_FLOAT_EQ(WALK_SPEED, 0.1f);
    EXPECT_FLOAT_EQ(FLY_VERTICAL_DRAG, 0.6f);
    EXPECT_FLOAT_EQ(FLY_HORIZONTAL_DRAG, 0.91f);
    EXPECT_FLOAT_EQ(SPRINT_FLY_MULTIPLIER, 2.0f);
}

TEST(PhysicsConstantsTest, ItemConstants_Correct) {
    EXPECT_FLOAT_EQ(ITEM_GRAVITY, 0.04f);
    EXPECT_FLOAT_EQ(ITEM_DRAG, 0.98f);
    EXPECT_FLOAT_EQ(ITEM_WATER_BOUNCE_FACTOR, 0.5f);
}

TEST(PhysicsConstantsTest, LadderConstants_Correct) {
    EXPECT_FLOAT_EQ(LADDER_SPEED_MAX, 0.15f);
    EXPECT_FLOAT_EQ(LADDER_CLIMB_SPEED, 0.15f);
    EXPECT_FLOAT_EQ(LADDER_SLIDE_SPEED, -0.15f);
}

TEST(PhysicsConstantsTest, ElytraConstants_Correct) {
    EXPECT_FLOAT_EQ(ELYTRA_DRAG_HORIZONTAL, 0.99f);
    EXPECT_FLOAT_EQ(ELYTRA_DRAG_VERTICAL, 0.98f);
    EXPECT_FLOAT_EQ(ELYTRA_MIN_SPEED, 0.4f);
    EXPECT_FLOAT_EQ(ELYTRA_LIFT_COEFFICIENT, 0.75f);
}

TEST(PhysicsConstantsTest, SlowFallingGravity_Correct) {
    EXPECT_FLOAT_EQ(SLOW_FALLING_GRAVITY, 0.01f);
}

// =====================================================
// 碰撞缓存测试（额外的线程安全测试）
// =====================================================

TEST(CollisionCacheThreadSafeTest, CacheAndRetrieve) {
    CollisionCache cache;

    std::vector<AxisAlignedBB> boxes;
    boxes.emplace_back(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    boxes.emplace_back(5.0f, 0.0f, 5.0f, 6.0f, 1.0f, 6.0f);

    cache.cacheChunkCollisionBoxes(0, 0, std::move(boxes), 1);

    const auto* retrieved = cache.getChunkCollisionBoxes(0, 0);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->size(), 2u);
}

TEST(CollisionCacheThreadSafeTest, CacheMiss) {
    CollisionCache cache;

    const auto* retrieved = cache.getChunkCollisionBoxes(100, 100);
    EXPECT_EQ(retrieved, nullptr);
    EXPECT_EQ(cache.missCount(), 1u);
}

TEST(CollisionCacheThreadSafeTest, InvalidateChunk) {
    CollisionCache cache;

    std::vector<AxisAlignedBB> boxes;
    boxes.emplace_back(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    cache.cacheChunkCollisionBoxes(0, 0, std::move(boxes), 1);
    EXPECT_NE(cache.getChunkCollisionBoxes(0, 0), nullptr);

    cache.invalidateChunk(0, 0);
    EXPECT_EQ(cache.getChunkCollisionBoxes(0, 0), nullptr);
}

TEST(CollisionCacheThreadSafeTest, InvalidateChunkAndNeighbors) {
    CollisionCache cache;

    // 缓存中心区块和邻居
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            std::vector<AxisAlignedBB> boxes;
            boxes.emplace_back(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            cache.cacheChunkCollisionBoxes(dx, dz, std::move(boxes), 1);
        }
    }

    // 失效中心及邻居
    cache.invalidateChunkAndNeighbors(0, 0, 1);

    // 所有邻居都应该失效
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            EXPECT_EQ(cache.getChunkCollisionBoxes(dx, dz), nullptr)
                << "Chunk (" << dx << ", " << dz << ") should be invalidated";
        }
    }
}

TEST(CollisionCacheThreadSafeTest, HitMissStats) {
    CollisionCache cache;

    // 缓存一个区块
    std::vector<AxisAlignedBB> boxes;
    boxes.emplace_back(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    cache.cacheChunkCollisionBoxes(0, 0, std::move(boxes), 1);

    // 命中
    cache.getChunkCollisionBoxes(0, 0);
    EXPECT_EQ(cache.hitCount(), 1u);

    // 未命中
    cache.getChunkCollisionBoxes(1, 1);
    EXPECT_EQ(cache.missCount(), 1u);

    // 重置统计
    cache.resetStats();
    EXPECT_EQ(cache.hitCount(), 0u);
    EXPECT_EQ(cache.missCount(), 0u);
}

TEST(CollisionCacheThreadSafeTest, ThreadSafeHitMissStats) {
    CollisionCache cache;

    // 并发测试统计计数器的线程安全
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&cache, i]() {
            for (int j = 0; j < 100; ++j) {
                if (i % 2 == 0) {
                    cache.getChunkCollisionBoxes(i, j);
                }
                // 故意不缓存，只测试统计计数器的原子性
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 所有线程完成后，统计应该是正确的
    // 只有 i % 2 == 0 的线程（5个线程）会调用，每个线程100次，共500次
    EXPECT_EQ(cache.missCount(), 500u);
}

}  // namespace
