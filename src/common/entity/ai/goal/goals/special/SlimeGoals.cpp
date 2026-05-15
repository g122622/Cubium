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
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/monster/basic/SlimeEntity.hpp"
#include "../../../controller/JumpController.hpp"
#include "../../../controller/MovementController.hpp"

namespace mc::entity::ai::goal {

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
    if (m_slime == nullptr) {
        return false;
    }

    // MC 1.16.5: 当史莱姆在水中或岩浆中时执行
    return m_slime->isInWater() || m_slime->isInLava();
}

void SlimeFloatGoal::tick()
{
    if (m_slime == nullptr) {
        return;
    }

    // MC 1.16.5 FloatGoal.tick()
    // 80% 概率触发跳跃
    math::Random rng = m_slime->getRandom();
    if (rng.nextFloat() < JUMP_CHANCE) {
        m_slime->jumpController()->setJumping();
    }

    // 设置游泳速度
    // MC 1.16.5: 使用 MoveHelperController.setSpeed(1.2D)
    // 在我们的实现中，使用 setAIMoveSpeed 来设置移动速度
    m_slime->setAIMoveSpeed(static_cast<f32>(SWIM_SPEED * m_slime->getAttributeValue(
        entity::attribute::Attributes::MOVEMENT_SPEED, 0.2)));
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
    if (m_slime == nullptr) {
        return false;
    }

    // MC 1.16.5 AttackGoal.shouldExecute()
    LivingEntity* target = m_slime->attackTarget();
    if (target == nullptr) {
        return false;
    }

    if (!target->isAlive()) {
        return false;
    }

    // 检查目标是否是创造模式玩家
    // MC 1.16.5: 如果目标是玩家且处于无敌状态，不执行
    // 注：Player 继承自 LivingEntity，可以通过 dynamic_cast 检查
    // 这里简化处理，直接返回 true
    m_attackTarget = target;
    return true;
}

bool SlimeAttackGoal::shouldContinueExecuting()
{
    if (m_slime == nullptr || m_attackTarget == nullptr) {
        return false;
    }

    // 检查攻击计时器
    if (m_attackTimer <= 0) {
        return false;
    }

    // 检查目标是否仍然存活
    if (!m_attackTarget->isAlive()) {
        return false;
    }

    // 检查目标是否仍是当前攻击目标
    LivingEntity* currentTarget = m_slime->attackTarget();
    if (currentTarget != m_attackTarget) {
        return false;
    }

    return true;
}

void SlimeAttackGoal::startExecuting()
{
    // MC 1.16.5: 设置攻击持续时间
    m_attackTimer = ATTACK_DURATION;
    m_slime->lookAt(*m_attackTarget, 10.0f, 10.0f);
}

void SlimeAttackGoal::tick()
{
    if (m_slime == nullptr || m_attackTarget == nullptr) {
        return;
    }

    // 减少攻击计时器
    --m_attackTimer;

    // MC 1.16.5 AttackGoal.tick()
    // 面向攻击目标
    m_slime->lookAt(*m_attackTarget, 10.0f, 10.0f);

    // 设置移动方向
    // MC 1.16.5: 使用 MoveHelperController.setDirection(yaw, canDamagePlayer())
    // aggressive = canDamagePlayer() 表示是否可以伤害玩家
    f32 yaw = m_slime->yaw();

    // 史莱姆通过跳跃和移动控制器来移动
    // 设置移动方向和速度
    if (m_slime->moveController() != nullptr) {
        m_slime->moveController()->setMoveTo(
            m_attackTarget->x(),
            m_attackTarget->y(),
            m_attackTarget->z(),
            1.0);
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
    if (m_slime == nullptr) {
        return false;
    }

    // MC 1.16.5 FaceRandomGoal.shouldExecute()
    // 没有攻击目标
    LivingEntity* target = m_slime->attackTarget();
    if (target != nullptr) {
        return false;
    }

    // 在地面、水中、岩浆中或有漂浮效果
    if (m_slime->onGround()) {
        return true;
    }
    if (m_slime->isInWater()) {
        return true;
    }
    if (m_slime->isInLava()) {
        return true;
    }
    // 注：漂浮效果检查需要 Potion 系统，暂时跳过
    // if (m_slime->isPotionActive(Effects::LEVITATION)) return true;

    return false;
}

void SlimeFaceRandomGoal::tick()
{
    if (m_slime == nullptr) {
        return;
    }

    // MC 1.16.5 FaceRandomGoal.tick()
    if (m_nextRandomizeTime <= 0) {
        // 设置下一次随机时间：40-99 tick
        math::Random rng = m_slime->getRandom();
        m_nextRandomizeTime = RANDOMIZE_TIME_MIN + rng.nextInt(RANDOMIZE_TIME_RANGE);

        // 随机选择面向角度：0-359 度
        m_chosenDegrees = rng.nextFloat() * 360.0f;
    }

    --m_nextRandomizeTime;

    // 史莱姆通过设置朝向来改变移动方向
    // MC 1.16.5: MoveHelperController.setDirection(chosenDegrees, false)
    // aggressive = false 表示不攻击玩家
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
    if (m_slime == nullptr) {
        return false;
    }

    // MC 1.16.5 HopGoal.shouldExecute()
    // 只要不是骑乘状态就执行
    return !m_slime->isRiding();
}

void SlimeHopGoal::tick()
{
    if (m_slime == nullptr) {
        return;
    }

    // MC 1.16.5 HopGoal.tick()
    // 设置移动速度为 1.0
    // MC 1.16.5: 使用 MoveHelperController.setSpeed(1.0D)
    // 在我们的实现中，使用 setAIMoveSpeed 来设置移动速度
    m_slime->setAIMoveSpeed(static_cast<f32>(1.0 * m_slime->getAttributeValue(
        entity::attribute::Attributes::MOVEMENT_SPEED, 0.2)));
}

} // namespace mc::entity::ai::goal
