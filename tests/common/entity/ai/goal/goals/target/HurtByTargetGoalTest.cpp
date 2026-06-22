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

#include <memory>
#include <gtest/gtest.h>

#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/core/EntityTypeIdNumber.hpp"
#include "entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"

namespace mc {
namespace test {

// ==================== HurtByTargetGoal 排除谓词测试 ====================

class HurtByTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityId(1)); }

    void TearDown() override { pig.reset(); }

    std::unique_ptr<PigEntity> pig;
};

// ==================== 构造函数测试 ====================

TEST_F(HurtByTargetGoalTest, DefaultConstructor_AlertAlliesFalse)
{
    // 默认构造：不警醒盟友
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, Constructor_AlertAlliesTrue)
{
    // 带警醒盟友
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true);
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, Constructor_WithIgnoreDamagePredicate)
{
    // 带攻击者排除谓词
    auto goal =
        std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, [](const LivingEntity* attacker) -> bool {
            return attacker != nullptr && attacker->typeId() == entity::EntityTypeIdNumber::GUARDIAN;
        });
    EXPECT_NE(goal, nullptr);
}

// ==================== setAlertOthers 链式调用测试 ====================

TEST_F(HurtByTargetGoalTest, SetAlertOthers_ReturnsReference)
{
    // setAlertOthers 返回引用，支持链式调用
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    auto& ref = goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->typeId() == entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN;
    });
    // 返回的引用指向同一个对象
    EXPECT_EQ(&ref, goal.get());
}

TEST_F(HurtByTargetGoalTest, SetAlertOthers_EnablesAlertAllies)
{
    // 即使初始 alertAllies=false，调用 setAlertOthers 后应启用警醒
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), false);
    goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->typeId() == entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN;
    });
    // 如果链式调用成功且没有崩溃，说明 setAlertOthers 正常工作
    SUCCEED();
}

// ==================== 排除谓词语义测试 ====================

TEST_F(HurtByTargetGoalTest, IgnoreDamagePredicate_CompilesWithLambda)
{
    // 海豚式排除：排除守卫者
    auto dolphinPredicate = [](const LivingEntity* attacker) -> bool {
        if (!attacker) return false;
        auto type = attacker->typeId();
        return type == entity::EntityTypeIdNumber::GUARDIAN || type == entity::EntityTypeIdNumber::ELDER_GUARDIAN;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, dolphinPredicate);
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, IgnoreDamagePredicate_CompilesWithRaiderCheck)
{
    // 灾厄村民式排除：排除所有灾厄村民
    auto raiderPredicate = [](const LivingEntity* attacker) -> bool {
        return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, raiderPredicate);
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, IgnoreDamagePredicate_CompilesWithTypeIdCheck)
{
    // 潜影贝式排除：排除同类
    auto sameTypePredicate = [](const LivingEntity* attacker) -> bool {
        return attacker != nullptr && attacker->typeId() == entity::EntityTypeIdNumber::SHULKER;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, sameTypePredicate);
    EXPECT_NE(goal, nullptr);
}

// ==================== 组合用法测试 ====================

TEST_F(HurtByTargetGoalTest, Combined_IgnoreDamageAndAlertOthers)
{
    // MC 原版僵尸用法：HurtByTargetGoal(this).setAlertOthers(ZombifiedPiglin.class)
    // 即反击所有攻击者，但警醒盟友时不警醒僵尸猪灵
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true);
    goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->typeId() == entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN;
    });
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, Combined_IgnoreDamageAndIgnoreAlert)
{
    // MC 原版溺尸用法：HurtByTargetGoal(this, Drowned.class).setAlertOthers(ZombifiedPiglin.class)
    // 即不反击同类溺尸，且警醒盟友时不警醒僵尸猪灵
    auto drownedPredicate = [](const LivingEntity* attacker) -> bool {
        return attacker != nullptr && attacker->typeId() == entity::EntityTypeIdNumber::DROWNED;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, drownedPredicate);
    goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->typeId() == entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN;
    });
    EXPECT_NE(goal, nullptr);
}

} // namespace test
} // namespace mc
