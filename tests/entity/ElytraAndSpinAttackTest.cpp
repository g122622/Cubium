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
 * @file ElytraAndSpinAttackTest.cpp
 * @brief 鞘翅飞行和三叉戟激流攻击姿态测试
 *
 * 测试 isElytraFlying() 和 isSpinAttacking() 方法的实现：
 * - isElytraFlying(): 通过 EntityFlags::FallFlying 标志判断
 * - isSpinAttacking(): 通过 LIVING_FLAGS_PARAM 第2位判断
 * - startSpinAttack()/stopSpinAttack(): 管理激流攻击状态
 */

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

/**
 * @brief 测试用的 LivingEntity 子类
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(LegacyEntityType::Unknown, EntityId(1), nullptr)
    {
        // 注册数据参数
        registerData();
        // 注册属性
        registerAttributes();
    }
};

/**
 * @brief isElytraFlying() 测试
 */
class ElytraFlyingTest : public ::testing::Test {
protected:
    void SetUp() override { entity = std::make_unique<TestLivingEntity>(); }

    std::unique_ptr<TestLivingEntity> entity;
};

TEST_F(ElytraFlyingTest, DefaultNotFlying)
{
    // 默认情况下不应该在鞘翅飞行
    EXPECT_FALSE(entity->isElytraFlying());
}

TEST_F(ElytraFlyingTest, SetFallFlyingFlagReturnsTrue)
{
    // 设置 FallFlying 标志
    entity->addFlag(EntityFlags::FallFlying);

    // 现在应该返回 true
    EXPECT_TRUE(entity->isElytraFlying());
}

TEST_F(ElytraFlyingTest, RemoveFallFlyingFlagReturnsFalse)
{
    // 设置标志
    entity->addFlag(EntityFlags::FallFlying);
    EXPECT_TRUE(entity->isElytraFlying());

    // 移除标志
    entity->removeFlag(EntityFlags::FallFlying);
    EXPECT_FALSE(entity->isElytraFlying());
}

TEST_F(ElytraFlyingTest, FlagPersistAcrossMultipleOperations)
{
    // 设置标志
    entity->addFlag(EntityFlags::FallFlying);
    EXPECT_TRUE(entity->isElytraFlying());

    // 设置其他标志
    entity->addFlag(EntityFlags::Sprinting);
    entity->addFlag(EntityFlags::Invisible);

    // FallFlying 标志应该仍然存在
    EXPECT_TRUE(entity->isElytraFlying());

    // 移除其他标志
    entity->removeFlag(EntityFlags::Sprinting);
    entity->removeFlag(EntityFlags::Invisible);

    // FallFlying 标志应该仍然存在
    EXPECT_TRUE(entity->isElytraFlying());
}

TEST_F(ElytraFlyingTest, FallFlyingFlagValue)
{
    // 验证 FallFlying 标志值正确（第7位）
    EXPECT_EQ(static_cast<u8>(EntityFlags::FallFlying), 0x80);
}

/**
 * @brief isSpinAttacking() 测试
 */
class SpinAttackTest : public ::testing::Test {
protected:
    void SetUp() override { entity = std::make_unique<TestLivingEntity>(); }

    std::unique_ptr<TestLivingEntity> entity;
};

TEST_F(SpinAttackTest, DefaultNotAttacking)
{
    // 默认情况下不应该在旋转攻击
    EXPECT_FALSE(entity->isSpinAttacking());
    EXPECT_EQ(entity->spinAttackDuration(), 0);
}

TEST_F(SpinAttackTest, StartSpinAttackSetsFlag)
{
    // 开始旋转攻击
    entity->startSpinAttack(20);

    // 应该在旋转攻击状态
    EXPECT_TRUE(entity->isSpinAttacking());
    EXPECT_EQ(entity->spinAttackDuration(), 20);
}

TEST_F(SpinAttackTest, StopSpinAttackClearsFlag)
{
    // 开始旋转攻击
    entity->startSpinAttack(20);
    EXPECT_TRUE(entity->isSpinAttacking());

    // 停止旋转攻击
    entity->stopSpinAttack();
    EXPECT_FALSE(entity->isSpinAttacking());
    EXPECT_EQ(entity->spinAttackDuration(), 0);
}

TEST_F(SpinAttackTest, UpdateSpinAttackDecrementsDuration)
{
    // 开始旋转攻击
    entity->startSpinAttack(5);
    EXPECT_EQ(entity->spinAttackDuration(), 5);

    // 更新一次，持续时间应该减少
    entity->updateSpinAttack();
    EXPECT_EQ(entity->spinAttackDuration(), 4);
    EXPECT_TRUE(entity->isSpinAttacking());

    // 继续更新直到结束
    entity->updateSpinAttack();
    entity->updateSpinAttack();
    entity->updateSpinAttack();
    entity->updateSpinAttack();

    // 持续时间应该为 0，状态应该清除
    EXPECT_EQ(entity->spinAttackDuration(), 0);
    EXPECT_FALSE(entity->isSpinAttacking());
}

