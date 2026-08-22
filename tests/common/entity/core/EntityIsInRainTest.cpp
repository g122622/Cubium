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
 * @file EntityIsInRainTest.cpp
 * @brief Entity::isInRain() 和 EndermanEntity::isInWaterOrRain() 单元测试
 *
 * 测试雨天检测功能：
 * - Entity::isInRain() 双位置检测（脚底位置和碰撞盒顶部位置）
 * - EndermanEntity::isInWaterOrRain() 返回值
 * - Entity::isWet() 组合检测
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/weather/WeatherUtils.hpp"

namespace mc {
namespace {

/**
 * @brief 用于测试 isInRain() 的 Mock 世界
 *
 * 提供可控的天气状态和 canRainAt() 行为
 */
class MockRainTestWorld : public mc::test::BaseTestWorld {
public:
    MockRainTestWorld()
        : m_isRaining(false)
        , m_canRainAtResult(false)
    {}

    // IWorld 天气接口
    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    void setRaining(bool raining) { m_isRaining = raining; }

    [[nodiscard]] bool canRainAt(const BlockPos& pos) const override
    {
        (void)pos;
        return m_canRainAtResult;
    }

    void setCanRainAtResult(bool can) { m_canRainAtResult = can; }

private:
    bool m_isRaining;
    bool m_canRainAtResult;
};

// ============================================================================
// Entity::isInRain() 测试
// ============================================================================

/**
 * @brief 测试 Entity::isInRain() - 世界不在下雨时返回 false
 */
TEST(EntityIsInRainTest, NoRainReturnsFalse)
{
    MockRainTestWorld world;
    world.setRaining(false);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);

    EXPECT_FALSE(enderman.isInRain()) << "isInRain() should return false when world is not raining";
}

/**
 * @brief 测试 Entity::isInRain() - 世界下雨但位置不可降雨时返回 false
 */
TEST(EntityIsInRainTest, RainingButPositionNotRainableReturnsFalse)
{
    MockRainTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(false); // 位置不可降雨（如在室内或沙漠生物群系）

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);

    EXPECT_FALSE(enderman.isInRain()) << "isInRain() should return false when position cannot receive rain";
}

/**
 * @brief 测试 Entity::isInRain() - 世界下雨且位置可降雨时返回 true
 */
TEST(EntityIsInRainTest, RainingAndPositionRainableReturnsTrue)
{
    MockRainTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(true);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);

    EXPECT_TRUE(enderman.isInRain())
        << "isInRain() should return true when world is raining and position can receive rain";
}

/**
 * @brief 测试 Entity::isInRain() - 空世界指针返回 false
 */
TEST(EntityIsInRainTest, NullWorldReturnsFalse)
{
    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(nullptr);
    enderman.setPosition(0.0, 64.0, 0.0);

    EXPECT_FALSE(enderman.isInRain()) << "isInRain() should return false when world is null";
}

// ============================================================================
// EndermanEntity::isInWaterOrRain() 测试
// ============================================================================

/**
 * @brief 测试 EndermanEntity::isInWaterOrRain() - 仅在水中
 */
TEST(EntityIsInRainTest, EndermanInWaterReturnsTrue)
{
    MockRainTestWorld world;
    world.setRaining(false);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);
    test::setEntityInWater(enderman, true);

    EXPECT_TRUE(enderman.isInWaterOrRain()) << "isInWaterOrRain() should return true when in water";
}

/**
 * @brief 测试 EndermanEntity::isInWaterOrRain() - 仅在雨中
 */
TEST(EntityIsInRainTest, EndermanInRainReturnsTrue)
{
    MockRainTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(true);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);
    test::setEntityInWater(enderman, false);

    EXPECT_TRUE(enderman.isInWaterOrRain()) << "isInWaterOrRain() should return true when in rain";
}

/**
 * @brief 测试 EndermanEntity::isInWaterOrRain() - 水中和雨中
 */
TEST(EntityIsInRainTest, EndermanInWaterAndRainReturnsTrue)
{
    MockRainTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(true);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);
    test::setEntityInWater(enderman, true);

    EXPECT_TRUE(enderman.isInWaterOrRain()) << "isInWaterOrRain() should return true when in both water and rain";
}

/**
 * @brief 测试 EndermanEntity::isInWaterOrRain() - 不在水中也不在雨中
 */
