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

#include "WitherSkeletonEntity.hpp"

#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../effect/EffectType.hpp"
#include "../nether/NetherEntities.hpp"

namespace mc {

WitherSkeletonEntity::WitherSkeletonEntity(EntityId id)
    : AbstractSkeletonEntity(id)
{
    registerGoals();
    registerAttributes();
    // MC 1.16.5: 在 registerGoals() 之后设置战斗目标
    // 凋灵骷髅使用近战攻击（重写的 setCombatTask 会选择 MeleeAttackGoal）
    setCombatTask();
}

std::unique_ptr<Entity> WitherSkeletonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherSkeletonEntity>(EntityId(0));
}

void WitherSkeletonEntity::registerGoals()
{
    // MC 1.16.5 WitherSkeletonEntity.registerGoals()
    // 注意：凋灵骷髅先添加攻击猪灵的目标，然后调用父类方法
    // 但是父类的 registerGoals() 会添加非战斗目标（移动、看向等），不会添加战斗目标
    // 战斗目标通过 setCombatTask() 添加

    // 优先级 3: 攻击最近的猪灵（需要视线检查）
    // MC 1.16.5: this.targetSelector.addGoal(3, new NearestAttackableTargetGoal<>(this, AbstractPiglinEntity.class,
    // true));
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<AbstractPiglinEntity>>(this,
            true, // checkSight - 需要视线检查
            0     // chance - 每tick都检查
            ));

    // 调用父类方法注册基础目标（游泳、被攻击反击、移动、看向等）
    AbstractSkeletonEntity::registerGoals();
}

void WitherSkeletonEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();

    // MC 1.16.5 WitherSkeletonEntity.func_234277_m_()
    // 凋灵骷髅攻击伤害为 4.0（比普通骷髅的 2.0 高）
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
}

void WitherSkeletonEntity::setCombatTask()
{
    // MC 1.16.5: 凋灵骷髅使用近战攻击
    // 重写父类方法，不检查装备，始终使用近战
    // 因为凋灵骷髅在 onInitialSpawn() 中被装备石剑

    // 移除现有的战斗目标
    if (m_rangedAttackGoal) {
        m_goalSelector.removeGoal(m_rangedAttackGoal.get());
    }
    if (m_meleeAttackGoal) {
        m_goalSelector.removeGoal(m_meleeAttackGoal.get());
    }

    // 凋灵骷髅始终使用近战攻击
    if (m_meleeAttackGoal) {
        m_goalSelector.addGoal(COMBAT_GOAL_PRIORITY, m_meleeAttackGoal.get());
    }
}

bool WitherSkeletonEntity::attackEntityAsMob(LivingEntity& target)
{
    // MC 1.16.5 WitherSkeletonEntity.attackEntityAsMob()
    // 首先调用父类方法执行基础攻击
    if (!AbstractSkeletonEntity::attackEntityAsMob(target)) {
        return false;
    }

    // 对目标施加凋零效果
    // MC 1.16.5: 目标如果是 LivingEntity，则添加 Wither 效果持续 200 ticks (10秒)
    // 效果等级为 0 (Wither I)
    target.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Wither,
        WITHER_DURATION_TICKS, // 200 ticks = 10 秒
        0,                     // 等级 0 (Wither I)
        false,                 // 不是来自药水（环境效果）
        true,                  // 显示粒子
        true                   // 显示图标
        ));

    return true;
}

} // namespace mc
