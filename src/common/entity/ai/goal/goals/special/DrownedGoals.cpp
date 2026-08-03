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

#include "DrownedGoals.hpp"

#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/WorldConstants.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../../world/block/Material.hpp"
#include "../../../../combat/DifficultyHelper.hpp"
#include "../../../../core/CreatureEntity.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/monster/undead/DrownedEntity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../controller/MovementController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/special/MoveToBlockGoal.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <limits>

namespace mc::entity::ai::goal {

// ============================================================================
// DrownedGoToWaterGoal
// ============================================================================

DrownedGoToWaterGoal::DrownedGoToWaterGoal(DrownedEntity* drowned, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_drowned(drowned)
    , m_speed(speed)
{
    MC_ASSERT(drowned != nullptr);
}

bool DrownedGoToWaterGoal::shouldExecute()
{
    if (m_drowned == nullptr || m_drowned->world() == nullptr) {
        return false;
    }

    // 仅在白天（室外明亮）且不在水中时激活
    if (m_drowned->isInWater() || !m_drowned->world()->isBrightOutside()) {
        return false;
    }

    return _findWater();
}

bool DrownedGoToWaterGoal::shouldContinueExecuting()
{
    if (m_drowned == nullptr) {
        return false;
    }

    // 已进入水中则停止
    if (m_drowned->isInWater()) {
        return false;
    }

    // 仍需找到水源且导航器有路径
    auto* mob = dynamic_cast<MobEntity*>(m_drowned);
    if (mob == nullptr || mob->navigator() == nullptr) {
        return false;
    }

    return m_foundWater && mob->navigator()->hasPath();
}

void DrownedGoToWaterGoal::startExecuting()
{
    if (m_drowned != nullptr && m_foundWater) {
        m_drowned->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
    }
}

void DrownedGoToWaterGoal::resetTask()
{
    m_foundWater = false;
}

void DrownedGoToWaterGoal::tick()
{
    if (m_drowned == nullptr) {
        return;
    }

    // 如果已经进入水中，清除导航路径
    if (m_drowned->isInWater()) {
        auto* mob = dynamic_cast<MobEntity*>(m_drowned);
        if (mob != nullptr && mob->navigator() != nullptr) {
            mob->navigator()->clearPath();
        }
        m_foundWater = false;
    }
}

bool DrownedGoToWaterGoal::_findWater()
{
    if (m_drowned == nullptr || m_drowned->world() == nullptr) {
        return false;
    }

    IWorld* world = m_drowned->world();
    BlockPos centerPos(static_cast<i32>(std::floor(m_drowned->x())),
        static_cast<i32>(std::floor(m_drowned->y())),
        static_cast<i32>(std::floor(m_drowned->z())));

    f64 closestDistSq = std::numeric_limits<f64>::max();
    bool found = false;

    math::Random& rng = m_drowned->getRandom();

    // 随机采样 10 个位置，在 -10..+10 X, -6..+2 Y, -10..+10 Z 范围内
    for (i32 i = 0; i < 10; ++i) {
        i32 dx = rng.nextInt(-SEARCH_RANGE_HORIZONTAL, SEARCH_RANGE_HORIZONTAL);
        i32 dy = rng.nextInt(-6, 2);
        i32 dz = rng.nextInt(-SEARCH_RANGE_HORIZONTAL, SEARCH_RANGE_HORIZONTAL);

        BlockPos checkPos(centerPos.x + dx, centerPos.y + dy, centerPos.z + dz);

        if (!world->isWithinWorldBounds(checkPos.x, checkPos.y, checkPos.z)) {
            continue;
        }

        // 检查目标位置是否为水
        const BlockState* state = world->getBlockState(checkPos);
        if (state == nullptr || !state->getMaterial().isLiquid()) {
            continue;
        }

        f64 distSq = static_cast<f64>(dx * dx + dy * dy + dz * dz);
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            m_targetX = static_cast<f64>(checkPos.x) + 0.5;
            m_targetY = static_cast<f64>(checkPos.y);
            m_targetZ = static_cast<f64>(checkPos.z) + 0.5;
            found = true;
        }
    }

    m_foundWater = found;
    return found;
}

// ============================================================================
// DrownedTridentAttackGoal
// ============================================================================

DrownedTridentAttackGoal::DrownedTridentAttackGoal(
    DrownedEntity* drowned, f64 speed, i32 attackIntervalMin, f32 attackRadius)
    : RangedAttackGoal(drowned, speed, attackIntervalMin, attackIntervalMin, attackRadius)
    , m_drowned(drowned)
{
    MC_ASSERT(drowned != nullptr);
}

bool DrownedTridentAttackGoal::shouldExecute()
{
    // 仅当溺尸手持三叉戟时才激活
    if (!m_drowned->hasTrident()) {
        return false;
    }

    return RangedAttackGoal::shouldExecute();
}

void DrownedTridentAttackGoal::startExecuting()
{
    RangedAttackGoal::startExecuting();
    m_drowned->setAggroed(true);

    // 开始使用三叉戟（举起蓄力动画）
    m_drowned->setActiveHand(Hand::MainHand);
}

void DrownedTridentAttackGoal::resetTask()
{
    RangedAttackGoal::resetTask();
    m_drowned->setAggroed(false);
    m_drowned->stopActiveHand();
}

// ============================================================================
// DrownedAttackGoal
// ============================================================================

DrownedAttackGoal::DrownedAttackGoal(DrownedEntity* drowned, f64 speed, bool useLongMemory)
    : MeleeAttackGoal(drowned, speed, useLongMemory)
    , m_drowned(drowned)
{
    MC_ASSERT(drowned != nullptr);
}

bool DrownedAttackGoal::shouldExecute()
{
    if (!MeleeAttackGoal::shouldExecute()) {
        return false;
    }

    // 溺尸的近战攻击目标过滤：只有非白天或目标在水中时才攻击
    LivingEntity* target = m_creature->attackTarget();
    if (target == nullptr) {
        return false;
    }

    return m_drowned->okTarget(target);
}

bool DrownedAttackGoal::shouldContinueExecuting()
{
    if (!MeleeAttackGoal::shouldContinueExecuting()) {
        return false;
    }

    LivingEntity* target = m_creature->attackTarget();
    if (target == nullptr) {
        return false;
    }

    return m_drowned->okTarget(target);
}

// ============================================================================
// DrownedGoToBeachGoal
// ============================================================================

DrownedGoToBeachGoal::DrownedGoToBeachGoal(DrownedEntity* drowned, f64 speed)
    : MoveToBlockGoal(drowned, speed, 8, 2)
    , m_drowned(drowned)
{
    MC_ASSERT(drowned != nullptr);
}

bool DrownedGoToBeachGoal::shouldExecute()
{
    if (m_drowned == nullptr || m_drowned->world() == nullptr) {
        return false;
    }

    // 仅在夜晚、在水中、且接近海平面时激活
    if (m_drowned->world()->isBrightOutside()) {
        return false;
    }
    if (!m_drowned->isInWater()) {
        return false;
    }
    if (m_drowned->y() < static_cast<f64>(mc::world::SEA_LEVEL) - 3.0) {
        return false;
    }

    return MoveToBlockGoal::shouldExecute();
}

bool DrownedGoToBeachGoal::shouldContinueExecuting()
{
    return MoveToBlockGoal::shouldContinueExecuting();
}

void DrownedGoToBeachGoal::startExecuting()
{
    // 停止搜索陆地标志（开始前往海滩时不再向上游）
    m_drowned->setSearchingForLand(false);
    MoveToBlockGoal::startExecuting();
}

bool DrownedGoToBeachGoal::shouldMoveTo(IWorld* world, const BlockPos& pos)
{
    if (world == nullptr) {
        return false;
    }

    if (!world->isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        return false;
    }

    // 目标方块本身必须是固体（可以站在上面）
    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr || !state->isSolid()) {
        return false;
    }

