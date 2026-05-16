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

#include "IllusionerGoals.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../effect/EffectInstance.hpp"
#include "../../../../effect/EffectType.hpp"
#include "../../../../entities/monster/illager/IllusionerEntity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../util/assert/AssertAll.hpp"
#include "../../../../../util/math/random/Random.hpp"

namespace mc::entity::ai::goal {

// ============================================================================
// IllusionerSpellGoal - 基类
// ============================================================================

IllusionerSpellGoal::IllusionerSpellGoal(IllusionerEntity* illusioner)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_illusioner(illusioner)
{
    MC_ASSERT_RELEASE(illusioner != nullptr);
}

bool IllusionerSpellGoal::shouldExecute()
{
    if (m_illusioner == nullptr) {
        return false;
    }

    // 如果正在施法，不执行
    if (m_illusioner->isSpellcasting()) {
        return false;
    }

    // 如果冷却中，不执行
    if (m_spellCooldown > 0) {
        return false;
    }

    return true;
}

bool IllusionerSpellGoal::shouldContinueExecuting()
{
    return m_illusioner != nullptr && m_spellWarmup > 0;
}

void IllusionerSpellGoal::startExecuting()
{
    m_spellWarmup = getCastWarmupTime();
    m_spellCooldown = getCastingInterval();

    // 设置施法状态
    m_illusioner->setSpellType(getSpellType());
    m_illusioner->setSpellTicks(m_spellWarmup + getCastingTime());
}

void IllusionerSpellGoal::resetTask()
{
    m_illusioner->clearSpellcasting();
}

void IllusionerSpellGoal::tick()
{
    if (m_illusioner == nullptr) {
        return;
    }

    // 减少冷却
    if (m_spellCooldown > 0) {
        m_spellCooldown--;
    }

    // 看向目标
    LivingEntity* target = m_illusioner->attackTarget();
    if (target != nullptr) {
        m_illusioner->lookController()->setLookPosition(
            target->x(),
            target->y() + target->eyeHeight(),
            target->z(),
            30.0f,
            30.0f
        );
    }

    // 施法准备阶段
    if (m_spellWarmup > 0) {
        m_spellWarmup--;
        if (m_spellWarmup <= 0) {
            // 执行施法
            castSpell();
        }
    }
}

// ============================================================================
// IllusionerBlindnessSpellGoal - 失明法术
// ============================================================================

IllusionerBlindnessSpellGoal::IllusionerBlindnessSpellGoal(IllusionerEntity* illusioner)
    : IllusionerSpellGoal(illusioner)
    , m_lastTargetId(0)
{
}

bool IllusionerBlindnessSpellGoal::shouldExecute()
{
    if (!IllusionerSpellGoal::shouldExecute()) {
        return false;
    }

    LivingEntity* target = m_illusioner->attackTarget();
    if (target == nullptr) {
        return false;
    }

    // 不能对同一个目标重复施法
    if (target->id() == m_lastTargetId) {
        return false;
    }

    // MC 1.16.5: 难度必须高于普通
    IWorld* world = m_illusioner->world();
    if (world == nullptr) {
        return false;
    }

    // 检查难度是否高于普通
    Difficulty difficulty = world->difficulty();
    if (difficulty != Difficulty::Hard) {
        return false;
    }

    return true;
}

void IllusionerBlindnessSpellGoal::startExecuting()
{
    IllusionerSpellGoal::startExecuting();

    LivingEntity* target = m_illusioner->attackTarget();
    if (target != nullptr) {
        m_lastTargetId = target->id();
    }
}

i32 IllusionerBlindnessSpellGoal::getCastWarmupTime() const
{
    return WARMUP_TIME;
}

i32 IllusionerBlindnessSpellGoal::getCastingTime() const
{
    return CASTING_TIME;
}

i32 IllusionerBlindnessSpellGoal::getCastingInterval() const
{
    return COOLDOWN;
}

void IllusionerBlindnessSpellGoal::castSpell()
{
    if (m_illusioner == nullptr) {
        return;
    }

    LivingEntity* target = m_illusioner->attackTarget();
    if (target == nullptr) {
        return;
    }

    // MC 1.16.5: 施加失明效果，持续 400 ticks (20秒)
    entity::effect::EffectInstance blindness(
        entity::effect::EffectType::Blindness,
        BLINDNESS_DURATION,
        0,    // amplifier = 0 (效果等级 I)
        false, // 非环境效果
        true,  // 显示粒子
        true   // 显示图标
    );

    target->addEffect(std::move(blindness));
}

SpellcastingIllagerEntity::SpellType IllusionerBlindnessSpellGoal::getSpellType() const
{
    return SpellcastingIllagerEntity::SpellType::Blindness;
}

// ============================================================================
// IllusionerMirrorSpellGoal - 镜像法术（隐身）
// ============================================================================

IllusionerMirrorSpellGoal::IllusionerMirrorSpellGoal(IllusionerEntity* illusioner)
    : IllusionerSpellGoal(illusioner)
{
}

bool IllusionerMirrorSpellGoal::shouldExecute()
{
    if (!IllusionerSpellGoal::shouldExecute()) {
        return false;
    }

    // MC 1.16.5: 只有当幻术师没有隐身效果时才施法
    if (m_illusioner->hasEffect(entity::effect::EffectType::Invisibility)) {
        return false;
    }

    return true;
}

i32 IllusionerMirrorSpellGoal::getCastWarmupTime() const
{
    return WARMUP_TIME;
}

i32 IllusionerMirrorSpellGoal::getCastingTime() const
{
    return CASTING_TIME;
}

i32 IllusionerMirrorSpellGoal::getCastingInterval() const
{
    return COOLDOWN;
}

void IllusionerMirrorSpellGoal::castSpell()
{
    if (m_illusioner == nullptr) {
        return;
    }

    // MC 1.16.5: 施加隐身效果，持续 1200 ticks (60秒)
    entity::effect::EffectInstance invisibility(
        entity::effect::EffectType::Invisibility,
        INVISIBILITY_DURATION,
        0,    // amplifier = 0 (效果等级 I)
        false, // 非环境效果
        true,  // 显示粒子
        true   // 显示图标
    );

    m_illusioner->addEffect(std::move(invisibility));

    // TODO: 客户端粒子效果和音效
    // 在客户端，幻术师隐身时会生成云粒子和播放镜像移动音效
}

SpellcastingIllagerEntity::SpellType IllusionerMirrorSpellGoal::getSpellType() const
{
    return SpellcastingIllagerEntity::SpellType::Disappear;
}

} // namespace mc::entity::ai::goal
