/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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

#include "FleeSunGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::entity::ai::goal {

FleeSunGoal::FleeSunGoal(CreatureEntity* creature, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_speed(speed)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

bool FleeSunGoal::shouldExecute()
{
    if (m_creature->isBeingRidden()) {
        return false;
    }

    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return false;
    }

    // MC原版使用 isBrightOutside()，考虑天气（雷暴时白天也会变暗）
    if (!world->isBrightOutside()) {
        return false;
    }

    // 检查生物当前位置是否暴露在阳光下
    const BlockPos entityPos(
        math::floorTo<i32>(m_creature->x()), math::floorTo<i32>(m_creature->y()), math::floorTo<i32>(m_creature->z()));

    if (!world->canSeeSky(entityPos)) {
        return false;
    }

    // 寻找阴影位置
    return _findShadedPosition();
}

bool FleeSunGoal::shouldContinueExecuting()
{
    // 如果导航还在进行中，继续执行
    auto* nav = m_creature->navigator();
    if (nav && nav->hasPath() && !nav->isDone()) {
        return true;
    }

    return false;
}

void FleeSunGoal::startExecuting()
{
    static_cast<void>(m_creature->navigator()->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
}

void FleeSunGoal::resetTask()
{
    m_creature->clearNavigation();
}

void FleeSunGoal::tick()
{
    // 持续导航到阴影位置
    // PathNavigator的tick由MobEntity::tick调用，无需手动调用
}

bool FleeSunGoal::_findShadedPosition()
{
    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return false;
    }

    // 使用RandomPositionGenerator找到目标位置，然后验证该位置不在阳光下
    Vector3 targetPos;
    for (i32 attempt = 0; attempt < 10; ++attempt) {
        if (!util::RandomPositionGenerator::findRandomTarget(
                m_creature, SHELTER_XZ_RANGE, SHELTER_Y_RANGE, targetPos)) {
            continue;
        }

        const i32 blockX = math::floorTo<i32>(targetPos.x);
        const i32 blockY = math::floorTo<i32>(targetPos.y);
        const i32 blockZ = math::floorTo<i32>(targetPos.z);

        // 检查目标位置是否不在阳光下
        const BlockPos pos(blockX, blockY, blockZ);
        if (!world->canSeeSky(pos)) {
            m_targetX = static_cast<f64>(blockX) + 0.5;
            m_targetY = static_cast<f64>(blockY);
            m_targetZ = static_cast<f64>(blockZ) + 0.5;
            return true;
        }
    }

    return false;
}

} // namespace mc::entity::ai::goal
