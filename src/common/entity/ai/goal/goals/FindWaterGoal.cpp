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

#include "FindWaterGoal.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/BlockState.hpp"
#include "../../../../world/block/Material.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <cmath>
#include <limits>

namespace mc::entity::ai::goal {

FindWaterGoal::FindWaterGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
{
    MC_ASSERT(creature != nullptr);
}

bool FindWaterGoal::shouldExecute()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 只在不在水中时执行
    if (m_creature->isInWater()) {
        return false;
    }

    // 寻找水源
    return _findWater();
}

bool FindWaterGoal::shouldContinueExecuting()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 如果已经在水中，停止
    if (m_creature->isInWater()) {
        return false;
    }

    // 如果没有找到水源，停止
    if (!m_foundWater) {
        return false;
    }

    // 检查是否仍有路径
    auto* mob = dynamic_cast<MobEntity*>(m_creature);
    if (mob == nullptr) {
        return false;
    }
    return mob->navigator() != nullptr && mob->navigator()->hasPath();
}

void FindWaterGoal::startExecuting()
{
    if (m_creature == nullptr || !m_foundWater) {
        return;
    }

    // 移动到水源
    m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, 1.0);
}

void FindWaterGoal::resetTask()
{
    m_foundWater = false;
}

void FindWaterGoal::tick()
{
    // 每tick检查是否已经到达水中
    if (m_creature != nullptr && m_creature->isInWater()) {
        // 已到达水中，停止导航
        auto* mob = dynamic_cast<MobEntity*>(m_creature);
        if (mob != nullptr && mob->navigator() != nullptr) {
            mob->navigator()->clearPath();
        }
        m_foundWater = false;
    }
}

bool FindWaterGoal::_findWater()
{
    if (m_creature == nullptr || m_creature->world() == nullptr) {
        return false;
    }

    IWorld* world = m_creature->world();
    BlockPos entityPos(
        static_cast<i32>(m_creature->x()), static_cast<i32>(m_creature->y()), static_cast<i32>(m_creature->z()));

    // 在实体周围搜索水源
    // 搜索范围：水平方向 16 格，垂直方向 5 格
    constexpr i32 HORIZONTAL_RANGE = 16;
    constexpr i32 VERTICAL_RANGE = 5;

    f64 bestDistance = std::numeric_limits<f64>::max();
    bool found = false;

    for (i32 dx = -HORIZONTAL_RANGE; dx <= HORIZONTAL_RANGE; ++dx) {
        for (i32 dy = -VERTICAL_RANGE; dy <= VERTICAL_RANGE; ++dy) {
            for (i32 dz = -HORIZONTAL_RANGE; dz <= HORIZONTAL_RANGE; ++dz) {
                BlockPos checkPos(entityPos.x + dx, entityPos.y + dy, entityPos.z + dz);

                // 检查是否是水源方块
                const BlockState* state = world->getBlockState(checkPos);
                if (state == nullptr || !state->getMaterial().isLiquid()) {
                    continue;
                }

                // 计算距离
                f64 distance =
                    std::sqrt(static_cast<f64>(dx * dx) + static_cast<f64>(dy * dy) + static_cast<f64>(dz * dz));

                if (distance < bestDistance) {
                    bestDistance = distance;
                    m_targetX = static_cast<f64>(checkPos.x) + 0.5;
                    m_targetY = static_cast<f64>(checkPos.y);
                    m_targetZ = static_cast<f64>(checkPos.z) + 0.5;
                    found = true;
                }
            }
        }
    }

    m_foundWater = found;
    return found;
}

} // namespace mc::entity::ai::goal