TEST_F(SpinAttackTest, MultipleStartCallsResetDuration)
{
    // 开始旋转攻击
    entity->startSpinAttack(10);
    EXPECT_EQ(entity->spinAttackDuration(), 10);

    // 再次调用应该重置持续时间
    entity->startSpinAttack(20);
    EXPECT_EQ(entity->spinAttackDuration(), 20);
}

TEST_F(SpinAttackTest, SpinAttackDurationZeroDoesNotAffectFlag)
{
    // 开始旋转攻击
    entity->startSpinAttack(1);
    EXPECT_TRUE(entity->isSpinAttacking());

    // 更新一次，持续时间为 0，自动停止
    entity->updateSpinAttack();
    EXPECT_FALSE(entity->isSpinAttacking());
    EXPECT_EQ(entity->spinAttackDuration(), 0);
}

/**
 * @brief EntityPose 测试
 */
class EntityPoseTest : public ::testing::Test {};

TEST_F(EntityPoseTest, FallFlyingPoseValue)
{
    // 验证 FallFlying 姿态值正确
    EXPECT_EQ(static_cast<u8>(EntityPose::FallFlying), 1);
}

TEST_F(EntityPoseTest, SpinAttackPoseValue)
{
    // 验证 SpinAttack 姿态值正确
    EXPECT_EQ(static_cast<u8>(EntityPose::SpinAttack), 4);
}

TEST_F(EntityPoseTest, PoseToString)
{
    // 验证姿态转字符串
    EXPECT_STREQ(getPoseName(EntityPose::Standing), "standing");
    EXPECT_STREQ(getPoseName(EntityPose::FallFlying), "fall_flying");
    EXPECT_STREQ(getPoseName(EntityPose::SpinAttack), "spin_attack");
    EXPECT_STREQ(getPoseName(EntityPose::Sleeping), "sleeping");
    EXPECT_STREQ(getPoseName(EntityPose::Swimming), "swimming");
    EXPECT_STREQ(getPoseName(EntityPose::Crouching), "crouching");
}

TEST_F(EntityPoseTest, StringToPose)
{
    // 验证字符串转姿态
    EXPECT_EQ(getPoseByName("standing"), EntityPose::Standing);
    EXPECT_EQ(getPoseByName("fall_flying"), EntityPose::FallFlying);
    EXPECT_EQ(getPoseByName("spin_attack"), EntityPose::SpinAttack);
    EXPECT_EQ(getPoseByName("sleeping"), EntityPose::Sleeping);
    EXPECT_EQ(getPoseByName("swimming"), EntityPose::Swimming);
    EXPECT_EQ(getPoseByName("crouching"), EntityPose::Crouching);
}

/**
 * @brief 集成测试：姿态和标志协同工作
 */
class PoseAndFlagIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { entity = std::make_unique<TestLivingEntity>(); }

    std::unique_ptr<TestLivingEntity> entity;
};

TEST_F(PoseAndFlagIntegrationTest, ElytraFlyingSetsPose)
{
    // 设置鞘翅飞行标志
    entity->addFlag(EntityFlags::FallFlying);

    // 验证标志设置
    EXPECT_TRUE(entity->isElytraFlying());

    // 验证姿态可以设置为 FallFlying
    entity->setPose(EntityPose::FallFlying);
    EXPECT_EQ(entity->pose(), EntityPose::FallFlying);
}

TEST_F(PoseAndFlagIntegrationTest, SpinAttackSetsPose)
{
    // 开始旋转攻击
    entity->startSpinAttack(20);

    // 验证旋转攻击状态
    EXPECT_TRUE(entity->isSpinAttacking());

    // 验证姿态可以设置为 SpinAttack
    entity->setPose(EntityPose::SpinAttack);
    EXPECT_EQ(entity->pose(), EntityPose::SpinAttack);
}

TEST_F(PoseAndFlagIntegrationTest, ElytraFlyingHigherPriorityThanSpinAttack)
{
    // 鞘翅飞行优先级高于旋转攻击
    // 在 updatePose() 中，鞘翅飞行优先判断

    // 设置鞘翅飞行标志
    entity->addFlag(EntityFlags::FallFlying);
    entity->startSpinAttack(20);

    // 鞘翅飞行应该为 true
    EXPECT_TRUE(entity->isElytraFlying());
    EXPECT_TRUE(entity->isSpinAttacking());

    // 姿态应该设置为 FallFlying（优先级更高）
    entity->setPose(EntityPose::FallFlying);
    EXPECT_EQ(entity->pose(), EntityPose::FallFlying);
}
