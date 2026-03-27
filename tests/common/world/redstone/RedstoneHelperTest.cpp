#include <gtest/gtest.h>
#include "world/redstone/RedstoneHelper.hpp"
#include "world/redstone/RedstonePower.hpp"

using namespace mc;
using namespace mc::world::redstone;

/**
 * @brief RedstoneHelper 单元测试
 *
 * 测试红石辅助函数。
 */
class RedstoneHelperTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

// ========== 信号衰减测试 ==========

TEST_F(RedstoneHelperTest, AttenuateBasic) {
    // 信号衰减测试
    EXPECT_EQ(RedstoneHelper::attenuate(15, 1), 14);
    EXPECT_EQ(RedstoneHelper::attenuate(15, 5), 10);
    EXPECT_EQ(RedstoneHelper::attenuate(15, 15), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(15, 16), 0); // 不会变成负数
}

TEST_F(RedstoneHelperTest, AttenuateFromLowStrength) {
    // 从低强度开始衰减
    EXPECT_EQ(RedstoneHelper::attenuate(5, 1), 4);
    EXPECT_EQ(RedstoneHelper::attenuate(5, 5), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(1, 1), 0);
}

TEST_F(RedstoneHelperTest, AttenuateZeroDistance) {
    // 零距离传输不衰减
    EXPECT_EQ(RedstoneHelper::attenuate(15, 0), 15);
    EXPECT_EQ(RedstoneHelper::attenuate(10, 0), 10);
    EXPECT_EQ(RedstoneHelper::attenuate(0, 0), 0);
}

TEST_F(RedstoneHelperTest, AttenuateZeroStrength) {
    // 零强度无论传输多远都是零
    EXPECT_EQ(RedstoneHelper::attenuate(0, 0), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(0, 1), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(0, 100), 0);
}

// ========== 信号限制测试 ==========

TEST_F(RedstoneHelperTest, ClampInRange) {
    // 在范围内的值不变
    EXPECT_EQ(RedstoneHelper::clamp(0), 0);
    EXPECT_EQ(RedstoneHelper::clamp(5), 5);
    EXPECT_EQ(RedstoneHelper::clamp(10), 10);
    EXPECT_EQ(RedstoneHelper::clamp(15), 15);
}

TEST_F(RedstoneHelperTest, ClampOutOfRange) {
    // 超出范围的值被限制
    EXPECT_EQ(RedstoneHelper::clamp(-1), 0);
    EXPECT_EQ(RedstoneHelper::clamp(-100), 0);
    EXPECT_EQ(RedstoneHelper::clamp(16), 15);
    EXPECT_EQ(RedstoneHelper::clamp(100), 15);
}

TEST_F(RedstoneHelperTest, ClampBoundary) {
    // 边界值测试
    EXPECT_EQ(RedstoneHelper::clamp(-1), 0);
    EXPECT_EQ(RedstoneHelper::clamp(0), 0);
    EXPECT_EQ(RedstoneHelper::clamp(15), 15);
    EXPECT_EQ(RedstoneHelper::clamp(16), 15);
}

// ========== 常量测试 ==========

TEST_F(RedstoneHelperTest, ConstantsCorrect) {
    // 红石信号范围是 0-15
    EXPECT_EQ(RedstoneHelper::MIN_POWER, 0);
    EXPECT_EQ(RedstoneHelper::MAX_POWER, 15);
    EXPECT_EQ(RedstonePower::MIN_POWER, 0);
    EXPECT_EQ(RedstonePower::MAX_POWER, 15);
}

// ========== 方向判断测试 ==========

TEST_F(RedstoneHelperTest, IsHorizontal) {
    using namespace mc::Directions;

    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::North));
    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::South));
    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::East));
    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::West));

    EXPECT_FALSE(RedstoneHelper::isHorizontal(Direction::Up));
    EXPECT_FALSE(RedstoneHelper::isHorizontal(Direction::Down));
}

TEST_F(RedstoneHelperTest, IsVertical) {
    using namespace mc::Directions;

    EXPECT_TRUE(RedstoneHelper::isVertical(Direction::Up));
    EXPECT_TRUE(RedstoneHelper::isVertical(Direction::Down));

    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::North));
    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::South));
    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::East));
    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::West));
}

// ========== 组合衰减和限制测试 ==========

TEST_F(RedstoneHelperTest, AttenuateThenClamp) {
    // 典型使用场景：衰减后限制范围
    i32 strength = 10;
    i32 distance = 5;

    i32 result = RedstoneHelper::clamp(RedstoneHelper::attenuate(strength, distance));
    EXPECT_EQ(result, 5);

    // 超过最大距离
    distance = 20;
    result = RedstoneHelper::clamp(RedstoneHelper::attenuate(strength, distance));
    EXPECT_EQ(result, 0);
}
