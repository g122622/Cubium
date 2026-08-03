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

#include "SlimeGoals.hpp"
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../attribute/Attributes.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../effect/EffectInstance.hpp"
#include "../../../../effect/EffectType.hpp"
#include "../../../../entities/monster/basic/SlimeEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../controller/JumpController.hpp"
#include "../../../controller/MovementController.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/DataParameter.hpp"

namespace mc::entity::ai::goal {

namespace {
constexpr f32 DEFAULT_MOVEMENT_SPEED = 0.2f;
}

// ============================================================================
// SlimeFloatGoal
// ============================================================================

SlimeFloatGoal::SlimeFloatGoal(SlimeEntity* slime)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Jump, GoalFlag::Move})
    , m_slime(slime)
{
    MC_ASSERT_RELEASE(slime != nullptr);
}

bool SlimeFloatGoal::shouldExecute()
{
    return m_slime->isInWater() || m_slime->isInLava();
}

void SlimeFloatGoal::tick()
{
    math::Random& rng = m_slime->getRandom();
    if (rng.nextFloat() < JUMP_CHANCE) {
        m_slime->jumpController()->setJumping();
    }

    m_slime->setAIMoveSpeed(static_cast<f32>(SWIM_SPEED *
        m_slime->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, DEFAULT_MOVEMENT_SPEED)));
}

// ============================================================================
// SlimeAttackGoal
// ============================================================================

SlimeAttackGoal::SlimeAttackGoal(SlimeEntity* slime)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_slime(slime)
    , m_attackTarget(nullptr)
    , m_attackTimer(0)
{
    MC_ASSERT_RELEASE(slime != nullptr);
}

bool SlimeAttackGoal::shouldExecute()
{
    LivingEntity* target = m_slime->attackTarget();
    if (target == nullptr) {
        return false;
    }

    if (!target->isAlive()) {
        return false;
    }

    // 创造模式或旁观者玩家不会被攻击
    auto* targetPlayer = dynamic_cast<Player*>(target);
    if (targetPlayer != nullptr && (targetPlayer->isCreative() || targetPlayer->isSpectator())) {
        return false;
    }

    m_attackTarget = target;
    return true;
}

bool SlimeAttackGoal::shouldContinueExecuting()
{
    if (m_attackTimer <= 0) {
        return false;
    }

    // 与 MC 1.21.11 SlimeAttackGoal.canContinueToUse 对齐：重新获取目标而非
    // 直接解引用缓存的 m_attackTarget。目标可能在 tick 之间被清除（死亡、
    // 切换目标、攻击者清除等），此时 m_attackTarget 仍指向旧对象或为空。
    // 直接解引用会导致空指针/悬空指针崩溃（Tick_WithoutTarget_DoesNotCrash
    // 即复现此路径：未调用 shouldExecute 直接 tick，m_attackTarget 为空）。
    LivingEntity* currentTarget = m_slime->attackTarget();
    if (currentTarget == nullptr) {
        return false;
    }

    if (!currentTarget->isAlive()) {
        return false;
    }

    return currentTarget == m_attackTarget;
}

void SlimeAttackGoal::startExecuting()
{
    m_attackTimer = ATTACK_DURATION;
    m_slime->lookAt(*m_attackTarget, 10.0f, 10.0f);
}

void SlimeAttackGoal::tick()
{
    --m_attackTimer;

    // 与 MC 1.21.11 SlimeAttackGoal.tick 对齐：每帧重新获取目标并做空检查。
    // 原实现直接解引用缓存的 m_attackTarget，若目标在执行期间被清除
    // （Tick_WithoutTarget_DoesNotCrash 测试即此场景：无目标直接 tick）
    // 会触发空指针解引用崩溃。
    LivingEntity* target = m_slime->attackTarget();
    if (target != nullptr) {
        m_slime->lookAt(*target, 10.0f, 10.0f);

        if (m_slime->moveController() != nullptr) {
            m_slime->moveController()->setMoveTo(target->x(), target->y(), target->z(), 1.0);
        }
    }
}

// ============================================================================
// SlimeFaceRandomGoal
// ============================================================================

SlimeFaceRandomGoal::SlimeFaceRandomGoal(SlimeEntity* slime)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_slime(slime)
    , m_chosenDegrees(0.0f)
    , m_nextRandomizeTime(0)
{
    MC_ASSERT_RELEASE(slime != nullptr);
}

bool SlimeFaceRandomGoal::shouldExecute()
{
    LivingEntity* target = m_slime->attackTarget();
    if (target != nullptr) {
        return false;
    }

    // 漂浮效果下也会随机转向（与 MC 一致）
    return m_slime->onGround() || m_slime->isInWater() || m_slime->isInLava() ||
        m_slime->hasEffect(entity::effect::EffectType::Levitation);
}

void SlimeFaceRandomGoal::tick()
{
    if (m_nextRandomizeTime <= 0) {
        math::Random& rng = m_slime->getRandom();
        m_nextRandomizeTime = RANDOMIZE_TIME_MIN + rng.nextInt(RANDOMIZE_TIME_RANGE);
        m_chosenDegrees = rng.nextFloat() * 360.0f;
    }

    --m_nextRandomizeTime;

    m_slime->setRotation(m_chosenDegrees, m_slime->pitch());
}

// ============================================================================
// SlimeHopGoal
// ============================================================================

SlimeHopGoal::SlimeHopGoal(SlimeEntity* slime)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Jump, GoalFlag::Move})
    , m_slime(slime)
{
    MC_ASSERT_RELEASE(slime != nullptr);
}

bool SlimeHopGoal::shouldExecute()
{
    return !m_slime->isRiding();
}

void SlimeHopGoal::tick()
{
    m_slime->setAIMoveSpeed(static_cast<f32>(
        1.0 * m_slime->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, DEFAULT_MOVEMENT_SPEED)));
}

} // namespace mc::entity::ai::goal
