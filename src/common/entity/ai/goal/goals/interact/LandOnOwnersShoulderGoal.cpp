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

#include "LandOnOwnersShoulderGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/entities/passive/tamable/ShoulderRidingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"

namespace mc::entity::ai::goal {

LandOnOwnersShoulderGoal::LandOnOwnersShoulderGoal(ShoulderRidingEntity* entity)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_entity(entity)
    , m_owner(nullptr)
    , m_isSittingOnShoulder(false)
{
    MC_ASSERT_RELEASE(entity != nullptr);
}

bool LandOnOwnersShoulderGoal::shouldExecute()
{
    // 条件1：未坐下
    if (m_entity->isSitting()) {
        return false;
    }

    // 条件2：已驯服
    if (!m_entity->isTamed()) {
        return false;
    }

    // 条件3：肩膀乘坐冷却已过
    if (!m_entity->canSitOnShoulder()) {
        return false;
    }

    // 获取主人
    m_owner = m_entity->getOwner();
    if (m_owner == nullptr) {
        return false;
    }

    // 检查主人状态：不是旁观者模式、不在飞行、不在水中
    if (m_owner->isSpectator()) {
        return false;
    }

    if (m_owner->abilities().flying) {
        return false;
    }

    if (m_owner->isInWater()) {
        return false;
    }

    return true;
}

bool LandOnOwnersShoulderGoal::shouldContinueExecuting()
{
    // 如果已经坐到肩膀上，继续执行以维持状态
    if (m_isSittingOnShoulder) {
        return m_entity->isOnShoulder();
    }

    // 检查条件是否仍然满足
    if (m_entity->isSitting()) {
        return false;
    }

    if (!m_entity->isTamed()) {
        return false;
    }

    if (m_owner == nullptr || m_owner->isSpectator() || m_owner->abilities().flying || m_owner->isInWater()) {
        return false;
    }

    return true;
}

bool LandOnOwnersShoulderGoal::isPreemptible() const noexcept
{
    // 如果已经在肩膀上，不可被抢占
    return !m_isSittingOnShoulder;
}

void LandOnOwnersShoulderGoal::startExecuting()
{
    m_isSittingOnShoulder = false;
    // owner 已在 shouldExecute 中设置
}

void LandOnOwnersShoulderGoal::tick()
{
    // 如果已经坐到肩膀上，不需要再做任何事
    if (m_isSittingOnShoulder) {
        return;
    }

    // 没有主人时无法落肩。正常情况下 shouldExecute() 会先填充 m_owner，
    // 但 Goal 系统或测试可能在异常状态下直接调用 tick()，此处需判空避免空指针解引用。
    if (m_owner == nullptr) {
        return;
    }

    // 检查是否被命令坐下
    if (m_entity->isSitting()) {
        return;
    }

    // 检查是否被拴住，被拴住的实体不能坐到主人肩膀上
    if (m_entity->isLeashed()) {
        return;
    }

    // 检查碰撞箱是否与主人相交
    if (m_entity->boundingBox().intersects(m_owner->boundingBox())) {
        // 尝试坐到主人肩膀上
        if (m_entity->mountShoulder(m_owner->playerId())) {
            m_isSittingOnShoulder = true;
        }
    }
}

} // namespace mc::entity::ai::goal
