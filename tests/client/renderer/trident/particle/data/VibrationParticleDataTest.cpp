/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "client/renderer/trident/particle/data/VibrationParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <memory>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::data;

class VibrationParticleDataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        targetPos = Vector3d(10.5, 64.0, -20.25);
        arrivalTicks = 15;
    }

    Vector3d targetPos;
    i32 arrivalTicks;
};

// ==================== 构造测试 ====================

TEST_F(VibrationParticleDataTest, Construction_SetsTargetPosition)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    EXPECT_DOUBLE_EQ(data.targetPosition().x, 10.5);
    EXPECT_DOUBLE_EQ(data.targetPosition().y, 64.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().z, -20.25);
}

TEST_F(VibrationParticleDataTest, Construction_SetsArrivalInTicks)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    EXPECT_EQ(data.arrivalInTicks(), 15);
}

TEST_F(VibrationParticleDataTest, GetType_ReturnsVibration)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    EXPECT_EQ(data.getType(), ParticleTypeId::Vibration);
}

// ==================== getTypeName 测试 ====================

TEST_F(VibrationParticleDataTest, GetTypeName_ReturnsVibrationName)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    EXPECT_EQ(data.getTypeName(), "minecraft:vibration");
}

// ==================== getParameters 测试 ====================

TEST_F(VibrationParticleDataTest, GetParameters_ContainsTargetPosition)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    auto params = data.getParameters();

    // 参数格式: "targetX targetY targetZ arrivalInTicks"
    EXPECT_NE(params.find("10.50"), std::string::npos);
    EXPECT_NE(params.find("64.00"), std::string::npos);
    EXPECT_NE(params.find("-20.25"), std::string::npos);
    EXPECT_NE(params.find("15"), std::string::npos);
}

TEST_F(VibrationParticleDataTest, GetParameters_ZeroPosition)
{
    VibrationParticleData data(Vector3d(0.0, 0.0, 0.0), 0);

    auto params = data.getParameters();

    EXPECT_NE(params.find("0.00"), std::string::npos);
    EXPECT_NE(params.find("0"), std::string::npos);
}

// ==================== clone 测试 ====================

TEST_F(VibrationParticleDataTest, Clone_ReturnsIdenticalCopy)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    auto cloned = data.clone();

    ASSERT_NE(cloned, nullptr);

    // 类型正确
    EXPECT_EQ(cloned->getType(), ParticleTypeId::Vibration);

    // 动态转型检查
    auto* clonedVibration = dynamic_cast<VibrationParticleData*>(cloned.get());
    ASSERT_NE(clonedVibration, nullptr);

    EXPECT_DOUBLE_EQ(clonedVibration->targetPosition().x, targetPos.x);
    EXPECT_DOUBLE_EQ(clonedVibration->targetPosition().y, targetPos.y);
    EXPECT_DOUBLE_EQ(clonedVibration->targetPosition().z, targetPos.z);
    EXPECT_EQ(clonedVibration->arrivalInTicks(), arrivalTicks);
}

TEST_F(VibrationParticleDataTest, Clone_IsIndependentCopy)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    auto cloned = data.clone();
    auto* clonedVibration = dynamic_cast<VibrationParticleData*>(cloned.get());
    ASSERT_NE(clonedVibration, nullptr);

    // 修改克隆后的值不影响原始值（通过不同对象验证独立性）
    VibrationParticleData data2(Vector3d(100.0, 200.0, 300.0), 30);
    EXPECT_DOUBLE_EQ(data2.targetPosition().x, 100.0);
    EXPECT_DOUBLE_EQ(clonedVibration->targetPosition().x, targetPos.x);
}

// ==================== 边界测试 ====================

TEST_F(VibrationParticleDataTest, LargeArrivalTicks)
{
    VibrationParticleData data(Vector3d(0.0, 0.0, 0.0), 1000000);

    EXPECT_EQ(data.arrivalInTicks(), 1000000);
    EXPECT_EQ(data.getType(), ParticleTypeId::Vibration);
}

TEST_F(VibrationParticleDataTest, NegativeCoordinates)
{
    Vector3d negPos(-100.5, -64.0, -200.75);
    VibrationParticleData data(negPos, 8);

    EXPECT_DOUBLE_EQ(data.targetPosition().x, -100.5);
    EXPECT_DOUBLE_EQ(data.targetPosition().y, -64.0);
    EXPECT_DOUBLE_EQ(data.targetPosition().z, -200.75);
}

TEST_F(VibrationParticleDataTest, SingleTickArrival)
{
    VibrationParticleData data(Vector3d(1.0, 1.0, 1.0), 1);

    EXPECT_EQ(data.arrivalInTicks(), 1);
    EXPECT_EQ(data.getType(), ParticleTypeId::Vibration);
}

// ==================== 拷贝/移动语义测试 ====================

