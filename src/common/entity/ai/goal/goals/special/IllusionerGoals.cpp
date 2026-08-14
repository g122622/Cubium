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

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/illager/IllusionerEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include <utility>

namespace mc::entity::ai::goal {

// ============================================================================
// IllusionerSpellGoal - 基类
// ============================================================================

IllusionerSpellGoal::IllusionerSpellGoal(IllusionerEntity* illusioner)
    : Goal()
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

    // 播放施法准备音效
    const char* prepareSound = getSpellPrepareSoundId();
    if (prepareSound != nullptr && prepareSound[0] != '\0') {
        m_illusioner->playSound(ResourceLocation(prepareSound), 1.0f, 1.0f);
    }
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
            target->x(), target->y() + target->eyeHeight(), target->z(), 30.0f, 30.0f);
    }

    // 施法准备阶段
    if (m_spellWarmup > 0) {
        m_spellWarmup--;
        if (m_spellWarmup <= 0) {
            // 执行施法
            castSpell();

            // 播放施法完成音效
            m_illusioner->playSound(SoundEvents::ENTITY_ILLUSIONER_CAST_SPELL, 1.0f, 1.0f);
        }
    }
}

// ============================================================================
// IllusionerCastingSpellGoal - 施法时看向目标并停步
//
// 对齐原版 SpellcasterIllager.SpellcasterCastingSpellGoal(MOVE+LOOK)：isSpellcasting()
// 期间启动，占用 Move+Look 使幻术师施法时停步、持续看向攻击目标。原版 Illusioner 在
// registerGoals 优先级1 显式注册此 goal（Illusioner.java:66），Cubium 此前缺失本次补齐。
// ============================================================================

IllusionerCastingSpellGoal::IllusionerCastingSpellGoal(IllusionerEntity* illusioner)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_illusioner(illusioner)
{
    MC_ASSERT_RELEASE(illusioner != nullptr);
}

bool IllusionerCastingSpellGoal::shouldExecute()
{
    return m_illusioner != nullptr && m_illusioner->isSpellcasting();
}

bool IllusionerCastingSpellGoal::shouldContinueExecuting()
{
    return shouldExecute();
}

void IllusionerCastingSpellGoal::tick()
{
    if (m_illusioner == nullptr) {
        return;
    }

    // 施法期间持续看向攻击目标（对齐原版 SpellcasterCastingSpellGoal.tick 调 lookControl）
    LivingEntity* target = m_illusioner->attackTarget();
    if (target != nullptr) {
        m_illusioner->lookController()->setLookPositionWithEntity(*target, 30.0f, 30.0f);
    }
}

// ============================================================================
// IllusionerBlindnessSpellGoal - 失明法术
// ============================================================================

IllusionerBlindnessSpellGoal::IllusionerBlindnessSpellGoal(IllusionerEntity* illusioner)
    : IllusionerSpellGoal(illusioner)
    , m_lastTargetId(0)
{}

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

    // 难度必须 >= Normal（原版 isHarderThan(Difficulty.NORMAL)）
    IWorld* world = m_illusioner->world();
    if (world == nullptr) {
        return false;
    }

    Difficulty difficulty = world->difficulty();
    if (difficulty < Difficulty::Normal) {
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

    // 施加失明效果，持续 400 ticks (20秒)
    entity::effect::EffectInstance blindness(entity::effect::EffectType::Blindness,
        BLINDNESS_DURATION,
        0,     // amplifier = 0 (效果等级 I)
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

const char* IllusionerBlindnessSpellGoal::getSpellPrepareSoundId() const
{
    return "entity.illusioner.prepare_blindness";
}

// ============================================================================
// IllusionerMirrorSpellGoal - 镜像法术（隐身）
// ============================================================================

IllusionerMirrorSpellGoal::IllusionerMirrorSpellGoal(IllusionerEntity* illusioner)
    : IllusionerSpellGoal(illusioner)
{}

bool IllusionerMirrorSpellGoal::shouldExecute()
{
    if (!IllusionerSpellGoal::shouldExecute()) {
        return false;
    }

    // 只有当幻术师没有隐身效果时才施法
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

    // 施加隐身效果，持续 1200 ticks (60秒)
    entity::effect::EffectInstance invisibility(entity::effect::EffectType::Invisibility,
        INVISIBILITY_DURATION,
        0,     // amplifier = 0 (效果等级 I)
        false, // 非环境效果
        true,  // 显示粒子
        true   // 显示图标
    );

    m_illusioner->addEffect(std::move(invisibility));
}

SpellcastingIllagerEntity::SpellType IllusionerMirrorSpellGoal::getSpellType() const
{
    return SpellcastingIllagerEntity::SpellType::Disappear;
}

const char* IllusionerMirrorSpellGoal::getSpellPrepareSoundId() const
{
    return "entity.illusioner.prepare_mirror";
}

} // namespace mc::entity::ai::goal
