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

#include "MovementGoals.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace mc::math;

// ==================== WaterAvoidingRandomWalkingGoal ====================

WaterAvoidingRandomWalkingGoal::WaterAvoidingRandomWalkingGoal(CreatureEntity* creature, f64 speed)
    : WaterAvoidingRandomWalkingGoal(creature, speed, 0.001f)
{}

WaterAvoidingRandomWalkingGoal::WaterAvoidingRandomWalkingGoal(CreatureEntity* creature, f64 speed, f32 chance)
    : m_creature(creature)
    , m_speed(speed)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool WaterAvoidingRandomWalkingGoal::shouldExecute()
{
    if (!m_creature) return false;

    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // 检查执行概率
    if (m_chance > 0.0f) {
        math::Random& rng = m_creature->getRandom();
        if (rng.nextFloat() >= m_chance) return false;
    }

    // 获取随机位置
    return getRandomPosition();
}

bool WaterAvoidingRandomWalkingGoal::shouldContinueExecuting()
{
    if (!m_creature) return false;

    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // 检查超时
    if (m_timeout <= 0) return false;

    // 检查导航路径
    auto* nav = m_creature->navigator();
    if (nav && nav->hasPath() && !nav->isDone()) {
        return true;
    }

    // 检查移动控制器
    auto* moveCtrl = m_creature->moveController();
    if (moveCtrl && moveCtrl->isUpdating()) {
        return true;
    }

    return false;
}

void WaterAvoidingRandomWalkingGoal::startExecuting()
{
    if (m_creature) {
        m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
        m_timeout = MAX_TIMEOUT;
        m_isRunning = true;
    }
}

void WaterAvoidingRandomWalkingGoal::resetTask()
{
    if (m_creature) {
        m_creature->clearNavigation();
    }
    m_isRunning = false;
    m_timeout = 0;
}

void WaterAvoidingRandomWalkingGoal::tick()
{
    if (m_timeout > 0) {
        m_timeout--;
    }
}

bool WaterAvoidingRandomWalkingGoal::getRandomPosition()
{
    if (!m_creature) return false;

    math::Random& rng = m_creature->getRandom();

    // 尝试多次找到合适的位置
    for (i32 attempt = 0; attempt < 10; ++attempt) {
        // 随机方向
        f32 angle = rng.nextFloat() * mc::math::TWO_PI;
        f32 distance = 10.0f + rng.nextFloat() * 10.0f;

        f64 x = m_creature->x() + std::cos(angle) * distance;
        f64 z = m_creature->z() + std::sin(angle) * distance;
        f64 y = m_creature->y();

        // 检查是否在水或岩浆中
        if (isInWaterOrLava(x, y, z)) {
            continue;
        }

        m_targetX = x;
        m_targetY = y;
        m_targetZ = z;
        return true;
    }

    return false;
}

bool WaterAvoidingRandomWalkingGoal::isInWaterOrLava(f64 x, f64 y, f64 z) const
{
    if (!m_creature || !m_creature->world()) {
        return false;
    }

    IWorld* world = m_creature->world();
    const i32 blockX = floorTo<i32>(x);
    const i32 blockY = floorTo<i32>(y);
    const i32 blockZ = floorTo<i32>(z);
    const BlockPos pos(blockX, blockY, blockZ);
    return world->isWaterAt(pos) || world->isLavaAt(pos);
}

// ==================== LeapAtTargetGoal ====================

LeapAtTargetGoal::LeapAtTargetGoal(MobEntity* mob, f32 leapHeight)
    : m_mob(mob)
    , m_leapHeight(leapHeight)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Jump});
}

bool LeapAtTargetGoal::shouldExecute()
{
    if (!m_mob) return false;

    // 获取攻击目标
    LivingEntity* target = m_mob->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    // 检查距离
    f64 distSq = m_mob->distanceSqTo(*target);
    if (distSq < MIN_DISTANCE * MIN_DISTANCE || distSq > MAX_DISTANCE * MAX_DISTANCE) {
        return false;
    }

    // 检查是否已经在地面
    if (!m_mob->onGround()) {
        return false;
    }

    m_target = target;
    m_leaped = false;
    return true;
}

bool LeapAtTargetGoal::shouldContinueExecuting()
{
    // 跳跃后立即结束
    return !m_leaped;
}

void LeapAtTargetGoal::startExecuting()
{
    if (!m_mob || !m_target) return;

    // 计算跳跃向量
    f64 dx = m_target->x() - m_mob->x();
    f64 dz = m_target->z() - m_mob->z();
    f64 distSq = dx * dx + dz * dz;

    if (distSq < 0.000001) {
        m_leaped = true;
        return;
    }

    f64 dist = std::sqrt(distSq);

    // 计算跳跃速度
    f64 motionX = (dx / dist) * 0.5;
    f64 motionZ = (dz / dist) * 0.5;

    // 设置速度
    m_mob->setVelocity(static_cast<f32>(motionX), m_leapHeight, static_cast<f32>(motionZ));
    m_mob->setJumping(true);
    m_leaped = true;
}

void LeapAtTargetGoal::resetTask()
{
    m_target = nullptr;
    m_leaped = false;
    if (m_mob) {
        m_mob->setJumping(false);
    }
}

// ==================== MoveTowardsTargetGoal ====================

