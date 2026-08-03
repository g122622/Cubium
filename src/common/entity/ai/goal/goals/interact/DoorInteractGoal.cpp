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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "DoorInteractGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/DoorBlock.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace mc::entity::ai::goal {

DoorInteractGoal::DoorInteractGoal(MobEntity* mob)
    : m_mob(mob)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool DoorInteractGoal::shouldExecute()
{
    if (!m_mob || !m_mob->world()) {
        return false;
    }

    // 检查导航器是否支持地面路径
    auto* navigator = m_mob->navigator();
    if (!navigator || !navigator->canOpenDoors()) {
        return false;
    }

    // 检查生物是否水平碰撞
    if (!m_mob->collidedHorizontally()) {
        return false;
    }

    m_world = m_mob->world();

    // 沿路径节点查找门
    const pathfinding::Path* path = navigator->getPath();
    if (path && !path->isFinished()) {
        i32 currentIndex = path->getCurrentIndex();
        i32 endIndex = std::min(currentIndex + 2, static_cast<i32>(path->length()) - 1);

        for (i32 i = currentIndex; i <= endIndex; ++i) {
            const pathfinding::PathPoint* point = path->getPoint(static_cast<size_t>(i));
            if (!point) {
                continue;
            }

            // 检查路径节点上方是否有木门（门的下半部分在节点Y+1）
            BlockPos doorCandidate(point->x(), point->y() + 1, point->z());

            // 计算生物到门位置的水平距离（1.5方块以内）
            f32 distSq = math::distanceHorizontalSq(m_mob->position().x,
                m_mob->position().z,
                static_cast<f32>(doorCandidate.x) + 0.5f,
                static_cast<f32>(doorCandidate.z) + 0.5f);

            if (distSq > 2.25f) { // 1.5^2 = 2.25
                continue;
            }

            const BlockState* state = m_world->getBlockState(doorCandidate);
            if (state && blocks::DoorBlock::isWooden(*state)) {
                m_doorPos = doorCandidate;
                m_hasDoor = true;
                return true;
            }
        }
    }

    // 路径上未找到门，检查生物正上方方块
    BlockPos entityPos(static_cast<i32>(std::floor(m_mob->position().x)),
        static_cast<i32>(std::floor(m_mob->position().y)),
        static_cast<i32>(std::floor(m_mob->position().z)));

    // 检查生物位置上方（Y+1）是否有门
    BlockPos abovePos(entityPos.x, entityPos.y + 1, entityPos.z);
    const BlockState* aboveState = m_world->getBlockState(abovePos);
    if (aboveState && blocks::DoorBlock::isWooden(*aboveState)) {
        m_doorPos = abovePos;
        m_hasDoor = true;
        return true;
    }

    return false;
}

bool DoorInteractGoal::shouldContinueExecuting()
{
    return !m_hasPassedDoor;
}

void DoorInteractGoal::startExecuting()
{
    m_hasPassedDoor = false;

    // 记录生物相对于门中心的方向
    if (m_mob && m_hasDoor) {
        f32 dx = static_cast<f32>(m_doorPos.x) + 0.5f - static_cast<f32>(m_mob->position().x);
        f32 dz = static_cast<f32>(m_doorPos.z) + 0.5f - static_cast<f32>(m_mob->position().z);
        f32 len = std::sqrt(dx * dx + dz * dz);
        if (len > 0.001f) {
            m_doorOpenDirX = dx / len;
            m_doorOpenDirZ = dz / len;
        } else {
            m_doorOpenDirX = 0.0f;
            m_doorOpenDirZ = 0.0f;
        }
    }
}

void DoorInteractGoal::tick()
{
    if (!m_mob || !m_hasDoor) {
        return;
    }

    // 计算当前方向向量（从生物到门中心）
    f32 curDx = static_cast<f32>(m_doorPos.x) + 0.5f - static_cast<f32>(m_mob->position().x);
    f32 curDz = static_cast<f32>(m_doorPos.z) + 0.5f - static_cast<f32>(m_mob->position().z);

    // 计算点积：如果方向发生了反转，说明生物已穿过门
    f32 dot = curDx * m_doorOpenDirX + curDz * m_doorOpenDirZ;
    if (dot < 0.0f) {
        m_hasPassedDoor = true;
    }
}

bool DoorInteractGoal::_isDoorOpen()
{
    if (!m_hasDoor || !m_world) {
        return false;
    }

    const BlockState* state = m_world->getBlockState(m_doorPos);
    if (!state) {
        m_hasDoor = false;
        return false;
    }

    // 检查是否仍然是门
    const blocks::DoorBlock* doorBlock = dynamic_cast<const blocks::DoorBlock*>(&state->getBlock());
    if (!doorBlock) {
        m_hasDoor = false;
        return false;
    }

    return blocks::DoorBlock::isOpen(*state);
}

void DoorInteractGoal::_setDoorOpen(bool open)
{
    if (!m_hasDoor || !m_world) {
        return;
    }

    const BlockState* state = m_world->getBlockState(m_doorPos);
    if (!state) {
        return;
    }

    // 获取 DoorBlock 实例并调用其 toggleDoor 方法
    blocks::DoorBlock* doorBlock =
        const_cast<blocks::DoorBlock*>(dynamic_cast<const blocks::DoorBlock*>(&state->getBlock()));
    if (doorBlock) {
        doorBlock->toggleDoor(*m_world, m_doorPos, open);
    }
}

} // namespace mc::entity::ai::goal
