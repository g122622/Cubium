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

#include "TurtleGoals.hpp"
#include "../../../../../item/Items.hpp"
#include "../../../../../item/core/ItemStack.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../../world/block/BlockTags.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/passive/special/TurtleEntity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../util/RandomPositionGenerator.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/world/WorldConstants.hpp"
#include <cmath>
#include <limits>

namespace mc::entity::ai::goal {

// ============================================================================
// TurtleGoHomeGoal
// ============================================================================

TurtleGoHomeGoal::TurtleGoHomeGoal(TurtleEntity* turtle, f64 speed)
    : m_turtle(turtle)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool TurtleGoHomeGoal::shouldExecute()
{
    if (m_turtle == nullptr) return false;

    // 幼年海龟不回家
    if (m_turtle->isChild()) {
        return false;
    }

    // 有蛋必须回家
    if (m_turtle->hasEgg()) {
        return m_turtle->hasHomePos();
    }

    // 1/700 概率检查
    mc::math::Random& rng = m_turtle->getRandom();
    if (rng.nextInt(RANDOM_TRIGGER_CHANCE) != 0) {
        return false;
    }

    // 距离出生地超过 64 格才触发
    if (!m_turtle->hasHomePos()) {
        return false;
    }

    const BlockPos& homePos = m_turtle->getHomePos();
    f64 dx = m_turtle->x() - (homePos.x + 0.5);
    f64 dy = m_turtle->y() - homePos.y;
    f64 dz = m_turtle->z() - (homePos.z + 0.5);
    f64 distSq = dx * dx + dy * dy + dz * dz;

    return distSq > HOME_DISTANCE_TRIGGER * HOME_DISTANCE_TRIGGER;
}

bool TurtleGoHomeGoal::shouldContinueExecuting()
{
    if (m_turtle == nullptr || !m_turtle->hasHomePos()) return false;

    // 距离出生地 > 7 格 AND 未放弃 AND 未超时
    const BlockPos& homePos = m_turtle->getHomePos();
    f64 dx = m_turtle->x() - (homePos.x + 0.5);
    f64 dy = m_turtle->y() - homePos.y;
    f64 dz = m_turtle->z() - (homePos.z + 0.5);
    f64 distSq = dx * dx + dy * dy + dz * dz;

    return distSq > HOME_DISTANCE_ARRIVE * HOME_DISTANCE_ARRIVE && !m_gaveUp && m_closeToHomeTimer <= MAX_TRAVEL_TIME;
}

void TurtleGoHomeGoal::startExecuting()
{
    if (m_turtle == nullptr) return;

    m_turtle->setGoingHome(true);
    m_gaveUp = false;
    m_closeToHomeTimer = 0;

    // 尝试找到路径
    if (!_tryFindPathToHome()) {
        m_gaveUp = true;
    }
}

void TurtleGoHomeGoal::resetTask()
{
    if (m_turtle) {
        m_turtle->setGoingHome(false);
        m_turtle->clearNavigation();
    }
    m_gaveUp = false;
    m_closeToHomeTimer = 0;
}

void TurtleGoHomeGoal::tick()
{
    if (m_turtle == nullptr || !m_turtle->hasHomePos()) return;

    const BlockPos& homePos = m_turtle->getHomePos();

    // 计算距离
    f64 dx = m_turtle->x() - (homePos.x + 0.5);
    f64 dy = m_turtle->y() - homePos.y;
    f64 dz = m_turtle->z() - (homePos.z + 0.5);
    f64 distSq = dx * dx + dy * dy + dz * dz;

    // 在 16 格范围内时增加计时器
    constexpr f64 CLOSE_THRESHOLD_SQ = 256.0; // 16 * 16
    bool closeToHome = distSq < CLOSE_THRESHOLD_SQ;
    if (closeToHome) {
        m_closeToHomeTimer++;
    }

    // 检查导航器
    auto* nav = m_turtle->navigator();
    if (nav && nav->noPath()) {
        // 尝试找到新路径
        if (!_tryFindPathToHome()) {
            m_gaveUp = true;
        }
    }

    // 看向出生地方向
    if (auto* lookCtrl = m_turtle->lookController()) {
        lookCtrl->setLookPosition(
            static_cast<f64>(homePos.x) + 0.5, static_cast<f64>(homePos.y), static_cast<f64>(homePos.z) + 0.5);
    }
}

bool TurtleGoHomeGoal::_tryFindPathToHome()
{
    if (m_turtle == nullptr || !m_turtle->hasHomePos()) return false;

    const BlockPos& homePos = m_turtle->getHomePos();

    // 使用 RandomPositionGenerator 找到通往出生地的路径
    Vector3 targetPos(
        static_cast<f64>(homePos.x) + 0.5, static_cast<f64>(homePos.y), static_cast<f64>(homePos.z) + 0.5);

    Vector3 movePos;
    // 首先尝试向目标方向移动
    if (util::RandomPositionGenerator::findRandomTargetTowards(m_turtle, 16, 3, targetPos, movePos)) {
        auto* nav = m_turtle->navigator();
        if (nav) {
            return nav->moveTo(movePos.x, movePos.y, movePos.z, m_speed);
        }
    }

    // 备用方案：直接导航到目标
    auto* nav = m_turtle->navigator();
    if (nav) {
        return nav->moveTo(targetPos.x, targetPos.y, targetPos.z, m_speed);
    }

    return false;
}

// ============================================================================
// TurtleLayEggGoal
// ============================================================================

TurtleLayEggGoal::TurtleLayEggGoal(TurtleEntity* turtle, f64 speed)
    : m_turtle(turtle)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool TurtleLayEggGoal::shouldExecute()
{
    if (m_turtle == nullptr) return false;

    // 必须有蛋
    if (!m_turtle->hasEgg()) {
        return false;
    }

    // 必须有出生地且距离出生地 <= 9 格
    if (!m_turtle->hasHomePos()) {
        return false;
    }

    const BlockPos& homePos = m_turtle->getHomePos();
    f64 dx = m_turtle->x() - (homePos.x + 0.5);
    f64 dy = m_turtle->y() - homePos.y;
    f64 dz = m_turtle->z() - (homePos.z + 0.5);
    f64 distSq = dx * dx + dy * dy + dz * dz;

    if (distSq > HOME_DISTANCE_MAX * HOME_DISTANCE_MAX) {
        return false;
    }

    // 搜索产卵位置
    return _findLayEggPosition();
}

bool TurtleLayEggGoal::shouldContinueExecuting()
{
    if (m_turtle == nullptr) return false;

    // 继续执行条件
    if (!m_turtle->hasEgg()) return false;
    if (!m_turtle->hasHomePos()) return false;

    const BlockPos& homePos = m_turtle->getHomePos();
    f64 dx = m_turtle->x() - (homePos.x + 0.5);
    f64 dy = m_turtle->y() - homePos.y;
    f64 dz = m_turtle->z() - (homePos.z + 0.5);
    f64 distSq = dx * dx + dy * dy + dz * dz;

    return distSq <= HOME_DISTANCE_MAX * HOME_DISTANCE_MAX && m_foundTarget && m_timeoutCounter <= MAX_TIMEOUT;
}

void TurtleLayEggGoal::startExecuting()
{
    if (m_turtle == nullptr) return;

    m_timeoutCounter = 0;

    // 移动到目标位置
    if (m_foundTarget) {
        m_turtle->tryMoveTo(static_cast<f64>(m_targetPos.x) + 0.5,
            static_cast<f64>(m_targetPos.y),
            static_cast<f64>(m_targetPos.z) + 0.5,
            m_speed);
    }
}

void TurtleLayEggGoal::resetTask()
{
    m_foundTarget = false;
    m_timeoutCounter = 0;
    if (m_turtle) {
        m_turtle->clearNavigation();
    }
}

void TurtleLayEggGoal::tick()
{
    if (m_turtle == nullptr || !m_foundTarget) return;

    m_timeoutCounter++;

    // 看向目标位置
    if (auto* lookCtrl = m_turtle->lookController()) {
        lookCtrl->setLookPosition(static_cast<f64>(m_targetPos.x) + 0.5,
            static_cast<f64>(m_targetPos.y) + 0.5,
            static_cast<f64>(m_targetPos.z) + 0.5);
    }

    // 检查是否到达目标位置
    f64 dx = m_turtle->x() - (m_targetPos.x + 0.5);
    f64 dy = m_turtle->y() - m_targetPos.y;
    f64 dz = m_turtle->z() - (m_targetPos.z + 0.5);
    f64 distSq = dx * dx + dz * dz; // 只检查水平距离

    // 到达目标位置且不在水中
    constexpr f64 ARRIVE_THRESHOLD_SQ = 2.25; // 1.5 * 1.5 = 2.25
    if (distSq < ARRIVE_THRESHOLD_SQ && !m_turtle->isInWater()) {
        // 开始产卵
        m_turtle->startLayEgg();
        m_foundTarget = false;
    } else {
        // 继续移动到目标位置
        auto* nav = m_turtle->navigator();
        if (nav && nav->noPath()) {
            m_turtle->tryMoveTo(static_cast<f64>(m_targetPos.x) + 0.5,
                static_cast<f64>(m_targetPos.y),
                static_cast<f64>(m_targetPos.z) + 0.5,
                m_speed);
        }
    }
}

bool TurtleLayEggGoal::_shouldMoveTo(const BlockPos& pos)
{
    if (m_turtle == nullptr || m_turtle->world() == nullptr) return false;

    IWorld* world = m_turtle->world();

    // 检查位置上方是否为空气
    const BlockState* aboveState = world->getBlockState(pos.up());
    if (aboveState == nullptr || !aboveState->isAir()) {
        return false;
    }

    // 检查位置下方是否为沙子
    const BlockState* belowState = world->getBlockState(pos);
    if (belowState == nullptr || !BlockTags::SAND().contains(*belowState)) {
        return false;
    }

    return true;
}

bool TurtleLayEggGoal::_findLayEggPosition()
{
    if (m_turtle == nullptr || m_turtle->world() == nullptr) return false;

    IWorld* world = m_turtle->world();
    BlockPos entityPos(static_cast<i32>(std::floor(m_turtle->x())),
        static_cast<i32>(std::floor(m_turtle->y())),
        static_cast<i32>(std::floor(m_turtle->z())));

    // 在周围搜索合适的位置
    for (i32 dx = -SEARCH_RANGE; dx <= SEARCH_RANGE; ++dx) {
        for (i32 dy = -3; dy <= 3; ++dy) {
            for (i32 dz = -SEARCH_RANGE; dz <= SEARCH_RANGE; ++dz) {
                BlockPos checkPos(entityPos.x + dx, entityPos.y + dy - 1, entityPos.z + dz);

                if (_shouldMoveTo(checkPos)) {
                    m_targetPos = checkPos.up(); // 目标是沙子上方的空气位置
                    m_foundTarget = true;
                    return true;
                }
            }
        }
    }

    m_foundTarget = false;
    return false;
}

// ============================================================================
// TurtleTravelGoal
// ============================================================================

TurtleTravelGoal::TurtleTravelGoal(TurtleEntity* turtle, f64 speed)
    : m_turtle(turtle)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool TurtleTravelGoal::shouldExecute()
{
    if (m_turtle == nullptr) return false;

    // 不在回家状态 AND 没有蛋 AND 在水中
    return !m_turtle->isGoingHome() && !m_turtle->hasEgg() && m_turtle->isInWater();
}

bool TurtleTravelGoal::shouldContinueExecuting()
{
    if (m_turtle == nullptr) return false;

    auto* nav = m_turtle->navigator();

    // 有路径 AND 未放弃 AND 不在回家 AND 不在恋爱 AND 没有蛋
    return nav != nullptr && !nav->noPath() && !m_gaveUp && !m_turtle->isGoingHome() && !m_turtle->isInLove() &&
        !m_turtle->hasEgg();
}

void TurtleTravelGoal::startExecuting()
{
    if (m_turtle == nullptr) return;

    // 设置随机旅行目标
    _setRandomTravelPos();

    m_turtle->setTravelling(true);
    m_gaveUp = false;

    // 尝试找到路径
    if (!_tryFindPathToTravelPos()) {
        m_gaveUp = true;
    }
}

void TurtleTravelGoal::resetTask()
{
    if (m_turtle) {
        m_turtle->setTravelling(false);
        m_turtle->clearNavigation();
    }
    m_gaveUp = false;
}

void TurtleTravelGoal::tick()
{
    if (m_turtle == nullptr) return;

    auto* nav = m_turtle->navigator();

    // 如果没有路径，尝试找到新路径
    if (nav && nav->noPath()) {
        if (!_tryFindPathToTravelPos()) {
            m_gaveUp = true;
        }
    }
}

void TurtleTravelGoal::_setRandomTravelPos()
{
    if (m_turtle == nullptr || m_turtle->world() == nullptr) return;

    mc::math::Random& rng = m_turtle->getRandom();

    // 在 512 格范围内随机选择目标
    i32 k = rng.nextInt(TRAVEL_RANGE * 2 + 1) - TRAVEL_RANGE;                   // X: -512 到 +512
    i32 l = rng.nextInt(TRAVEL_VERTICAL_RANGE * 2 + 1) - TRAVEL_VERTICAL_RANGE; // Y: -4 到 +4
    i32 i1 = rng.nextInt(TRAVEL_RANGE * 2 + 1) - TRAVEL_RANGE;                  // Z: -512 到 +512

    // 确保不会游到海平面以上
    if (static_cast<f64>(l) + m_turtle->y() > static_cast<f64>(world::SEA_LEVEL - 1)) {
        l = 0;
    }

    m_travelPos = BlockPos(static_cast<i32>(std::floor(m_turtle->x())) + k,
        static_cast<i32>(std::floor(m_turtle->y())) + l,
        static_cast<i32>(std::floor(m_turtle->z())) + i1);
}

bool TurtleTravelGoal::_tryFindPathToTravelPos()
{
    if (m_turtle == nullptr) return false;

    // 使用 RandomPositionGenerator 找到通往旅行目标的路径
    Vector3 targetPos(
        static_cast<f64>(m_travelPos.x) + 0.5, static_cast<f64>(m_travelPos.y), static_cast<f64>(m_travelPos.z) + 0.5);

    Vector3 movePos;
    bool found = util::RandomPositionGenerator::findRandomTargetTowards(m_turtle, 16, 3, targetPos, movePos);

    if (found) {
        // 检查区域是否已加载
        i32 checkX = static_cast<i32>(std::floor(movePos.x));
        i32 checkZ = static_cast<i32>(std::floor(movePos.z));
        constexpr i32 CHECK_RANGE = 34;

        IWorld* world = m_turtle->world();
        // 检查目标位置是否在世界范围内
        if (world && !world->isWithinWorldBounds(checkX - CHECK_RANGE, 0, checkZ - CHECK_RANGE)) {
            found = false;
        }
    }

    if (!found) {
        // 备用方案：直接导航到目标
        movePos = targetPos;
    }

    auto* nav = m_turtle->navigator();
    if (nav) {
        return nav->moveTo(movePos.x, movePos.y, movePos.z, m_speed);
    }

    return false;
}

// ============================================================================
// TurtleGoToWaterGoal
// ============================================================================

TurtleGoToWaterGoal::TurtleGoToWaterGoal(TurtleEntity* turtle, f64 speed)
    : m_turtle(turtle)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool TurtleGoToWaterGoal::shouldExecute()
{
    if (m_turtle == nullptr) return false;

    // 幼龟不在水中时触发
    if (m_turtle->isChild() && !m_turtle->isInWater()) {
        return _findWater();
    }

    // 成龟条件：不在回家 AND 不在水中 AND 没有蛋
    if (m_turtle->isGoingHome() || m_turtle->isInWater() || m_turtle->hasEgg()) {
        return false;
    }

    return _findWater();
}

bool TurtleGoToWaterGoal::shouldContinueExecuting()
{
    if (m_turtle == nullptr) return false;

    // 仍在陆地 AND 超时 <= 1200 AND 目标仍是水
    if (m_turtle->isInWater()) {
        return false;
    }

    if (m_timeoutCounter > MAX_TIMEOUT) {
        return false;
    }

    if (!m_foundWater) {
        return false;
    }

    // 检查路径是否完成
    auto* nav = m_turtle->navigator();
    return nav != nullptr && nav->hasPath();
}

void TurtleGoToWaterGoal::startExecuting()
{
    if (m_turtle == nullptr || !m_foundWater) return;

    m_timeoutCounter = 0;
    m_turtle->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
}

void TurtleGoToWaterGoal::resetTask()
{
    m_foundWater = false;
    m_timeoutCounter = 0;
    if (m_turtle) {
        m_turtle->clearNavigation();
    }
}

void TurtleGoToWaterGoal::tick()
{
    if (m_turtle == nullptr) return;

    m_timeoutCounter++;

    // 如果已经在水中，停止
    if (m_turtle->isInWater()) {
        m_turtle->clearNavigation();
        m_foundWater = false;
        return;
    }

    // 继续移动到水源
    auto* nav = m_turtle->navigator();
    if (nav && nav->noPath() && m_foundWater) {
        m_turtle->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
    }
}

bool TurtleGoToWaterGoal::_findWater()
{
    if (m_turtle == nullptr || m_turtle->world() == nullptr) return false;

    IWorld* world = m_turtle->world();
    BlockPos entityPos(static_cast<i32>(std::floor(m_turtle->x())),
        static_cast<i32>(std::floor(m_turtle->y())),
        static_cast<i32>(std::floor(m_turtle->z())));

    // 幼龟使用更大的搜索范围
    i32 horizontalRange = m_turtle->isChild() ? 2 : SEARCH_RANGE_HORIZONTAL;

    f64 closestDistSq = std::numeric_limits<f64>::max();
    bool found = false;

    for (i32 dx = -horizontalRange; dx <= horizontalRange; ++dx) {
        for (i32 dy = -SEARCH_RANGE_VERTICAL; dy <= SEARCH_RANGE_VERTICAL; ++dy) {
            for (i32 dz = -horizontalRange; dz <= horizontalRange; ++dz) {
                BlockPos checkPos(entityPos.x + dx, entityPos.y + dy, entityPos.z + dz);

                if (!world->isWithinWorldBounds(checkPos.x, checkPos.y, checkPos.z)) {
                    continue;
                }

                if (world->isWaterAt(checkPos)) {
                    f64 distSq = static_cast<f64>(dx * dx + dy * dy + dz * dz);
                    if (distSq < closestDistSq) {
                        closestDistSq = distSq;
                        m_targetX = static_cast<f64>(checkPos.x) + 0.5;
                        m_targetY = static_cast<f64>(checkPos.y);
                        m_targetZ = static_cast<f64>(checkPos.z) + 0.5;
                        found = true;
                    }
                }
            }
        }
    }

    m_foundWater = found;
    return found;
}

// ============================================================================
// TurtleMateGoal
// ============================================================================

TurtleMateGoal::TurtleMateGoal(TurtleEntity* turtle, f64 speed)
    : BreedGoal(turtle, speed)
    , m_turtle(turtle)
{}

bool TurtleMateGoal::shouldExecute()
{
    // 海龟繁殖额外条件：没有蛋
    if (m_turtle != nullptr && m_turtle->hasEgg()) {
        return false;
    }
    return BreedGoal::shouldExecute();
}

// ============================================================================
// TurtlePanicGoal
// ============================================================================

TurtlePanicGoal::TurtlePanicGoal(TurtleEntity* turtle, f64 speed)
    : PanicGoal(turtle, speed)
    , m_turtle(turtle)
{}

bool TurtlePanicGoal::shouldExecute()
{
    // 如果正在被攻击或着火，优先找水
    return PanicGoal::shouldExecute();
}

// ============================================================================
// TurtleTemptGoal
// ============================================================================

TurtleTemptGoal::TurtleTemptGoal(TurtleEntity* turtle, f64 speed)
    : TemptGoal(turtle, speed, isSeagrass, false)
{}

bool TurtleTemptGoal::isSeagrass(const ItemStack& stack)
{
    const Item* item = stack.getItem();
    return item != nullptr && item == Items::SEAGRASS;
}

// ============================================================================
// TurtleWanderGoal
// ============================================================================

TurtleWanderGoal::TurtleWanderGoal(TurtleEntity* turtle, f64 speed, i32 chance)
    : RandomWalkingGoal(turtle, speed, chance)
    , m_turtle(turtle)
{}

bool TurtleWanderGoal::shouldExecute()
{
    if (m_turtle == nullptr) return false;

    // 不在水中 AND 不在回家 AND 没有蛋
    if (m_turtle->isInWater()) {
        return false;
    }

    if (m_turtle->isGoingHome()) {
        return false;
    }

    if (m_turtle->hasEgg()) {
        return false;
    }

    return RandomWalkingGoal::shouldExecute();
}

} // namespace mc::entity::ai::goal
