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
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../entities/passive/tamable/ShoulderRidingEntity.hpp"
#include "../../../../entities/player/Player.hpp"

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
    if (m_entity == nullptr) {
        return false;
    }

    // MC 1.16.5: 检查是否可以坐到肩膀上
    // 条件1：未坐下（原版使用 isOrderedToSit，这里用 isSitting）
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

    // MC 1.16.5: 检查主人状态
    // - 不是旁观者模式
    // - 不在飞行（创造模式飞行）
    // - 不在水中
    if (m_owner->isSpectator()) {
        return false;
    }

    // 检查主人是否在飞行（创造模式/旁观者模式的飞行能力）
    if (m_owner->abilities().flying) {
        return false;
    }

    // 检查主人是否在水中
    if (m_owner->isInWater()) {
        return false;
    }

    return true;
}

bool LandOnOwnersShoulderGoal::shouldContinueExecuting()
{
    if (m_entity == nullptr || m_owner == nullptr) {
        return false;
    }

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

    if (m_owner->isSpectator() || m_owner->abilities().flying || m_owner->isInWater()) {
        return false;
    }

    return true;
}

bool LandOnOwnersShoulderGoal::isPreemptible() const
{
    // MC 1.16.5: 如果已经在肩膀上，不可被抢占
    return !m_isSittingOnShoulder;
}

void LandOnOwnersShoulderGoal::startExecuting()
{
    m_isSittingOnShoulder = false;
    // owner 已在 shouldExecute 中设置
}

void LandOnOwnersShoulderGoal::tick()
{
    if (m_entity == nullptr || m_owner == nullptr) {
        return;
    }

    // 如果已经坐到肩膀上，不需要再做任何事
    if (m_isSittingOnShoulder) {
        return;
    }

    // MC 1.16.5: 检查是否被命令坐下
    // 原版使用 func_233684_eK_() 即 isOrderedToSit()
    // 这里用 isSitting() 代替
    if (m_entity->isSitting()) {
        return;
    }

    // MC 1.16.5: 检查是否被拴住
    // 如果实体被拴绳拴住，不能坐到肩膀上
    // 注意：拴绳系统尚未完全实现，暂时跳过此检查
    // if (m_entity->isLeashed()) {
    //     return;
    // }

    // MC 1.16.5: 检查碰撞箱是否与主人相交
    // if (this.entity.getBoundingBox().intersects(this.owner.getBoundingBox()))
    if (m_entity->boundingBox().intersects(m_owner->boundingBox())) {
        // 尝试坐到主人肩膀上
        // MC 1.16.5: func_213439_d() 即 setEntityOnShoulder()
        if (m_entity->mountShoulder(m_owner->playerId())) {
            m_isSittingOnShoulder = true;
            // 成功坐到肩膀上后，实体会被从世界中移除（存储在玩家NBT中）
            // 但在我们的实现中，只是设置状态
        }
    }
}

} // namespace mc::entity::ai::goal
