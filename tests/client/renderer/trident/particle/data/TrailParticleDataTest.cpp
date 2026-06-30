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

#include "client/renderer/trident/particle/data/TrailParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <memory>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::data;

class TrailParticleDataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        targetPos = Vector3d(100.5, 70.0, -50.25);
        testColor = 0xFFFFFFFF;
        testDuration = 10;
    }

    Vector3d targetPos;
    u32 testColor;
    i32 testDuration;
};

// ==================== 构造测试 ====================

TEST_F(TrailParticleDataTest, Construction_SetsTargetPosition)
{
    TrailParticleData data(targetPos, testColor, testDuration);

    EXPECT_DOUBLE_EQ(data.targetPosition().x, 100.5);
    EXPECT_DOUBLE_EQ(data.targetPosition().y, 70.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().z, -50.25);
}

TEST_F(TrailParticleDataTest, Construction_SetsColor)
{
    TrailParticleData data(targetPos, 0xFFFF0000, testDuration);

    EXPECT_EQ(data.color(), 0xFFFF0000u);
}

TEST_F(TrailParticleDataTest, Construction_SetsDuration)
{
    TrailParticleData data(targetPos, testColor, 42);

    EXPECT_EQ(data.durationInTicks(), 42);
}

TEST_F(TrailParticleDataTest, GetType_ReturnsTrail)
{
    TrailParticleData data(targetPos, testColor, testDuration);

    EXPECT_EQ(data.getType(), ParticleTypeId::Trail);
}

// ==================== getTypeName 测试 ====================

TEST_F(TrailParticleDataTest, GetTypeName_ReturnsTrailName)
{
    TrailParticleData data(targetPos, testColor, testDuration);

    EXPECT_EQ(data.getTypeName(), "minecraft:trail");
}

// ==================== getParameters 测试 ====================

TEST_F(TrailParticleDataTest, GetParameters_ContainsTargetPosition)
{
    TrailParticleData data(targetPos, testColor, testDuration);

    auto params = data.getParameters();

    // 参数格式: "targetX targetY targetZ color duration"
    EXPECT_NE(params.find("100.50"), std::string::npos);
    EXPECT_NE(params.find("70.00"), std::string::npos);
    EXPECT_NE(params.find("-50.25"), std::string::npos);
}

TEST_F(TrailParticleDataTest, GetParameters_ContainsColor)
{
    TrailParticleData data(targetPos, 0xFF00FF00, testDuration);

    auto params = data.getParameters();

    EXPECT_NE(params.find("0xFF00FF00"), std::string::npos);
}

TEST_F(TrailParticleDataTest, GetParameters_ContainsDuration)
{
    TrailParticleData data(targetPos, testColor, 25);

    auto params = data.getParameters();

    EXPECT_NE(params.find("25"), std::string::npos);
}

// ==================== clone 测试 ====================

TEST_F(TrailParticleDataTest, Clone_ReturnsIdenticalCopy)
{
    TrailParticleData data(targetPos, 0xFF123456, 20);

    auto cloned = data.clone();

    ASSERT_NE(cloned, nullptr);

    // 类型正确
    EXPECT_EQ(cloned->getType(), ParticleTypeId::Trail);

    // 动态转型检查
    auto* clonedTrail = dynamic_cast<TrailParticleData*>(cloned.get());
    ASSERT_NE(clonedTrail, nullptr);

    EXPECT_DOUBLE_EQ(clonedTrail->targetPosition().x, targetPos.x);
    EXPECT_DOUBLE_EQ(clonedTrail->targetPosition().y, targetPos.y);
    EXPECT_DOUBLE_EQ(clonedTrail->targetPosition().z, targetPos.z);
    EXPECT_EQ(clonedTrail->color(), 0xFF123456u);
    EXPECT_EQ(clonedTrail->durationInTicks(), 20);
}

