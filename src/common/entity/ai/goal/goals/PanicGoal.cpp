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

#include "PanicGoal.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../util/RandomPositionGenerator.hpp"
#include "../GoalConstants.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

PanicGoal::PanicGoal(CreatureEntity* creature, f64 speed)
    : m_creature(creature)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool PanicGoal::shouldExecute()
{
    if (!m_creature) return false;

    // MC 1.16.5: 检查是否有复仇目标(getRevengeTarget)或着火(isBurning)
    // getRevengeTarget() 对应 getLastHurtBy()
    LivingEntity* revengeTarget = m_creature->getLastHurtBy();
    bool isBurning = m_creature->isOnFire();

    if (revengeTarget == nullptr && !isBurning) {
        return false;
    }

    // MC 1.16.5: 如果着火，尝试找水
    if (isBurning) {
        BlockPos waterPos = getRandomWaterPosition(
            static_cast<i32>(PANIC_WATER_SEARCH_RANGE), static_cast<i32>(PANIC_WATER_SEARCH_VERTICAL));
        if (waterPos.x != 0 || waterPos.y != 0 || waterPos.z != 0) {
            m_targetX = static_cast<f64>(waterPos.x) + 0.5;
            m_targetY = static_cast<f64>(waterPos.y) + 0.5;
            m_targetZ = static_cast<f64>(waterPos.z) + 0.5;
            return true;
        }
    }

    // MC 1.16.5: 否则使用 RandomPositionGenerator.findRandomTarget(creature, 5, 4)
    return findRandomPosition();
}

bool PanicGoal::shouldContinueExecuting()
{
    if (!m_creature) return false;

    // MC 1.16.5: 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return true;
}

void PanicGoal::startExecuting()
{
    if (m_creature) {
        // MC 1.16.5: 使用 navigator.tryMoveToXYZ
        if (auto* nav = m_creature->navigator()) {
            static_cast<void>(nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
        }
        m_running = true;
    }
}

void PanicGoal::resetTask()
{
    m_running = false;
    // MC 1.16.5: 不清除导航路径，让其他目标接管
}

void PanicGoal::tick()
{
    // MC 1.16.5: PanicGoal 的 tick 是空的
    // 路径在 startExecuting 中设置，不需要每 tick 更新
}

bool PanicGoal::findRandomPosition()
{
    if (!m_creature) return false;

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTarget(creature, 5, 4)
    Vector3 targetPos;
    if (util::RandomPositionGenerator::findRandomTarget(m_creature,
            PANIC_ESCAPE_MIN_DISTANCE,   // 5 水平范围
            PANIC_WATER_SEARCH_VERTICAL, // 4 垂直范围
            targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        return true;
    }

    return false;
}

BlockPos PanicGoal::getRandomWaterPosition(i32 horizontalRange, i32 verticalRange)
{
    if (!m_creature || !m_creature->world()) {
        return BlockPos(0, 0, 0);
    }

    IWorld* world = m_creature->world();

    // MC 1.16.5: 在立方体区域内搜索水源，找最近的水方块
    i32 cx = static_cast<i32>(m_creature->x());
    i32 cy = static_cast<i32>(m_creature->y());
    i32 cz = static_cast<i32>(m_creature->z());

    // MC 1.16.5: 初始最远距离为 horizontalRange^2 * verticalRange * 2
    f32 closestDistSq = static_cast<f32>(horizontalRange * horizontalRange * verticalRange * 2);
    BlockPos closestWater(0, 0, 0);

    // MC 1.16.5: 遍历立方体区域
    for (i32 x = cx - horizontalRange; x <= cx + horizontalRange; ++x) {
        for (i32 y = cy - verticalRange; y <= cy + verticalRange; ++y) {
            for (i32 z = cz - horizontalRange; z <= cz + horizontalRange; ++z) {
                if (!world->isWithinWorldBounds(x, y, z)) {
                    continue;
                }

                BlockPos pos(x, y, z);
                // MC 1.16.5: 使用 FluidTags.WATER 检查水
                if (world->isWaterAt(pos)) {
                    f32 distSq = static_cast<f32>((x - cx) * (x - cx) + (y - cy) * (y - cy) + (z - cz) * (z - cz));
                    if (distSq < closestDistSq) {
                        closestDistSq = distSq;
                        closestWater = pos;
                    }
                }
            }
        }
    }

    return closestWater;
}

} // namespace mc::entity::ai::goal