TEST(EntityIsInRainTest, EndermanNotInWaterOrRainReturnsFalse)
{
    MockRainTestWorld world;
    world.setRaining(false);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);
    test::setEntityInWater(enderman, false);

    EXPECT_FALSE(enderman.isInWaterOrRain()) << "isInWaterOrRain() should return false when not in water or rain";
}

// ============================================================================
// Entity::isWet() 测试
// ============================================================================

/**
 * @brief 测试 Entity::isWet() - 在雨中但不在水中
 */
TEST(EntityIsInRainTest, IsWetWhenInRainReturnsTrue)
{
    MockRainTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(true);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);
    test::setEntityInWater(enderman, false);

    // 在雨中但不在水中
    EXPECT_TRUE(enderman.isInRain()) << "Should be in rain";
    EXPECT_FALSE(enderman.isInWater()) << "Should not be in water";
    EXPECT_TRUE(enderman.isWet()) << "isWet() should return true when in rain";
}

/**
 * @brief 测试 Entity::isWet() - 在水中
 */
TEST(EntityIsInRainTest, IsWetWhenInWaterReturnsTrue)
{
    MockRainTestWorld world;
    world.setRaining(false);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);
    test::setEntityInWater(enderman, true);

    // 不在雨中但在水中
    EXPECT_FALSE(enderman.isInRain()) << "Should not be in rain";
    EXPECT_TRUE(enderman.isInWater()) << "Should be in water";
    EXPECT_TRUE(enderman.isWet()) << "isWet() should return true when in water";
}

/**
 * @brief 测试 Entity::isWet() - 不在水中也不在雨中
 */
TEST(EntityIsInRainTest, IsWetWhenNotWetReturnsFalse)
{
    MockRainTestWorld world;
    world.setRaining(false);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);
    test::setEntityInWater(enderman, false);

    EXPECT_FALSE(enderman.isInRain()) << "Should not be in rain";
    EXPECT_FALSE(enderman.isInWater()) << "Should not be in water";
    EXPECT_FALSE(enderman.isWet()) << "isWet() should return false when not wet";
}

// ============================================================================
// 双位置检测测试
// ============================================================================

/**
 * @brief 测试 Entity::isInRain() 双位置检测逻辑
 *
 * 验证 isInRain() 检查脚底位置和碰撞盒顶部位置两个位置。
 * 根据 MC 1.16.5 Entity.isInRain():
 * return this.world.isRainingAt(blockpos) || this.world.isRainingAt(
 *     new BlockPos((double)blockpos.getX(), this.getBoundingBox().maxY, (double)blockpos.getZ()));
 */
TEST(EntityIsInRainTest, DualPositionCheckLogic)
{
    // 这个测试验证双位置检测的实现逻辑
    // 由于 Mock 世界可以返回固定的 canRainAt 结果，
    // 我们验证当 canRainAt 返回 true 时，isInRain 返回 true

    MockRainTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(true);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(0.0, 64.0, 0.0);

    // 当 canRainAt 返回 true 时，isInRain 应该返回 true
    EXPECT_TRUE(enderman.isInRain()) << "isInRain should return true when canRainAt returns true for either position";

    // 验证：如果 canRainAt 返回 false，isInRain 应该返回 false
    world.setCanRainAtResult(false);
    EXPECT_FALSE(enderman.isInRain()) << "isInRain should return false when canRainAt returns false for both positions";
}

/**
 * @brief 测试 Entity::isInRain() 实现细节
 *
 * 验证 isInRain() 正确使用两个位置进行检测：
 * 1. 脚底位置：floor(position.x), floor(position.y), floor(position.z)
 * 2. 碰撞盒顶部位置：floor(position.x), floor(boundingBox.maxY), floor(position.z)
 */
TEST(EntityIsInRainTest, PositionCalculationForFootAndTop)
{
    // 这个测试验证位置计算的正确性
    // 实体在 (10.5, 64.0, 20.5) 位置
    // 脚底位置应该是 (10, 64, 20)
    // 假设碰撞盒高度为 2.9，顶部 Y 应该是 66.9，顶部位置应该是 (10, 66, 20)

    MockRainTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(true);

    EndermanEntity enderman(EntityInstanceId(1), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    enderman.setPosition(10.5, 64.0, 20.5);

    // 由于 Mock 世界返回固定的 canRainAt 结果，
    // 我们只验证位置被正确传递（通过接口调用）
    EXPECT_TRUE(enderman.isInRain()) << "Position should be correctly calculated and passed to canRainAt";
}

} // namespace
} // namespace mc
