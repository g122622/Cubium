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

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;

// ============================================================================
// IronGolemEntity 功能测试
// ============================================================================
// 测试 TODO 收敛后新增的功能：攻击计时器递减、持花状态递减、属性注册、
// canAttackType 类型判断、attackEntityAsMob 核心逻辑等
// 基础构造和状态测试见 IronGolemGoalsTest.cpp

namespace {

/**
 * @brief 铁傀儡测试用世界
 */
class IronGolemTestWorld final : public test::BaseTestWorld {
public:
    IronGolemTestWorld() = default;

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    // 测试辅助
    void incrementTick() { m_currentTick++; }

private:
    u64 m_currentTick = 0;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    world::gamerule::GameRules m_gameRules;
};

} // anonymous namespace

// ==================== 基础状态测试 ====================
// 注意：部分用例调用 ironGolem->tick()，tick 链路 LivingEntity::tick 会解引用
// m_world（位置附魔检测等），故 SetUp 必须装配 world。复用本文件已定义的
// IronGolemTestWorld（与 IronGolemAttackTest 同一模式），避免无世界 tick 崩溃。

class IronGolemEntityFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册原版实体类型，使 VanillaEntityTypeKeys 指针非空，可解引用传入 canAttackType。
        // registerAll() 幂等且线程安全，多次调用无副作用。
        entity::VanillaEntities::registerAll();
        m_world = std::make_unique<IronGolemTestWorld>();
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1));
        ironGolem->setWorld(m_world.get());
    }

    void TearDown() override
    {
        ironGolem.reset();
        m_world.reset();
    }

    std::unique_ptr<IronGolemTestWorld> m_world;
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

// ==================== 攻击类型判断 ====================

TEST_F(IronGolemEntityFeatureTest, PlayerCreatedDoesNotAttackPlayer)
{
    // 玩家创建的铁傀儡不攻击玩家
    ironGolem->setPlayerCreated(true);
    EXPECT_FALSE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::PLAYER));
}

// 注意：canAttackType 依赖 VanillaEntityTypeKeys 的运行时注册值，
// SetUp 中已调用 VanillaEntities::registerAll() 注册原版实体类型，
// 因此 PLAYER/CREEPER 指针非空，可解引用测试真实的类型排除逻辑。

TEST_F(IronGolemEntityFeatureTest, NeverAttacksCreeper)
{
    // 铁傀儡不攻击苦力怕，无论是否玩家创建
    ironGolem->setPlayerCreated(true);
    EXPECT_FALSE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::CREEPER));

    ironGolem->setPlayerCreated(false);
    EXPECT_FALSE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::CREEPER));
}

TEST_F(IronGolemEntityFeatureTest, CanAttackOtherTypesByDefault)
{
    // 非玩家创建的铁傀儡默认允许攻击（除苦力怕和玩家创建者外的类型）
    // 使用 EntityType::UNKNOWN 测试默认行为：UNKNOWN 不等于 PLAYER/CREEPER/GHAST，应允许攻击
    ironGolem->setPlayerCreated(false);
    EXPECT_TRUE(ironGolem->canAttackType(entity::EntityType::UNKNOWN));
}

TEST_F(IronGolemEntityFeatureTest, WildGolemCanAttackPlayer)
{
    // 野生（非玩家创建的）铁傀儡可以攻击玩家
    ironGolem->setPlayerCreated(false);
    EXPECT_TRUE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::PLAYER));
}

// ==================== attackEntityAsMob 核心逻辑测试 ====================

class IronGolemAttackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册原版实体类型，使 VanillaEntityTypeKeys 指针非空，可解引用传入 canAttackType。
        // registerAll() 幂等且线程安全，多次调用无副作用。
        entity::VanillaEntities::registerAll();
        m_world = std::make_unique<IronGolemTestWorld>();
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1));
        ironGolem->setWorld(m_world.get());
        ironGolem->setPosition(0.0, 64.0, 0.0);
    }

    void TearDown() override
    {
        ironGolem.reset();
        m_world.reset();
    }

    std::unique_ptr<IronGolemTestWorld> m_world;
    std::unique_ptr<IronGolemEntity> ironGolem;
};

TEST_F(IronGolemAttackTest, AttackSetsAnimationState)
{
    // 攻击后应设置攻击计时器和手臂举起状态
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setPosition(2.0, 64.0, 0.0);

    ironGolem->attackEntityAsMob(*player);

    // 攻击动画：计时器 = ATTACK_DURATION(10)，手臂举起
    EXPECT_EQ(ironGolem->getAttackTimer(), 10);
    EXPECT_TRUE(ironGolem->isArmsRaised());
}

TEST_F(IronGolemAttackTest, AttackDealsDamageToTarget)
{
    // 铁傀儡攻击应对目标造成伤害
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setPosition(2.0, 64.0, 0.0);

    f32 healthBefore = player->health();
    bool success = ironGolem->attackEntityAsMob(*player);

    // 攻击应成功
    EXPECT_TRUE(success);
    // 目标生命值应减少（铁傀儡 ATTACK_DAMAGE = 15.0）
    EXPECT_LT(player->health(), healthBefore);
}

TEST_F(IronGolemAttackTest, AttackAppliesYAxisKnockback)
{
    // 铁傀儡攻击应将目标向上击飞
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setPosition(2.0, 64.0, 0.0);

    ironGolem->attackEntityAsMob(*player);

    // 目标应有Y轴速度（击退），玩家默认无击退抗性，所以系数为1.0
    // Y轴速度应为 0.4 * (1.0 - 0.0) = 0.4
    f64 yVelocity = player->velocity().y;
    EXPECT_GT(yVelocity, 0.0); // 向上击飞
    // 允许一定误差，因为 hurt 内部可能有额外修改
    EXPECT_NEAR(yVelocity, 0.4, 0.1);
}

TEST_F(IronGolemAttackTest, KnockbackResistedByTargetAttribute)
{
    // 目标有击退抗性时应减少击退
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setPosition(2.0, 64.0, 0.0);

    // 给玩家设置 100% 击退抗性（与铁傀儡相同）
    player->attributes().setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 1.0);

    ironGolem->attackEntityAsMob(*player);

    // 击退抗性为1.0时，击退系数 = max(0, 1.0 - 1.0) = 0，不应有Y轴击退
    f64 yVelocity = player->velocity().y;
    EXPECT_DOUBLE_EQ(yVelocity, 0.0);
}

TEST_F(IronGolemAttackTest, AttackDamageValue)
{
    // 铁傀儡攻击伤害应为 ATTACK_DAMAGE 属性值 (15.0)，MC 1.21.11 原版值
    f64 attackDamage = ironGolem->getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0);
    EXPECT_DOUBLE_EQ(attackDamage, 15.0);
}

TEST_F(IronGolemAttackTest, MobEntityCanAttackTypeDefaultAllowsNonGhast)
{
    // MobEntity 基类 canAttackType 排除恶魂，允许其他所有类型
    // 使用 IronGolemEntity 自身测试（非玩家创建、非苦力怕类型应允许）
    // IronGolemEntity 继承自 MobEntity，其 canAttackType 重写了部分类型
    // 使用 EntityType::UNKNOWN 测试 MobEntity 基类默认行为（非 GHAST 的类型应允许）
    EXPECT_TRUE(ironGolem->canAttackType(entity::EntityType::UNKNOWN));
}
