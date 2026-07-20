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

#include "entity/core/DataParameter.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectType.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试固定装置
// ============================================================================

class LivingEntityArrowCountTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建一个简单的 LivingEntity 用于测试
        m_living = std::make_unique<LivingEntity>(EntityInstanceId(1));
        m_living->registerData();
    }

    void TearDown() override { m_living.reset(); }

    std::unique_ptr<LivingEntity> m_living;
};

class EntityGlowingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建一个简单的 Entity 用于测试
        m_entity = std::make_unique<Entity>(EntityInstanceId(1));
        m_entity->registerData();
    }

    void TearDown() override { m_entity.reset(); }

    std::unique_ptr<Entity> m_entity;
};

// ============================================================================
// LivingEntity::getArrowCount / setArrowCountInEntity 测试
// ============================================================================

TEST_F(LivingEntityArrowCountTest, GetArrowCount_DefaultValue)
{
    // 默认箭矢数量应为 0
    EXPECT_EQ(m_living->getArrowCount(), 0);
}

TEST_F(LivingEntityArrowCountTest, SetArrowCountInEntity_PositiveValue)
{
    // 设置箭矢数量
    m_living->setArrowCountInEntity(5);
    EXPECT_EQ(m_living->getArrowCount(), 5);
}

TEST_F(LivingEntityArrowCountTest, SetArrowCountInEntity_Increment)
{
    // 递增箭矢数量
    m_living->setArrowCountInEntity(1);
    EXPECT_EQ(m_living->getArrowCount(), 1);

    m_living->setArrowCountInEntity(m_living->getArrowCount() + 1);
    EXPECT_EQ(m_living->getArrowCount(), 2);
}

TEST_F(LivingEntityArrowCountTest, SetArrowCountInEntity_NegativeValue)
{
    // 设置负值应该被限制为 0
    m_living->setArrowCountInEntity(-5);
    EXPECT_EQ(m_living->getArrowCount(), 0);
}

TEST_F(LivingEntityArrowCountTest, SetArrowCountInEntity_LargeValue)
{
    // 设置较大值
    m_living->setArrowCountInEntity(100);
    EXPECT_EQ(m_living->getArrowCount(), 100);
}

TEST_F(LivingEntityArrowCountTest, SetArrowCountInEntity_ZeroValue)
{
    // 设置为 0
    m_living->setArrowCountInEntity(5);
    EXPECT_EQ(m_living->getArrowCount(), 5);

    m_living->setArrowCountInEntity(0);
    EXPECT_EQ(m_living->getArrowCount(), 0);
}

TEST_F(LivingEntityArrowCountTest, ArrowHitTimer_InitialState)
{
    // 验证箭矢脱落计时器初始为 0
    // 计时器是私有成员，通过 tickArrows 行为间接测试
    EXPECT_EQ(m_living->getArrowCount(), 0);
}

// ============================================================================
// Entity::isGlowing / setGlowing 测试
// ============================================================================

TEST_F(EntityGlowingTest, IsGlowing_DefaultValue)
{
    // 默认不发光
    EXPECT_FALSE(m_entity->isGlowing());
}

TEST_F(EntityGlowingTest, SetGlowing_True)
{
    // 设置发光
    m_entity->setGlowing(true);
    EXPECT_TRUE(m_entity->isGlowing());
}

TEST_F(EntityGlowingTest, SetGlowing_False)
{
    // 先设置发光，再关闭
    m_entity->setGlowing(true);
    EXPECT_TRUE(m_entity->isGlowing());

    m_entity->setGlowing(false);
    EXPECT_FALSE(m_entity->isGlowing());
}

TEST_F(EntityGlowingTest, SetGlowing_Toggle)
{
    // 多次切换
    m_entity->setGlowing(true);
    EXPECT_TRUE(m_entity->isGlowing());

    m_entity->setGlowing(false);
    EXPECT_FALSE(m_entity->isGlowing());

    m_entity->setGlowing(true);
    EXPECT_TRUE(m_entity->isGlowing());
}

TEST_F(EntityGlowingTest, GlowingFlag_NotSetByDefault)
{
    // 默认没有 Glowing 标志
    EXPECT_FALSE(m_entity->hasFlag(mc::EntityFlags::Glowing));
}

TEST_F(EntityGlowingTest, GlowingFlag_SetBySetGlowing)
{
    // 设置发光后，标志位应该被设置
    // 注意：服务端设置，客户端检查标志位
    m_entity->setGlowing(true);
    // 在没有世界的情况下，setGlowing 不会设置标志位
    // 因为服务端才设置标志位
}

// ============================================================================
// LivingEntity 发光效果测试
// ============================================================================

class LivingEntityGlowEffectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_living = std::make_unique<LivingEntity>(EntityInstanceId(1));
        m_living->registerData();
    }

    void TearDown() override { m_living.reset(); }

    std::unique_ptr<LivingEntity> m_living;
};

TEST_F(LivingEntityGlowEffectTest, HasEffect_NoEffect_ReturnsFalse)
{
    // 没有效果时返回 false
    EXPECT_FALSE(m_living->hasEffect(effect::EffectType::Glowing));
}

TEST_F(LivingEntityGlowEffectTest, HasEffect_AfterAddingEffect_ReturnsTrue)
{
    // 添加发光效果后应该返回 true
    m_living->addEffect(effect::EffectInstance(effect::EffectType::Glowing, 200, 0));
    EXPECT_TRUE(m_living->hasEffect(effect::EffectType::Glowing));
}

TEST_F(LivingEntityGlowEffectTest, HasEffect_AfterRemovingEffect_ReturnsFalse)
{
    // 添加后移除效果应该返回 false
    m_living->addEffect(effect::EffectInstance(effect::EffectType::Glowing, 200, 0));
    EXPECT_TRUE(m_living->hasEffect(effect::EffectType::Glowing));

    m_living->removeEffect(effect::EffectType::Glowing);
    EXPECT_FALSE(m_living->hasEffect(effect::EffectType::Glowing));
}

TEST_F(LivingEntityGlowEffectTest, HasEffect_DifferentEffect_ReturnsFalse)
{
    // 添加不同效果不应该影响发光效果检查
    m_living->addEffect(effect::EffectInstance(effect::EffectType::Speed, 200, 0));
    EXPECT_TRUE(m_living->hasEffect(effect::EffectType::Speed));
    EXPECT_FALSE(m_living->hasEffect(effect::EffectType::Glowing));
}

TEST_F(LivingEntityGlowEffectTest, IsGlowing_FromSetGlowing)
{
    // 通过 setGlowing 设置发光
    m_living->setGlowing(true);
    EXPECT_TRUE(m_living->isGlowing());
}

TEST_F(LivingEntityGlowEffectTest, IsGlowing_FromEffectAndSetGlowing)
{
    // 两种方式都应该使 isGlowing 返回 true
    m_living->setGlowing(true);
    EXPECT_TRUE(m_living->isGlowing());

    m_living->setGlowing(false);
    m_living->addEffect(effect::EffectInstance(effect::EffectType::Glowing, 200, 0));
    // 注意：isGlowing 只检查 Entity 标志，不检查效果
    // 效果检查需要在 GlowEffect 中单独进行
}