    // 目标方块上方一格必须是空气
    BlockPos above1 = pos.up();
    const BlockState* above1State = world->getBlockState(above1);
    if (above1State == nullptr || !above1State->isAir()) {
        return false;
    }

    // 目标方块上方两格也必须是空气（确保溺尸可以站立）
    BlockPos above2 = above1.up();
    const BlockState* above2State = world->getBlockState(above2);
    if (above2State == nullptr || !above2State->isAir()) {
        return false;
    }

    return true;
}

// ============================================================================
// DrownedSwimUpGoal
// ============================================================================

DrownedSwimUpGoal::DrownedSwimUpGoal(DrownedEntity* drowned, f64 speed, i32 seaLevel)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_drowned(drowned)
    , m_speed(speed)
    , m_seaLevel(seaLevel)
{
    MC_ASSERT(drowned != nullptr);
}

bool DrownedSwimUpGoal::shouldExecute()
{
    if (m_drowned == nullptr || m_drowned->world() == nullptr) {
        return false;
    }

    // 仅在夜晚、在水中、且在海平面以下 2 格以上时激活
    if (m_drowned->world()->isBrightOutside()) {
        return false;
    }
    if (!m_drowned->isInWater()) {
        return false;
    }
    if (m_drowned->y() >= static_cast<f64>(m_seaLevel) - 2.0) {
        return false;
    }

    return true;
}

bool DrownedSwimUpGoal::shouldContinueExecuting()
{
    if (m_drowned == nullptr) {
        return false;
    }

    // 条件不再满足或被卡住时停止
    return shouldExecute() && !m_stuck;
}

void DrownedSwimUpGoal::startExecuting()
{
    m_drowned->setSearchingForLand(true);
    m_stuck = false;
}

void DrownedSwimUpGoal::resetTask()
{
    m_drowned->setSearchingForLand(false);
    m_stuck = false;
}

void DrownedSwimUpGoal::tick()
{
    if (m_drowned == nullptr || m_drowned->world() == nullptr) {
        return;
    }

    // 如果溺尸在海平面以下且导航完成或接近下一个路径点，尝试寻找向上的路径
    if (m_drowned->y() < static_cast<f64>(m_seaLevel) - 1.0) {
        auto* mob = dynamic_cast<MobEntity*>(m_drowned);
        if (mob == nullptr || mob->navigator() == nullptr) {
            return;
        }

        if (!mob->navigator()->hasPath()) {
            // 尝试找到海平面附近的随机位置并导航过去
            f64 targetX = m_drowned->x();
            f64 targetZ = m_drowned->z();
            f64 targetY = static_cast<f64>(m_seaLevel) - 1.0;

            // 使用随机偏移寻找可达位置
            math::Random& rng = m_drowned->getRandom();
            f64 angle = rng.nextFloat() * math::PI_DOUBLE * 2.0;
            f64 dist = static_cast<f64>(rng.nextInt(4, 8));

            f64 dx = -std::sin(angle) * dist;
            f64 dz = std::cos(angle) * dist;

            bool navigated = m_drowned->tryMoveTo(targetX + dx, targetY, targetZ + dz, m_speed);

            if (!navigated) {
                m_stuck = true;
            }
        }
    }
}

} // namespace mc::entity::ai::goal
