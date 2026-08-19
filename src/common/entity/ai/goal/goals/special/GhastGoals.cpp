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

#include "GhastGoals.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// GhastRandomFlyGoal
// ============================================================================

GhastRandomFlyGoal::GhastRandomFlyGoal(GhastEntity* ghast)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_ghast(ghast)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

bool GhastRandomFlyGoal::shouldExecute()
{
    if (m_ghast == nullptr || m_ghast->moveController() == nullptr) {
        return false;
    }

    auto* moveController = m_ghast->moveController();

    // 条件1: 移动控制器空闲（没有目标）
    if (!moveController->isUpdating()) {
        return true;
    }

    // 条件2: 目标距离太近（小于1格）
    f64 dx = moveController->getX() - m_ghast->x();
    f64 dy = moveController->getY() - m_ghast->y();
    f64 dz = moveController->getZ() - m_ghast->z();
    f64 distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < MIN_DISTANCE_SQ) {
        return true;
    }

    // 条件3: 目标距离太远（超过60格）
    if (distSq > MAX_DISTANCE_SQ) {
        return true;
    }

    return false;
}

void GhastRandomFlyGoal::startExecuting()
{
    if (m_ghast == nullptr || m_ghast->moveController() == nullptr) {
        return;
    }

    // 在当前位置周围选择随机目标点
    math::Random& rng = m_ghast->getRandom();

    // 目标范围: 当前位置 ±16格
    f64 targetX = m_ghast->x() + (rng.nextFloat() * 2.0 - 1.0) * WANDER_RANGE;
    f64 targetY = m_ghast->y() + (rng.nextFloat() * 2.0 - 1.0) * WANDER_RANGE;
    f64 targetZ = m_ghast->z() + (rng.nextFloat() * 2.0 - 1.0) * WANDER_RANGE;

    m_ghast->moveController()->setMoveTo(targetX, targetY, targetZ, 1.0);
}

// ============================================================================
// GhastLookAroundGoal
// ============================================================================

GhastLookAroundGoal::GhastLookAroundGoal(GhastEntity* ghast)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_ghast(ghast)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

void GhastLookAroundGoal::tick()
{
    if (m_ghast == nullptr) {
        return;
    }

    // 根据是否有攻击目标更新朝向
    LivingEntity* attackTarget = m_ghast->attackTarget();

    if (attackTarget == nullptr) {
        // 无攻击目标：朝向移动方向
        Vector3 velocity = m_ghast->velocity();
        f32 targetYaw = -static_cast<f32>(math::toDegrees(std::atan2(velocity.x, velocity.z)));
        m_ghast->setRotation(targetYaw, m_ghast->pitch());
        m_ghast->setRenderYawOffset(targetYaw);
    } else {
        // 有攻击目标：朝向攻击目标
        f64 dx = attackTarget->x() - m_ghast->x();
        f64 dz = attackTarget->z() - m_ghast->z();
        f64 distSq = dx * dx + dz * dz;

        if (distSq < LOOK_RANGE_SQ) {
            // 在攻击范围内，朝向目标
            f32 targetYaw = -static_cast<f32>(math::toDegrees(std::atan2(dx, dz)));
            m_ghast->setRotation(targetYaw, m_ghast->pitch());
            m_ghast->setRenderYawOffset(targetYaw);
        }
    }
}

// ============================================================================
// GhastFireballAttackGoal
// ============================================================================

GhastFireballAttackGoal::GhastFireballAttackGoal(GhastEntity* ghast)
    : Goal()
    , m_ghast(ghast)
    , m_target(nullptr)
    , m_attackTimer(0)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

bool GhastFireballAttackGoal::shouldExecute()
{
    if (m_ghast == nullptr) {
        return false;
    }

    // 有攻击目标时执行
    LivingEntity* target = m_ghast->attackTarget();
    return target != nullptr && target->isAlive();
}

void GhastFireballAttackGoal::startExecuting()
{
    // 重置攻击计时器
    m_attackTimer = 0;
}

void GhastFireballAttackGoal::resetTask()
{
    if (m_ghast == nullptr) {
        return;
    }

    // 清除攻击状态
    m_ghast->setCharging(false);
    m_target = nullptr;
}

void GhastFireballAttackGoal::tick()
{
    if (m_ghast == nullptr || m_ghast->world() == nullptr) {
        return;
    }

    m_target = m_ghast->attackTarget();

    if (m_target == nullptr || !m_target->isAlive()) {
        return;
    }

    f64 distSq = m_ghast->distanceSqTo(*m_target);

    // 在攻击范围内（64格）且能看到目标
    if (distSq < ATTACK_RANGE_SQ && m_ghast->canSee(*m_target)) {
        // 增加攻击计时器
        ++m_attackTimer;

        // 充能音效（第10 tick）
        if (m_attackTimer == CHARGE_SOUND_TICK && !m_ghast->isSilent()) {
            // 充能音效事件
            BlockPos pos(
                static_cast<i32>(m_ghast->x()), static_cast<i32>(m_ghast->y()), static_cast<i32>(m_ghast->z()));
            m_ghast->world()->playEvent(world::WorldEvents::GHAST_WARN_SOUND, pos, 0);
        }

        // 充能完成（第20 tick）
        if (m_attackTimer >= CHARGE_DURATION) {
            // 发射火球
            m_ghast->shootFireball();

            // 进入冷却
            m_attackTimer = -COOLDOWN_DURATION;
        }

        // 更新攻击状态（用于客户端动画）
        m_ghast->setCharging(m_attackTimer > 10);
    } else {
        // 不在攻击范围内，逐渐减少计时器
        if (m_attackTimer > 0) {
            --m_attackTimer;
        }
        m_ghast->setCharging(false);
    }
}

} // namespace mc::entity::ai::goal