MoveTowardsTargetGoal::MoveTowardsTargetGoal(CreatureEntity* creature, f64 speed, f32 maxTargetDistance)
    : m_creature(creature)
    , m_speed(speed)
    , m_maxTargetDistance(maxTargetDistance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool MoveTowardsTargetGoal::shouldExecute()
{
    if (!m_creature) return false;

    // 获取攻击目标
    LivingEntity* target = m_creature->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    // 检查距离是否在最大范围内
    f64 distSq = m_creature->distanceSqTo(*target);
    f32 maxDistSq = m_maxTargetDistance * m_maxTargetDistance;
    if (distSq > static_cast<f64>(maxDistSq)) {
        return false;
    }

    // 使用 RandomPositionGenerator 找到朝向目标的随机位置
    auto* world = m_creature->world();
    if (!world) return false;

    Vector3 targetPos = target->position();
    Vector3 randomPos;

    // 使用 RandomPositionGenerator 找到朝向目标的位置
    if (!entity::ai::util::RandomPositionGenerator::findRandomTargetTowards(m_creature, 16, 7, targetPos, randomPos)) {
        return false;
    }

    m_targetEntity = target;
    m_targetX = randomPos.x;
    m_targetY = randomPos.y;
    m_targetZ = randomPos.z;
    return true;
}

bool MoveTowardsTargetGoal::shouldContinueExecuting()
{
    if (!m_creature || !m_targetEntity) return false;

    // 检查路径是否还在进行中
    auto* nav = m_creature->navigator();
    if (!nav || nav->noPath()) {
        return false;
    }

    // 检查目标是否还活着
    if (!m_targetEntity->isAlive()) {
        return false;
    }

    // 检查距离是否仍在范围内
    f64 distSq = m_creature->distanceSqTo(*m_targetEntity);
    f32 maxDistSq = m_maxTargetDistance * m_maxTargetDistance;
    return distSq < static_cast<f64>(maxDistSq);
}

void MoveTowardsTargetGoal::startExecuting()
{
    if (m_creature) {
        m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
    }
}

void MoveTowardsTargetGoal::resetTask()
{
    m_targetEntity = nullptr;
    if (m_creature) {
        m_creature->clearNavigation();
    }
}

// ==================== MoveTowardsRestrictionGoal ====================

MoveTowardsRestrictionGoal::MoveTowardsRestrictionGoal(CreatureEntity* creature, f64 speed)
    : m_creature(creature)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool MoveTowardsRestrictionGoal::shouldExecute()
{
    if (!m_creature) {
        return false;
    }

    // 被骑乘时不执行
    if (m_creature->isBeingRidden()) {
        return false;
    }

    // 检查是否设有家位置
    if (!m_creature->hasHome()) {
        return false;
    }

    // 如果当前已在家范围内，不需要移动
    if (m_creature->isWithinHomeDistanceCurrentPosition()) {
        return false;
    }

    // 向家位置生成随机目标位置
    BlockPos homePos = m_creature->homePosition();
    Vector3 homeCenter(
        static_cast<f32>(homePos.x) + 0.5f, static_cast<f32>(homePos.y) + 0.5f, static_cast<f32>(homePos.z) + 0.5f);

    Vector3 targetPos;
    if (!entity::ai::util::RandomPositionGenerator::findRandomTargetTowards(
            m_creature, XZ_RANGE, Y_RANGE, homeCenter, targetPos)) {
        return false;
    }

    m_targetX = targetPos.x;
    m_targetY = targetPos.y;
    m_targetZ = targetPos.z;
    return true;
}

bool MoveTowardsRestrictionGoal::shouldContinueExecuting()
{
    if (!m_creature) {
        return false;
    }

    // 已经回到家园范围内，停止
    if (m_creature->hasHome() && m_creature->isWithinHomeDistanceCurrentPosition()) {
        return false;
    }

    // 导航仍在进行中
    auto* nav = m_creature->navigator();
    return nav && nav->hasPath() && !nav->isDone();
}

void MoveTowardsRestrictionGoal::startExecuting()
{
    if (m_creature) {
        static_cast<void>(m_creature->navigator()->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
    m_pathRecalcTimer = 0;
}

void MoveTowardsRestrictionGoal::resetTask()
{
    if (m_creature) {
        m_creature->clearNavigation();
    }
    m_pathRecalcTimer = 0;
}

void MoveTowardsRestrictionGoal::tick()
{
    if (m_pathRecalcTimer > 0) {
        --m_pathRecalcTimer;
        return;
    }

    // 定期重新计算路径
    _recalculatePath();
    m_pathRecalcTimer = PATH_RECALC_INTERVAL;
}

void MoveTowardsRestrictionGoal::_recalculatePath()
{
    if (!m_creature || !m_creature->hasHome()) {
        return;
    }

    // 生成朝向家位置的新路径
    BlockPos homePos = m_creature->homePosition();
    Vector3 homeCenter(
        static_cast<f32>(homePos.x) + 0.5f, static_cast<f32>(homePos.y) + 0.5f, static_cast<f32>(homePos.z) + 0.5f);

    Vector3 targetPos;
    if (entity::ai::util::RandomPositionGenerator::findRandomTargetTowards(
            m_creature, XZ_RANGE, Y_RANGE, homeCenter, targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        static_cast<void>(m_creature->navigator()->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
}

} // namespace mc::entity::ai::goal
