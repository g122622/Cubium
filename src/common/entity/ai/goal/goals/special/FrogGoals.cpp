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

#include "FrogGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/passive/frog/FrogEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc::entity::ai::goal {

// ============================================================================
// FrogAttackTargetGoal
// ============================================================================

FrogAttackTargetGoal::FrogAttackTargetGoal(FrogEntity* frog, f32 range, f32 chance)
    : m_frog(frog)
    , m_range(range)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Target});
}

bool FrogAttackTargetGoal::shouldExecute()
{
    if (m_frog == nullptr || !m_frog->isAlive()) {
        return false;
    }
    // 对齐 vanilla FrogAi.canAttack：求爱（繁殖）期间不捕食
    if (m_frog->isInLove()) {
        return false;
    }
    // 已有有效目标时不重复搜索
    if (LivingEntity* current = m_frog->attackTarget(); current != nullptr && current->isAlive()) {
        return true;
    }

    if (m_frog->getRandom().nextFloat() >= m_chance) {
        return false;
    }

    m_target = EntityUtils::findClosestEntity<LivingEntity>(
        m_frog->world(), m_frog->position(), m_range, m_frog, [](LivingEntity* candidate) -> bool {
            return candidate != nullptr && candidate->isAlive() && FrogEntity::canEat(*candidate);
        });
    return m_target != nullptr;
}

bool FrogAttackTargetGoal::shouldContinueExecuting()
{
    // 目标失效即放弃；其余由 FrogTongueGoal 负责实际执行
    LivingEntity* target = m_target.get();
    return m_frog != nullptr && target != nullptr && target->isAlive() && FrogEntity::canEat(*target);
}

void FrogAttackTargetGoal::startExecuting()
{
    if (m_frog != nullptr && m_target != nullptr) {
        m_frog->setAttackTarget(m_target.get());
    }
}

void FrogAttackTargetGoal::resetTask()
{
    m_target = nullptr;
    if (m_frog != nullptr) {
        m_frog->setAttackTarget(nullptr);
        m_frog->eraseTongueTarget();
    }
}

// ============================================================================
// FrogTongueGoal
// ============================================================================

FrogTongueGoal::FrogTongueGoal(FrogEntity* frog)
    : m_frog(frog)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool FrogTongueGoal::shouldExecute()
{
    if (m_frog == nullptr || !m_frog->isAlive()) {
        return false;
    }
    if (m_cooldown > 0) {
        --m_cooldown;
        return false;
    }
    if (m_frog->isInLove()) {
        return false;
    }
    LivingEntity* target = m_frog->attackTarget();
    if (target == nullptr || !target->isAlive() || !FrogEntity::canEat(*target)) {
        return false;
    }
    m_target = target;
    // 对齐 ShootTongue.checkExtraStartConditions：路径终点离目标超过 1.75 格视为不可达
    if (!_canPathfindToTarget(*target)) {
        _abandonTarget();
        return false;
    }
    return true;
}

bool FrogTongueGoal::shouldContinueExecuting()
{
    if (m_frog == nullptr || m_state == State::Done) {
        return false;
    }
    LivingEntity* target = m_target.get();
    return target != nullptr && target->isAlive();
}

void FrogTongueGoal::startExecuting()
{
    if (m_frog == nullptr) {
        return;
    }
    LivingEntity* target = m_target.get();
    if (target == nullptr) {
        return;
    }
    // 对齐 ShootTongue.start：注视目标、写入舌击目标、以 2.0 倍速走向目标
    m_frog->lookAt(*target);
    m_frog->setTongueTarget(target->id());
    if (auto* nav = m_frog->navigator()) {
        (void)nav->moveTo(*target, 2.0);
    }
    m_pathRecalcTimer = PATH_RECALC_INTERVAL;
    m_animationTimer = 0;
    m_state = State::MoveToTarget;
}

void FrogTongueGoal::resetTask()
{
    if (m_frog != nullptr) {
        m_frog->setAttackTarget(nullptr);
        m_frog->eraseTongueTarget();
        m_frog->setPose(EntityPose::Standing);
    }
    m_target = nullptr;
    m_state = State::Done;
    m_animationTimer = 0;
}

void FrogTongueGoal::tick()
{
    if (m_frog == nullptr) {
        return;
    }
    LivingEntity* target = m_target.get();
    if (target == nullptr) {
        m_state = State::Done;
        return;
    }
    m_frog->setTongueTarget(target->id());

    switch (m_state) {
        case State::MoveToTarget: {
            if (m_frog->distanceTo(*target) < EATING_DISTANCE) {
                // 对齐 vanilla：播放突舌音效、切换 USING_TONGUE 姿态、
                // 把目标以 0.75 倍速拉向自身，进入 6 tick 捕捉动画。
                m_frog->playSound(SoundEvents::FROG_TONGUE, 2.0f, 1.0f);
                m_frog->setPose(EntityPose::UsingTongue);
                const Vector3 pull = (m_frog->position() - target->position()).normalized() * EATING_MOVEMENT_FACTOR;
                target->setVelocity(pull);
                m_animationTimer = 0;
                m_state = State::CatchAnimation;
            } else if (m_pathRecalcTimer <= 0) {
                if (auto* nav = m_frog->navigator()) {
                    (void)nav->moveTo(*target, 2.0);
                }
                m_pathRecalcTimer = PATH_RECALC_INTERVAL;
            } else {
                --m_pathRecalcTimer;
            }
            break;
        }
        case State::CatchAnimation:
            if (m_animationTimer++ >= CATCH_ANIMATION_DURATION) {
                // 对齐 ShootTongue.eatEntity：捕捉动画结束后结算捕食
                m_animationTimer = 0;
                m_frog->eatTongueTarget();
                m_frog->setAttackTarget(nullptr);
                m_state = State::EatAnimation;
            }
            break;
        case State::EatAnimation:
            if (m_animationTimer++ >= TONGUE_ANIMATION_DURATION) {
                m_state = State::Done;
            }
            break;
        case State::Done:
            break;
    }
}

bool FrogTongueGoal::_canPathfindToTarget(LivingEntity& target) const
{
    if (m_frog == nullptr) {
        return false;
    }
    // 先请求一次到目标的路径（会写入导航器内部路径），再看路径终点离目标多远
    if (auto* nav = m_frog->navigator()) {
        (void)nav->moveTo(target, 2.0);
        const pathfinding::Path* path = nav->getPath();
        if (path == nullptr) {
            return false;
        }
        return path->distanceToTarget() < EATING_DISTANCE;
    }
    return false;
}

void FrogTongueGoal::_abandonTarget()
{
    if (m_frog != nullptr) {
        m_frog->setAttackTarget(nullptr);
        m_frog->eraseTongueTarget();
    }
    m_target = nullptr;
    m_state = State::Done;
    m_cooldown = COOLDOWN_TICKS;
}

} // namespace mc::entity::ai::goal
