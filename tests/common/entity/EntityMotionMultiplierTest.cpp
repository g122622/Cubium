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

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/util/math/Vector3.hpp"

using namespace mc;

namespace {

/**
 * @brief 测试用实体类
 */
class TestEntity : public Entity {
public:
    TestEntity()
        : Entity(EntityInstanceId(1))
    {
        // 初始化尺寸
        refreshDimensions();
    }

    [[nodiscard]] std::string getTypeId() const override { return "minecraft:test_entity"; }
};

/**
 * @brief 测试用 LivingEntity
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1))
    {
        setHealth(maxHealth());
    }
};

} // namespace

// ============================================================================
// Entity Motion Multiplier Tests
// ============================================================================

class EntityMotionMultiplierTest : public ::testing::Test {
protected:
    void SetUp() override { entity = std::make_unique<TestEntity>(); }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<TestEntity> entity;
};

TEST_F(EntityMotionMultiplierTest, DefaultNoMotionMultiplier)
{
    // 默认情况下，实体不应该有运动乘数
    EXPECT_FALSE(entity->hasMotionMultiplier());
    EXPECT_FLOAT_EQ(entity->motionMultiplier().x, 1.0f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().y, 1.0f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().z, 1.0f);
}

TEST_F(EntityMotionMultiplierTest, SetMotionMultiplier)
{
    // 设置运动乘数
    entity->setMotionMultiplier(Vector3(0.8f, 0.75f, 0.8f));

    EXPECT_TRUE(entity->hasMotionMultiplier());
    EXPECT_FLOAT_EQ(entity->motionMultiplier().x, 0.8f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().y, 0.75f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().z, 0.8f);
}

TEST_F(EntityMotionMultiplierTest, ClearMotionMultiplier)
{
    // 设置运动乘数
    entity->setMotionMultiplier(Vector3(0.5f, 0.5f, 0.5f));
    EXPECT_TRUE(entity->hasMotionMultiplier());

    // 清除运动乘数
    entity->clearMotionMultiplier();

    EXPECT_FALSE(entity->hasMotionMultiplier());
    EXPECT_FLOAT_EQ(entity->motionMultiplier().x, 1.0f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().y, 1.0f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().z, 1.0f);
}

TEST_F(EntityMotionMultiplierTest, SetMotionMultiplierSweetBerryBush)
{
    // 使用甜浆果丛的减速系数
    entity->setMotionMultiplier(Vector3(physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ,
        physics::SWEET_BERRY_BUSH_SLOWDOWN_Y,
        physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ));

    EXPECT_TRUE(entity->hasMotionMultiplier());
    EXPECT_FLOAT_EQ(entity->motionMultiplier().x, 0.8f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().y, 0.75f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().z, 0.8f);
}

TEST_F(EntityMotionMultiplierTest, BaseTickClearsMotionMultiplier)
{
    // 设置运动乘数
    entity->setMotionMultiplier(Vector3(0.5f, 0.5f, 0.5f));
    EXPECT_TRUE(entity->hasMotionMultiplier());

    // baseTick 应该清除运动乘数
    entity->baseTick();

    EXPECT_FALSE(entity->hasMotionMultiplier());
}

TEST_F(EntityMotionMultiplierTest, MultipleSetMotionMultiplier)
{
    // 第一次设置
    entity->setMotionMultiplier(Vector3(0.5f, 0.5f, 0.5f));
    EXPECT_TRUE(entity->hasMotionMultiplier());

    // 第二次设置（覆盖）
    entity->setMotionMultiplier(Vector3(0.8f, 0.75f, 0.8f));
    EXPECT_TRUE(entity->hasMotionMultiplier());
    EXPECT_FLOAT_EQ(entity->motionMultiplier().x, 0.8f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().y, 0.75f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().z, 0.8f);
}

TEST_F(EntityMotionMultiplierTest, MotionMultiplierDifferentValues)
{
    // 测试不同轴使用不同乘数
    entity->setMotionMultiplier(Vector3(0.1f, 0.2f, 0.3f));

    EXPECT_FLOAT_EQ(entity->motionMultiplier().x, 0.1f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().y, 0.2f);
    EXPECT_FLOAT_EQ(entity->motionMultiplier().z, 0.3f);
}

TEST_F(EntityMotionMultiplierTest, LivingEntityMotionMultiplier)
{
    // 测试 LivingEntity 也可以使用运动乘数
    auto livingEntity = std::make_unique<TestLivingEntity>();

    livingEntity->setMotionMultiplier(Vector3(0.8f, 0.75f, 0.8f));
    EXPECT_TRUE(livingEntity->hasMotionMultiplier());
    EXPECT_FLOAT_EQ(livingEntity->motionMultiplier().x, 0.8f);

    livingEntity->baseTick();
    EXPECT_FALSE(livingEntity->hasMotionMultiplier());
}

TEST_F(EntityMotionMultiplierTest, MotionMultiplierPhysicsConstants)
{
    // 验证物理常量正确
    EXPECT_FLOAT_EQ(physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ, 0.8f);
    EXPECT_FLOAT_EQ(physics::SWEET_BERRY_BUSH_SLOWDOWN_Y, 0.75f);
    EXPECT_FLOAT_EQ(physics::MOTION_THRESHOLD, 0.003f);
}
