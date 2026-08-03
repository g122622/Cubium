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

#include "MoveToLavaGoal.hpp"
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../../world/fluid/Fluid.hpp"
#include "../../../../../world/fluid/FluidTags.hpp"
#include "../../../../core/CreatureEntity.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "MoveToBlockGoal.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace MoveToBlockGoalConstants;
using namespace MoveToLavaGoalConstants;

// ============================================================================
// MoveToBlockGoal 基类实现
// ============================================================================

MoveToBlockGoal::MoveToBlockGoal(CreatureEntity* creature, f64 speed, i32 searchLength)
    : MoveToBlockGoal(creature, speed, searchLength, 1)
{}

MoveToBlockGoal::MoveToBlockGoal(CreatureEntity* creature, f64 speed, i32 searchLength, i32 verticalSearchRange)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Jump})
    , m_creature(creature)
    , m_movementSpeed(speed)
    , m_searchLength(searchLength)
    , m_verticalSearchRange(verticalSearchRange)
{
    MC_ASSERT(creature != nullptr);
}

bool MoveToBlockGoal::shouldExecute()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 如果有延迟，递减并返回 false
    if (m_runDelay > 0) {
        --m_runDelay;
        return false;
    }

    // 设置新的随机延迟
    m_runDelay = getRunDelay();

    // 搜索目标方块
    return searchForDestination();
}

bool MoveToBlockGoal::shouldContinueExecuting()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 检查超时计数器和目标有效性
    // timeoutCounter >= -maxStayTicks && timeoutCounter <= MAX_TIMEOUT && shouldMoveTo(...)
    return m_timeoutCounter >= -m_maxStayTicks && m_timeoutCounter <= MAX_TIMEOUT &&
        shouldMoveTo(m_creature->world(), m_destinationBlock);
}

void MoveToBlockGoal::startExecuting()
{
    if (m_creature == nullptr) {
        return;
    }

    // 移动到目标
    moveToTarget();

    // 重置超时计数器
    m_timeoutCounter = 0;

    // 随机最大停留时间
    math::Random& rng = m_creature->getRandom();
    m_maxStayTicks = rng.nextInt(rng.nextInt(MAX_STAY_RANGE) + MAX_STAY_BASE) + MAX_STAY_BASE;
}

void MoveToBlockGoal::resetTask()
{
    m_isAboveDestination = false;
    m_timeoutCounter = 0;

    // 清除导航
    if (m_creature != nullptr) {
        auto* mob = dynamic_cast<MobEntity*>(m_creature);
        if (mob != nullptr && mob->navigator() != nullptr) {
            mob->navigator()->clearPath();
        }
    }
}

void MoveToBlockGoal::tick()
{
    if (m_creature == nullptr) {
        return;
    }

    BlockPos targetPos = getTargetPosition();

    // 检查是否到达目标
    if (!isWithinDistance(targetPos, getTargetDistanceSq())) {
        m_isAboveDestination = false;
        ++m_timeoutCounter;

        // 每隔一定时间重新导航
        if (shouldMove()) {
            moveToTarget();
        }
    } else {
        m_isAboveDestination = true;
        --m_timeoutCounter;
    }
}

bool MoveToBlockGoal::shouldMove() const
{
    // 默认: 每 40 tick 检查一次
    return m_timeoutCounter % DEFAULT_MOVE_INTERVAL == 0;
}

BlockPos MoveToBlockGoal::getTargetPosition() const
{
    // 默认返回方块上方
    return m_destinationBlock.up();
}

f64 MoveToBlockGoal::getTargetDistanceSq() const
{
    return 1.0;
}

i32 MoveToBlockGoal::getRunDelay()
{
    if (m_creature == nullptr) {
        return RUN_DELAY_BASE;
    }
    // 200 + random(200) = 200-400 tick
    math::Random& rng = m_creature->getRandom();
    return RUN_DELAY_BASE + rng.nextInt(RUN_DELAY_RANGE);
}

void MoveToBlockGoal::moveToTarget()
{
    if (m_creature == nullptr) {
        return;
    }

    BlockPos targetPos = getTargetPosition();
    m_creature->tryMoveTo(static_cast<f64>(targetPos.x) + 0.5,
        static_cast<f64>(targetPos.y),
        static_cast<f64>(targetPos.z) + 0.5,
        m_movementSpeed);
}