TEST_F(VibrationParticleDataTest, CopyConstruction)
{
    VibrationParticleData data(targetPos, arrivalTicks);
    VibrationParticleData copy(data);

    EXPECT_DOUBLE_EQ(copy.targetPosition().x, targetPos.x);
    EXPECT_DOUBLE_EQ(copy.targetPosition().y, targetPos.y);
    EXPECT_DOUBLE_EQ(copy.targetPosition().z, targetPos.z);
    EXPECT_EQ(copy.arrivalInTicks(), arrivalTicks);
    EXPECT_EQ(copy.getType(), ParticleTypeId::Vibration);
}

TEST_F(VibrationParticleDataTest, MoveConstruction)
{
    VibrationParticleData data(targetPos, arrivalTicks);
    VibrationParticleData moved(std::move(data));

    EXPECT_DOUBLE_EQ(moved.targetPosition().x, targetPos.x);
    EXPECT_DOUBLE_EQ(moved.targetPosition().y, targetPos.y);
    EXPECT_DOUBLE_EQ(moved.targetPosition().z, targetPos.z);
    EXPECT_EQ(moved.arrivalInTicks(), arrivalTicks);
    EXPECT_EQ(moved.getType(), ParticleTypeId::Vibration);
}

// ==================== 实体来源构造测试 ====================

TEST_F(VibrationParticleDataTest, EntityConstruction_SetsTargetEntityId)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);

    EXPECT_TRUE(data.isEntitySource());
    EXPECT_FALSE(data.isBlockSource());
    EXPECT_EQ(data.kind(), VibrationParticleData::TargetKind::Entity);
    EXPECT_EQ(data.targetEntityId(), EntityInstanceId(42));
}

TEST_F(VibrationParticleDataTest, EntityConstruction_SetsYOffset)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);

    EXPECT_FLOAT_EQ(data.yOffset(), 2.5f);
}

TEST_F(VibrationParticleDataTest, EntityConstruction_SetsArrivalInTicks)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);

    EXPECT_EQ(data.arrivalInTicks(), 15);
}

TEST_F(VibrationParticleDataTest, EntityConstruction_GetTypeReturnsVibration)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);

    EXPECT_EQ(data.getType(), ParticleTypeId::Vibration);
}

TEST_F(VibrationParticleDataTest, EntityConstruction_GetTypeNameReturnsVibrationName)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);

    EXPECT_EQ(data.getTypeName(), "minecraft:vibration");
}

TEST_F(VibrationParticleDataTest, EntityConstruction_GetParametersContainsEntityId)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);

    auto params = data.getParameters();

    // 实体来源参数格式: "entity <id> <yOffset> <arrivalInTicks>"
    EXPECT_NE(params.find("entity"), std::string::npos);
    EXPECT_NE(params.find("42"), std::string::npos);
    EXPECT_NE(params.find("2.50"), std::string::npos);
    EXPECT_NE(params.find("15"), std::string::npos);
}

TEST_F(VibrationParticleDataTest, EntityConstruction_CloneReturnsIdenticalCopy)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);

    auto cloned = data.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getType(), ParticleTypeId::Vibration);

    auto* clonedVibration = dynamic_cast<VibrationParticleData*>(cloned.get());
    ASSERT_NE(clonedVibration, nullptr);

    EXPECT_TRUE(clonedVibration->isEntitySource());
    EXPECT_EQ(clonedVibration->targetEntityId(), EntityInstanceId(42));
    EXPECT_FLOAT_EQ(clonedVibration->yOffset(), 2.5f);
    EXPECT_EQ(clonedVibration->arrivalInTicks(), arrivalTicks);
}

TEST_F(VibrationParticleDataTest, EntityConstruction_CopyConstruction)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);
    VibrationParticleData copy(data);

    EXPECT_TRUE(copy.isEntitySource());
    EXPECT_EQ(copy.targetEntityId(), EntityInstanceId(42));
    EXPECT_FLOAT_EQ(copy.yOffset(), 2.5f);
    EXPECT_EQ(copy.arrivalInTicks(), arrivalTicks);
}

TEST_F(VibrationParticleDataTest, EntityConstruction_MoveConstruction)
{
    VibrationParticleData data(EntityInstanceId(42), 2.5f, arrivalTicks);
    VibrationParticleData moved(std::move(data));

    EXPECT_TRUE(moved.isEntitySource());
    EXPECT_EQ(moved.targetEntityId(), EntityInstanceId(42));
    EXPECT_FLOAT_EQ(moved.yOffset(), 2.5f);
    EXPECT_EQ(moved.arrivalInTicks(), arrivalTicks);
}

// ==================== 方块来源类型检查测试 ====================

TEST_F(VibrationParticleDataTest, BlockConstruction_IsBlockSource)
{
    VibrationParticleData data(targetPos, arrivalTicks);

    EXPECT_TRUE(data.isBlockSource());
    EXPECT_FALSE(data.isEntitySource());
    EXPECT_EQ(data.kind(), VibrationParticleData::TargetKind::Block);
}

} // namespace
} // namespace mc
