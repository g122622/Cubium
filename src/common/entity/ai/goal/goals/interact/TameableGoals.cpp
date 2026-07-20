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

#include "TameableGoals.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/tamable/TameableEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

using namespace mc::entity::ai;

namespace mc::entity::ai::goal {

// ============================================================================
// FollowOwnerGoal
// ============================================================================

FollowOwnerGoal::FollowOwnerGoal(
    TameableEntity* entity, f64 speed, f32 minDistance, f32 maxDistance, f32 teleportDistance)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_entity(entity)
    , m_speed(speed)
    , m_minDistance(minDistance)
    , m_maxDistance(maxDistance)
    , m_teleportDistance(teleportDistance)
{
    MC_ASSERT(entity != nullptr);
}

bool FollowOwnerGoal::shouldExecute()
{
    // 只有驯服且未坐下时才跟随
    if (!m_entity->isTamed() || m_entity->isSitting()) {
        return false;
    }

    // 检查是否有主人
    auto ownerId = m_entity->getOwnerId();
    if (!ownerId.has_value()) {
        return false;
    }

    // 从世界获取主人玩家
    m_owner = m_entity->getOwner();
    if (m_owner == nullptr) {
        return false;
    }

    // 距离检查：超过最小距离才执行
    f64 distance = m_entity->distanceTo(*m_owner);
    return distance > m_minDistance;
}

bool FollowOwnerGoal::shouldContinueExecuting()
{
    // 如果不再驯服或坐下，停止跟随
    if (!m_entity->isTamed() || m_entity->isSitting()) {
        return false;
    }

    // 如果主人不存在，停止跟随
    if (!m_owner) {
        return false;
    }

    // 如果距离太近，暂停跟随
    if (m_entity->distanceTo(*m_owner) < m_minDistance) {
        return false;
    }

    return true;
}

void FollowOwnerGoal::startExecuting()
{
    m_timeToRecalcPath = 0;
}

void FollowOwnerGoal::resetTask()
{
    m_owner = nullptr;
    m_entity->clearNavigation();
}

void FollowOwnerGoal::tick()
{
    if (!m_owner) {
        return;
    }

    // 看向主人
    m_entity->lookAt(*m_owner);

    // 定期重新计算路径
    if (--m_timeToRecalcPath <= 0) {
        m_timeToRecalcPath = PATH_RECALC_INTERVAL;

        f64 distance = m_entity->distanceTo(*m_owner);

        // 如果距离过远，尝试传送
        if (distance > m_teleportDistance) {
            _teleportToOwner();
            return;
        }

        // 否则尝试移动到主人身边
        m_entity->tryMoveTo(m_owner->x(), m_owner->y(), m_owner->z(), m_speed);
    }
}

bool FollowOwnerGoal::_canFollowOwner() const
{
    if (!m_entity->isTamed() || m_entity->isSitting()) {
        return false;
    }

    auto ownerId = m_entity->getOwnerId();
    if (!ownerId.has_value()) {
        return false;
    }

    return true;
}

bool FollowOwnerGoal::_teleportToOwner()
{
    if (!m_owner) {
        return false;
    }

    // 在主人附近找一个安全的传送位置
    IWorld* worldPtr = m_entity->world();
    if (!worldPtr) {
        return false;
    }

    math::Random& random = worldPtr->getRandom();

    // 在主人周围尝试传送位置
    for (i32 attempt = 0; attempt < 10; ++attempt) {
        // 随机选择主人周围的位置（距离1-3格）
        f32 angle = random.nextFloat() * math::TWO_PI;
        f32 distance = 1.0f + random.nextFloat() * 2.0f;

        f32 targetX = m_owner->x() + std::cos(angle) * distance;
        f32 targetZ = m_owner->z() + std::sin(angle) * distance;
        f32 targetY = m_owner->y();

        // 尝试找到安全的Y位置
        for (i32 yOffset = -1; yOffset <= 1; ++yOffset) {
            f32 testY = targetY + static_cast<f32>(yOffset);

            // 检查脚部位置是否有碰撞
            AxisAlignedBB entityBox = m_entity->boundingBox();
            AxisAlignedBB testBox(targetX - entityBox.width() / 2.0f,
                testY,
                targetZ - entityBox.width() / 2.0f,
                targetX + entityBox.width() / 2.0f,
                testY + entityBox.height(),
                targetZ + entityBox.width() / 2.0f);

            // 检查是否有碰撞
            if (worldPtr->hasNoCollisions(testBox)) {
                // 传送实体
                m_entity->setPosition(targetX, testY, targetZ);
                m_entity->clearNavigation();

                // 播放传送音效（如果有的话）
                // auto soundEvent = m_entity->getTeleportSound();
                // if (soundEvent) {
                //     m_entity->playSound(*soundEvent, 1.0f, 1.0f);
                // }

                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// SitGoal
// ============================================================================

SitGoal::SitGoal(TameableEntity* entity)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    , m_entity(entity)
{
    MC_ASSERT(entity != nullptr);
}

bool SitGoal::shouldExecute()
{
    // 驯服状态下执行坐下
    return m_entity->isTamed() && m_entity->isSitting();
}

bool SitGoal::shouldContinueExecuting()
{
    return m_entity->isTamed() && m_entity->isSitting();
}

void SitGoal::startExecuting()
{
    // 坐下时停止移动
    m_entity->clearNavigation();
}

void SitGoal::resetTask()
{
    // 站起时无需特殊处理
}

// ============================================================================
// BegGoal
// ============================================================================

BegGoal::BegGoal(TameableEntity* entity, f32 maxDistance)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_entity(entity)
    , m_maxDistance(maxDistance)
{
    MC_ASSERT(entity != nullptr);
}

bool BegGoal::shouldExecute()
{
    // 查找最近的手持食物的玩家
    IWorld* worldPtr = m_entity->world();
    if (!worldPtr) {
        return false;
    }

    // 获取范围内的所有实体
    std::vector<Entity*> entities = worldPtr->getEntitiesInRange(m_entity->position(), m_maxDistance, m_entity);

    m_targetPlayer = nullptr;
    f32 closestDistance = m_maxDistance * m_maxDistance; // 使用平方距离比较

    for (Entity* entity : entities) {
        if (entity == nullptr || entity->entityType() != entity::VanillaEntityTypeKeys::PLAYER) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 检查玩家是否手持食物
        if (!_isPlayerHoldingFood(player)) {
            continue;
        }

        // 检查距离
        f32 distanceSq = m_entity->distanceSqTo(*player);
        if (distanceSq < closestDistance) {
            closestDistance = distanceSq;
            m_targetPlayer = player;
        }
    }

    return m_targetPlayer != nullptr;
}

bool BegGoal::shouldContinueExecuting()
{
    if (!m_targetPlayer) {
        return false;
    }

    // 检查玩家是否仍然在范围内且手持食物
    if (m_entity->distanceTo(*m_targetPlayer) > m_maxDistance) {
        return false;
    }

    return _isPlayerHoldingFood(m_targetPlayer);
}

void BegGoal::startExecuting()
{
    m_begAngle = 0.0f;

    // 狼特有：标记为感兴趣状态，触发乞求头部倾斜动画
    // 对应 MC 1.21.11 BegGoal.start(): this.wolf.setIsInterested(true)
    // setInterested 通过 DataParameter 同步到客户端，客户端 ClientEntity::tick
    // 推进 wolfInterestedAngle 插值，WolfModel 读取渲染头部 Z 轴旋转。
    auto* wolf = dynamic_cast<WolfEntity*>(m_entity);
    if (wolf != nullptr) {
        wolf->setInterested(true);
    }
}

void BegGoal::resetTask()
{
    m_targetPlayer = nullptr;

    // 狼特有：取消感兴趣状态
    // 对应 MC 1.21.11 BegGoal.stop(): this.wolf.setIsInterested(false)
    auto* wolf = dynamic_cast<WolfEntity*>(m_entity);
    if (wolf != nullptr) {
        wolf->setInterested(false);
    }
}

void BegGoal::tick()
{
    if (!m_targetPlayer) {
        return;
    }

    // 看向玩家并乞求（摆头）
    m_entity->lookAt(*m_targetPlayer);

    // 乞求角度动画
    m_begAngle += BEG_ANGLE_SPEED;
    if (m_begAngle > 1.0f) {
        m_begAngle = -1.0f;
    }
}

bool BegGoal::_isPlayerHoldingFood(const Player* player) const
{
    if (!player) {
        return false;
    }

    // 1. 已驯服的狼对骨头乞求（isTamed() && item == BONE）
    // 2. 所有狼对繁殖物品（肉类）乞求（isBreedingItem()）

    // 获取主手物品
    ItemStack mainHandItem = player->getHeldItem(Hand::MainHand);
    if (!mainHandItem.isEmpty()) {
        // 已驯服的动物对驯服物品乞求
        if (m_entity->isTamed() && m_entity->isTameItem(mainHandItem)) {
            return true;
        }
        // 所有动物对繁殖物品乞求
        if (m_entity->isBreedingItem(mainHandItem)) {
            return true;
        }
    }

    // 获取副手物品
    ItemStack offHandItem = player->getHeldItem(Hand::OffHand);
    if (!offHandItem.isEmpty()) {
        // 已驯服的动物对驯服物品乞求
        if (m_entity->isTamed() && m_entity->isTameItem(offHandItem)) {
            return true;
        }
        // 所有动物对繁殖物品乞求
        if (m_entity->isBreedingItem(offHandItem)) {
            return true;
        }
    }

    return false;
}

} // namespace mc::entity::ai::goal
