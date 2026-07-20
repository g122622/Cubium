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

WitherSkeletonEntity::WitherSkeletonEntity(EntityInstanceId id)
    : AbstractSkeletonEntity(id)
{
    registerGoals();
    registerAttributes();
    // 凋灵骷髅使用近战攻击（setCombatTask 会选择 MeleeAttackGoal）
    setCombatTask();
}

std::unique_ptr<Entity> WitherSkeletonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherSkeletonEntity>(EntityInstanceId(0));
}

void WitherSkeletonEntity::registerGoals()
{
    // 凋灵骷髅优先攻击猪灵（需要视线检查）
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

    // 凋灵骷髅攻击伤害为 4.0（比普通骷髅的 2.0 高）
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
}

void WitherSkeletonEntity::setCombatTask()
{
    // 重写父类方法，凋灵骷髅始终使用近战攻击（装备石剑）

    // 移除现有的战斗目标
    // 使用 removeGoalsOfType 按类型移除，避免 unique_ptr 与 GoalSelector 之间的所有权冲突
    m_goalSelector.removeGoalsOfType<entity::ai::goal::RangedBowAttackGoal>();
    m_goalSelector.removeGoalsOfType<entity::ai::goal::MeleeAttackGoal>();

    // 凋灵骷髅始终使用近战攻击（创建新实例并转移所有权给 GoalSelector）
    m_goalSelector.addGoal(
        COMBAT_GOAL_PRIORITY, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, MELEE_ATTACK_SPEED, false));
}

bool WitherSkeletonEntity::attackEntityAsMob(LivingEntity& target)
{
    // 首先调用父类方法执行基础攻击
    if (!AbstractSkeletonEntity::attackEntityAsMob(target)) {
        return false;
    }

    // 对目标施加凋零效果（持续 200 ticks = 10 秒，等级 0 = Wither I）
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
