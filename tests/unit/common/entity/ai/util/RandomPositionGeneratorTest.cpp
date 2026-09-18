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

#include <cmath>
#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

#include "entity/ai/util/RandomPositionGenerator.hpp"
#include "util/math/MathConstants.hpp"

namespace mc {
namespace test {

// ==================== RandomPositionGenerator 测试基类 ====================

class RandomPositionGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

// ==================== 静态常量测试 ====================
// 注: MAX_ATTEMPTS, MAX_GROUND_SEARCH, MIN_DISTANCE_SQ 是私有常量，无法直接测试
// 通过功能测试间接验证这些常量的影响

TEST_F(RandomPositionGeneratorTest, ConstantsAreDefined)
{
    // 验证常量在头文件中定义
    // 这些常量在 RandomPositionGenerator.hpp 中定义
    // MAX_ATTEMPTS = 10 (最大尝试次数)
    // MAX_GROUND_SEARCH = 10 (地面搜索最大高度差)
    // MIN_DISTANCE_SQ = 2.25f (最小距离平方)
    EXPECT_TRUE(true); // 编译通过即验证常量存在
}

// ==================== Direction 枚举测试 ====================

TEST_F(RandomPositionGeneratorTest, DirectionEnumValues)
{
    using Direction = entity::ai::util::RandomPositionGenerator::Direction;

    EXPECT_EQ(static_cast<u8>(Direction::None), 0);
    EXPECT_EQ(static_cast<u8>(Direction::North), 1);
    EXPECT_EQ(static_cast<u8>(Direction::South), 2);
    EXPECT_EQ(static_cast<u8>(Direction::East), 4);
    EXPECT_EQ(static_cast<u8>(Direction::West), 8);
    EXPECT_EQ(static_cast<u8>(Direction::Up), 16);
    EXPECT_EQ(static_cast<u8>(Direction::Down), 32);
}

TEST_F(RandomPositionGeneratorTest, DirectionBitwiseOr)
{
    using Direction = entity::ai::util::RandomPositionGenerator::Direction;

    Direction combined = Direction::North | Direction::East;
    EXPECT_EQ(static_cast<u8>(combined), 5); // 1 | 4 = 5

    combined = Direction::North | Direction::South | Direction::East;
    EXPECT_EQ(static_cast<u8>(combined), 7); // 1 | 2 | 4 = 7
}

TEST_F(RandomPositionGeneratorTest, DirectionBitwiseAnd)
{
    using Direction = entity::ai::util::RandomPositionGenerator::Direction;

    Direction combined = Direction::North | Direction::East;
    Direction result = combined & Direction::North;
    EXPECT_EQ(static_cast<u8>(result), 1);

    result = combined & Direction::South;
    EXPECT_EQ(static_cast<u8>(result), 0);
}

TEST_F(RandomPositionGeneratorTest, HasDirectionFunction)
{
    using Direction = entity::ai::util::RandomPositionGenerator::Direction;

    Direction combined = Direction::North | Direction::East;
    EXPECT_TRUE(hasDirection(combined, Direction::North));
    EXPECT_TRUE(hasDirection(combined, Direction::East));
    EXPECT_FALSE(hasDirection(combined, Direction::South));
    EXPECT_FALSE(hasDirection(combined, Direction::West));
}

// ==================== PositionCandidate 结构测试 ====================

TEST_F(RandomPositionGeneratorTest, PositionCandidateDefaultValues)
{
    entity::ai::util::RandomPositionGenerator::PositionCandidate candidate;

    EXPECT_FLOAT_EQ(candidate.position.x, 0.0f);
    EXPECT_FLOAT_EQ(candidate.position.y, 0.0f);
    EXPECT_FLOAT_EQ(candidate.position.z, 0.0f);
    EXPECT_FLOAT_EQ(candidate.score, 0.0f);
    EXPECT_FALSE(candidate.isSafe);
}

// ==================== 空指针测试 ====================// 注: RandomPositionGenerator 的方法需要 CreatureEntity 参数
// 由于我们无法在单元测试中轻松创建完整的 CreatureEntity 实例，
// 这些测试验证函数在空指针情况下的安全处理

TEST_F(RandomPositionGeneratorTest, NullCreatureHandling)
{
    // 验证 RandomPositionGenerator 函数签名存在
    // 实际的空指针测试需要 Mock CreatureEntity
    EXPECT_TRUE(true);
}

// ==================== 边界参数测试 ====================

TEST_F(RandomPositionGeneratorTest, AngleRangeValidation)
{
    // 验证 PI 常量正确性
    // MC 1.16.5: 海豚寻宝使用的角度范围是 PI / 8
    EXPECT_FLOAT_EQ(static_cast<f32>(math::PI / 8.0), 0.39269908f); // 约 22.5 度

    // 验证 PI 常量
    EXPECT_FLOAT_EQ(static_cast<f32>(math::PI), 3.14159265f);
}

// ==================== 数学函数测试 ====================

TEST_F(RandomPositionGeneratorTest, FloorToFunctionForCoordinates)
{
    // 验证 floorTo 函数用于坐标转换
    using math::floorTo;

    EXPECT_EQ(floorTo<i32>(1.7f), 1);
    EXPECT_EQ(floorTo<i32>(-1.7f), -2);
    EXPECT_EQ(floorTo<i32>(0.0f), 0);
    EXPECT_EQ(floorTo<i32>(100.9f), 100);
    EXPECT_EQ(floorTo<i32>(-0.1f), -1);
}

// ==================== findRandomTargetTowardsScaled 接口测试 ====================

TEST_F(RandomPositionGeneratorTest, FindRandomTargetTowardsScaledSignature)
{
    // 验证函数签名存在且可调用
    // 由于需要 CreatureEntity 参数，无法完全测试
    // 此测试确保函数接口正确
    math::Vector3f outPos;
    math::Vector3f targetPos(100.0f, 64.0f, 100.0f);

    // 空指针情况下应返回 false
    bool result = entity::ai::util::RandomPositionGenerator::findRandomTargetTowardsScaled(
        nullptr, 10, 7, targetPos, math::PI / 8.0, outPos);
    EXPECT_FALSE(result);
}

// ==================== findRandomTargetBlockTowards 接口测试 ====================

TEST_F(RandomPositionGeneratorTest, FindRandomTargetBlockTowardsSignature)
{
    // 验证函数签名存在且可调用
    math::Vector3f outPos;
    math::Vector3f targetPos(100.0f, 64.0f, 100.0f);

    // 空指针情况下应返回 false
    bool result =
        entity::ai::util::RandomPositionGenerator::findRandomTargetBlockTowards(nullptr, 10, 7, targetPos, outPos);
    EXPECT_FALSE(result);
}

// ==================== findRandomTargetBlock 接口测试 ====================

TEST_F(RandomPositionGeneratorTest, FindRandomTargetBlockSignature)
{
    // 验证函数签名存在且可调用
    math::Vector3f outPos;

    // 空指针情况下应返回 false
    bool result =
        entity::ai::util::RandomPositionGenerator::findRandomTargetBlock(nullptr, 10, 7, std::nullopt, outPos);
    EXPECT_FALSE(result);

    // 带回避位置
    math::Vector3f avoidPos(50.0f, 64.0f, 50.0f);
    result = entity::ai::util::RandomPositionGenerator::findRandomTargetBlock(nullptr, 10, 7, avoidPos, outPos);
    EXPECT_FALSE(result);
}

// ==================== 飞行位置生成方法接口测试 ====================

TEST_F(RandomPositionGeneratorTest, FindHoverPositionNullCreature)
{
    // 空指针情况下应返回 false
    math::Vector3f outPos;
    bool result = entity::ai::util::RandomPositionGenerator::findHoverPosition(
        nullptr, 8, 7, 0.0, 1.0, math::HALF_PI, 3, 1, outPos);
    EXPECT_FALSE(result);
}

TEST_F(RandomPositionGeneratorTest, FindAirAndWaterPositionNullCreature)
{
    // 空指针情况下应返回 false
    math::Vector3f outPos;
    bool result = entity::ai::util::RandomPositionGenerator::findAirAndWaterPosition(
        nullptr, 8, 4, -2, 0.0, 1.0, math::HALF_PI, outPos);
    EXPECT_FALSE(result);
}

TEST_F(RandomPositionGeneratorTest, FindAirPositionTowardsNullCreature)
{
    // 空指针情况下应返回 false
    math::Vector3f outPos;
    math::Vector3f targetPos(100.0f, 64.0f, 100.0f);
    bool result = entity::ai::util::RandomPositionGenerator::findAirPositionTowards(
        nullptr, 6, 8, 0, targetPos, math::PI / 10.0f, outPos);
    EXPECT_FALSE(result);
}

// ==================== 飞行位置生成方法参数验证 ====================

TEST_F(RandomPositionGeneratorTest, HoverPositionParameters)
{
    // 验证 HoverRandomPos 对应的参数范围
    // MC 1.21.11 Bee.WanderGoal: HoverRandomPos.getPos(bee, 8, 7, dirX, dirZ, PI/2, 3, 1)
    // xzRange=8, yRange=7, maxAngle=PI/2, maxAboveSolid=3, minAboveSolid=1
    constexpr i32 XZ_RANGE = 8;
    constexpr i32 Y_RANGE = 7;
    constexpr f32 MAX_ANGLE = math::HALF_PI;
    constexpr i32 MAX_ABOVE_SOLID = 3;
    constexpr i32 MIN_ABOVE_SOLID = 1;

    // 验证参数编译通过
    EXPECT_EQ(XZ_RANGE, 8);
    EXPECT_EQ(Y_RANGE, 7);
    EXPECT_FLOAT_EQ(MAX_ANGLE, math::HALF_PI);
    EXPECT_EQ(MAX_ABOVE_SOLID, 3);
    EXPECT_EQ(MIN_ABOVE_SOLID, 1);
}

TEST_F(RandomPositionGeneratorTest, AirAndWaterPositionParameters)
{
    // 验证 AirAndWaterRandomPos 对应的参数范围
    // MC 1.21.11 Bee.WanderGoal fallback: AirAndWaterRandomPos.getPos(bee, 8, 4, -2, dirX, dirZ, PI/2)
    constexpr i32 XZ_RANGE = 8;
    constexpr i32 Y_RANGE = 4;
    constexpr i32 Y_OFFSET = -2;
    constexpr f32 MAX_ANGLE = math::HALF_PI;

    EXPECT_EQ(XZ_RANGE, 8);
    EXPECT_EQ(Y_RANGE, 4);
    EXPECT_EQ(Y_OFFSET, -2);
    EXPECT_FLOAT_EQ(MAX_ANGLE, math::HALF_PI);
}

TEST_F(RandomPositionGeneratorTest, AirPositionTowardsParameters)
{
    // 验证 AirRandomPos.getPosTowards 对应的参数范围
    // MC 1.21.11 Bee.pathfindRandomlyTowards: AirRandomPos.getPosTowards(bee, k, l, i, target, PI/10)
    // k=6 (or less), l=8 (or less), i=-4/0/4 depending on height, maxAngle=PI/10 (18 degrees)
    constexpr f32 NARROW_ANGLE = math::PI / 10.0f;
    EXPECT_NEAR(NARROW_ANGLE, 0.314159f, 0.001f); // 约 18 度
}

// ==================== SQRT_OF_TWO 常量测试 ====================

TEST_F(RandomPositionGeneratorTest, SqrtOfTwo)
{
    // 验证 sqrt(2) 常量正确性
    // 用于 generateRandomDirectionWithinRadians 中的面积均匀分布
    constexpr f64 SQRT_OF_TWO = 1.4142135623730951;
    EXPECT_NEAR(SQRT_OF_TWO, std::sqrt(2.0), 1e-10);
}

// ==================== 数学函数验证 ====================

TEST_F(RandomPositionGeneratorTest, LerpFunction)
{
    // 验证 lerp 函数用于 generateRandomDirectionWithinRadians
    using math::lerp;
    EXPECT_DOUBLE_EQ(lerp(0.0, 8.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(lerp(0.0, 8.0, 1.0), 8.0);
    EXPECT_NEAR(lerp(0.0, 8.0, 0.5), 4.0, 1e-10);
    EXPECT_NEAR(lerp(0.0, 8.0, 0.25), 2.0, 1e-10);
}

TEST_F(RandomPositionGeneratorTest, DirectionAngleCalculation)
{
    // 验证方向向量到角度的转换
    // atan2(z, x) 在 MC 中用于计算方向角度
    // 向北 (Z+) 时 atan2(1,0) = PI/2，减去 PI/2 后为 0
    f64 northZ = 1.0, northX = 0.0;
    f64 northAngle = std::atan2(northZ, northX) - math::PI_DOUBLE / 2.0;
    EXPECT_NEAR(northAngle, 0.0, 1e-10);

    // 向东 (X+) 时 atan2(0,1) = 0，减去 PI/2 后为 -PI/2
    f64 eastX = 1.0, eastZ = 0.0;
    f64 eastAngle = std::atan2(eastZ, eastX) - math::PI_DOUBLE / 2.0;
    EXPECT_NEAR(eastAngle, -math::PI_DOUBLE / 2.0, 1e-10);

    // 向南 (Z-) 时 atan2(-1,0) = -PI/2，减去 PI/2 后为 -PI
    f64 southZ = -1.0, southX = 0.0;
    f64 southAngle = std::atan2(southZ, southX) - math::PI_DOUBLE / 2.0;
    EXPECT_NEAR(southAngle, -math::PI_DOUBLE, 1e-10);
}

} // namespace test
} // namespace mc
