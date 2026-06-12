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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include "entity/attribute/Attributes.hpp"
#include "entity/entities/passive/golem/IronGolemEntity.hpp"

using namespace mc;

// ============================================================================
// IronGolemEntity 功能测试
// ============================================================================
// 测试 TODO 收敛后新增的功能：攻击计时器递减、持花状态递减、属性注册等
// 基础构造和状态测试见 IronGolemGoalsTest.cpp

class IronGolemEntityFeatureTest : public ::testing::Test {
protected:
    void SetUp() override { ironGolem = std::make_unique<IronGolemEntity>(EntityId(1)); }

    void TearDown() override { ironGolem.reset(); }

    std::unique_ptr<IronGolemEntity> ironGolem;
};

// ==================== 攻击计时器 tick 递减 ====================

TEST_F(IronGolemEntityFeatureTest, AttackTimerDecrementsOnTick)
{
    // 设置攻击计时器，tick 后应该递减
    ironGolem->setAttackTimer(5);
    EXPECT_EQ(ironGolem->getAttackTimer(), 5);

    // tick 后计时器递减，手臂举起
    ironGolem->tick();
    EXPECT_EQ(ironGolem->getAttackTimer(), 4);
    EXPECT_TRUE(ironGolem->isArmsRaised());

    // 继续tick直到为0
    for (int i = 0; i < 4; ++i) {
        ironGolem->tick();
    }
    EXPECT_EQ(ironGolem->getAttackTimer(), 0);
    // 当计时器减到0时，手臂放下
    EXPECT_FALSE(ironGolem->isArmsRaised());
}

TEST_F(IronGolemEntityFeatureTest, AttackTimerStaysArmsRaisedWhilePositive)
{
    ironGolem->setAttackTimer(3);

    // tick 1: timer=2, arms raised
    ironGolem->tick();
    EXPECT_EQ(ironGolem->getAttackTimer(), 2);
    EXPECT_TRUE(ironGolem->isArmsRaised());

    // tick 2: timer=1, arms raised
    ironGolem->tick();
    EXPECT_EQ(ironGolem->getAttackTimer(), 1);
    EXPECT_TRUE(ironGolem->isArmsRaised());
}

// ==================== 持花状态 tick 递减 ====================

TEST_F(IronGolemEntityFeatureTest, HoldRoseTickDecrementsOnTick)
{
    ironGolem->setHoldingRose(true);
    EXPECT_EQ(ironGolem->getHoldRoseTick(), 400);

    // tick 后计时器递减
    ironGolem->tick();
    EXPECT_EQ(ironGolem->getHoldRoseTick(), 399);
    EXPECT_TRUE(ironGolem->isHoldingRose()); // 仍然持花
}

TEST_F(IronGolemEntityFeatureTest, HoldRoseStopsWhenTickReachesZero)
{
    ironGolem->setHoldingRose(true);
    EXPECT_EQ(ironGolem->getHoldRoseTick(), 400);

    // 模拟 400 tick
    for (int i = 0; i < 400; ++i) {
        ironGolem->tick();
    }
    EXPECT_EQ(ironGolem->getHoldRoseTick(), 0);
    EXPECT_FALSE(ironGolem->isHoldingRose());
}

TEST_F(IronGolemEntityFeatureTest, SetHoldingRoseFalseClearsTick)
{
    ironGolem->setHoldingRose(true);
    EXPECT_TRUE(ironGolem->isHoldingRose());

    ironGolem->setHoldingRose(false);
    EXPECT_FALSE(ironGolem->isHoldingRose());
    EXPECT_EQ(ironGolem->getHoldRoseTick(), 0);
}

// ==================== 属性注册 ====================

TEST_F(IronGolemEntityFeatureTest, KnockbackResistanceAttribute)
{
    // 铁傀儡完全免疫击退
    f64 kbResistance = ironGolem->getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0);
    EXPECT_DOUBLE_EQ(kbResistance, 1.0);
}

// ==================== 攻击判断 ====================

TEST_F(IronGolemEntityFeatureTest, PlayerCreatedDoesNotAttackPlayer)
{
    // 玩家创建的铁傀儡不攻击玩家
    ironGolem->setPlayerCreated(true);
    EXPECT_FALSE(ironGolem->canAttackEntity(entity::EntityTypeIdNumber::PLAYER));
}

// 注意：canAttackEntity 依赖 EntityTypeIdNumber 的运行时注册值，
// 在单元测试环境中这些 extern 变量均为默认值 0，无法区分不同实体类型，
// 因此 canAttackEntity 对非 PLAYER/非 CREEPER 类型的测试需要在集成测试中进行。
// PlayerCreatedDoesNotAttackPlayer 和 NeverAttacksCreeper 测试之所以通过，
// 是因为 canAttackEntity 对 PLAYER(玩家创建) 和 CREEPER 返回 false 的逻辑
// 在 typeId == 0 时仍能正确匹配。

TEST_F(IronGolemEntityFeatureTest, NeverAttacksCreeper)
{
    // 铁傀儡不攻击苦力怕，无论是否玩家创建
    ironGolem->setPlayerCreated(true);
    EXPECT_FALSE(ironGolem->canAttackEntity(entity::EntityTypeIdNumber::CREEPER));

    ironGolem->setPlayerCreated(false);
    EXPECT_FALSE(ironGolem->canAttackEntity(entity::EntityTypeIdNumber::CREEPER));
}