TEST_F(TrailParticleDataTest, Clone_IsIndependentCopy)
{
    TrailParticleData data(targetPos, 0xFFABCDEF, 15);
    auto cloned = data.clone();
    auto* clonedTrail = dynamic_cast<TrailParticleData*>(cloned.get());
    ASSERT_NE(clonedTrail, nullptr);

    // 验证克隆是独立的
    TrailParticleData data2(Vector3d(999.0, 888.0, 777.0), 0x00000000, 100);
    EXPECT_DOUBLE_EQ(clonedTrail->targetPosition().x, targetPos.x);
    EXPECT_EQ(clonedTrail->color(), 0xFFABCDEFu);
    EXPECT_EQ(clonedTrail->durationInTicks(), 15);
}

// ==================== 边界测试 ====================

TEST_F(TrailParticleDataTest, NegativeTargetCoordinates)
{
    Vector3d negPos(-100.5, -64.0, -200.75);
    TrailParticleData data(negPos, testColor, testDuration);

    EXPECT_DOUBLE_EQ(data.targetPosition().x, -100.5);
    EXPECT_DOUBLE_EQ(data.targetPosition().y, -64.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().z, -200.75);
}

TEST_F(TrailParticleDataTest, ZeroTargetPosition)
{
    Vector3d zeroPos(0.0, 0.0, 0.0);
    TrailParticleData data(zeroPos, testColor, testDuration);

    EXPECT_DOUBLE_EQ(data.targetPosition().x, 0.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().y, 0.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().z, 0.0);
}

TEST_F(TrailParticleDataTest, LargeCoordinates)
{
    Vector3d largePos(30000000.0, 256.0, 30000000.0);
    TrailParticleData data(largePos, testColor, 1000000);

    EXPECT_DOUBLE_EQ(data.targetPosition().x, 30000000.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().y, 256.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().z, 30000000.0);
    EXPECT_EQ(data.durationInTicks(), 1000000);
}

TEST_F(TrailParticleDataTest, SingleTickDuration)
{
    TrailParticleData data(targetPos, testColor, 1);

    EXPECT_EQ(data.durationInTicks(), 1);
}

TEST_F(TrailParticleDataTest, ZeroColor)
{
    TrailParticleData data(targetPos, 0x00000000, testDuration);

    EXPECT_EQ(data.color(), 0x00000000u);
}

TEST_F(TrailParticleDataTest, MaxColor)
{
    TrailParticleData data(targetPos, 0xFFFFFFFF, testDuration);

    EXPECT_EQ(data.color(), 0xFFFFFFFFu);
}

// ==================== 拷贝/移动语义测试 ====================

TEST_F(TrailParticleDataTest, CopyConstruction)
{
    TrailParticleData data(targetPos, 0xFF123456, 25);
    TrailParticleData copy(data);

    EXPECT_DOUBLE_EQ(copy.targetPosition().x, targetPos.x);
    EXPECT_DOUBLE_EQ(copy.targetPosition().y, targetPos.y);
    EXPECT_DOUBLE_EQ(copy.targetPosition().z, targetPos.z);
    EXPECT_EQ(copy.color(), 0xFF123456u);
    EXPECT_EQ(copy.durationInTicks(), 25);
    EXPECT_EQ(copy.getType(), ParticleTypeId::Trail);
}

TEST_F(TrailParticleDataTest, MoveConstruction)
{
    TrailParticleData data(targetPos, 0xFF123456, 25);
    TrailParticleData moved(std::move(data));

    EXPECT_DOUBLE_EQ(moved.targetPosition().x, targetPos.x);
    EXPECT_DOUBLE_EQ(moved.targetPosition().y, targetPos.y);
    EXPECT_DOUBLE_EQ(moved.targetPosition().z, targetPos.z);
    EXPECT_EQ(moved.color(), 0xFF123456u);
    EXPECT_EQ(moved.durationInTicks(), 25);
    EXPECT_EQ(moved.getType(), ParticleTypeId::Trail);
}

} // namespace
} // namespace mc