bool MoveToBlockGoal::isWithinDistance(const BlockPos& pos, f64 distSq) const
{
    if (m_creature == nullptr) {
        return false;
    }

    f64 dx = m_creature->x() - (static_cast<f64>(pos.x) + 0.5);
    f64 dy = m_creature->y() - static_cast<f64>(pos.y);
    f64 dz = m_creature->z() - (static_cast<f64>(pos.z) + 0.5);
    return (dx * dx + dy * dy + dz * dz) < distSq;
}

bool MoveToBlockGoal::searchForDestination()
{
    if (m_creature == nullptr || m_creature->world() == nullptr) {
        return false;
    }

    IWorld* world = m_creature->world();
    i32 range = m_searchLength;
    i32 yRange = m_verticalSearchRange;

    // 使用实体位置作为搜索中心
    BlockPos centerPos(static_cast<i32>(std::floor(m_creature->x())),
        static_cast<i32>(std::floor(m_creature->y())),
        static_cast<i32>(std::floor(m_creature->z())));

    // 获取 MobEntity 以检查家范围（CreatureEntity 继承自 MobEntity）
    MobEntity* mob = dynamic_cast<MobEntity*>(m_creature);

    // Y轴交替搜索：0, 1, -1, 2, -2, ...
    for (i32 y = m_verticalSearchStart; y <= yRange;) {
        // 水平螺旋搜索
        for (i32 layer = 0; layer < range; ++layer) {
            for (i32 dx = 0; dx <= layer; dx = (dx > 0 ? -dx : 1 - dx)) {
                for (i32 dz = (dx < layer && dx > -layer ? layer : 0); dz <= layer; dz = (dz > 0 ? -dz : 1 - dz)) {

                    BlockPos checkPos(centerPos.x + dx, centerPos.y + y - 1, centerPos.z + dz);

                    // 检查是否在家的范围内（如果有家限制）
                    if (mob != nullptr && !mob->isWithinHomeDistanceFromPosition(checkPos)) {
                        continue;
                    }

                    if (shouldMoveTo(world, checkPos)) {
                        m_destinationBlock = checkPos;
                        return true;
                    }
                }
            }
        }

        // Y轴交替递增
        if (y == 0) {
            y = 1;
        } else if (y > 0) {
            y = -y;
        } else {
            y = -y + 1;
        }
    }

    return false;
}

// ============================================================================
// MoveToLavaGoal 实现
// ============================================================================

MoveToLavaGoal::MoveToLavaGoal(CreatureEntity* creature, f64 speed)
    : MoveToBlockGoal(creature, speed, LAVA_SEARCH_LENGTH, LAVA_VERTICAL_SEARCH_RANGE)
{}

BlockPos MoveToLavaGoal::getTargetPosition() const
{
    // 直接返回熔岩位置，不是上方
    return m_destinationBlock;
}

bool MoveToLavaGoal::shouldExecute()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 如果已经在熔岩中，不需要寻找
    // 使用 Entity::isInLava() 检查
    if (m_creature->isInLava()) {
        return false;
    }

    // 调用父类方法搜索目标
    return MoveToBlockGoal::shouldExecute();
}

bool MoveToLavaGoal::shouldContinueExecuting()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 如果已经到达熔岩，停止
    if (m_creature->isInLava()) {
        return false;
    }

    // 检查目标熔岩是否仍然有效
    return shouldMoveTo(m_creature->world(), m_destinationBlock);
}

bool MoveToLavaGoal::shouldMove() const
{
    // 每 20 tick 检查一次（父类默认是 40 tick），使炽足兽更频繁地更新导航路径
    return m_timeoutCounter % LAVA_MOVE_INTERVAL == 0;
}

bool MoveToLavaGoal::shouldMoveTo(IWorld* world, const BlockPos& pos)
{
    if (world == nullptr) {
        return false;
    }

    // 检查位置是否在世界边界内
    if (!world->isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        return false;
    }

    // 检查目标方块是否是熔岩
    const fluid::FluidState* fluidState = world->getFluidState(pos);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 使用 FluidTags::LAVA 检查
    if (!fluidState->getFluid().isIn(fluid::FluidTags::LAVA())) {
        return false;
    }

    // 检查上方方块是否可以通过
    BlockPos abovePos = pos.up();
    const BlockState* aboveState = world->getBlockState(abovePos);
    if (aboveState == nullptr) {
        return false;
    }

    // 检查上方是否是空气或可通过的方块
    // canBeReplacedByFluid() = canBeReplaced() || !isSolid()
    // 对应 MC Java 的 BlockState.canBeReplaced(Fluid)
    return aboveState->canBeReplacedByFluid();
}

} // namespace mc::entity::ai::goal
