#include "TameableGoals.hpp"
#include "../../core/MobEntity.hpp"
#include "../passive/tamable/TameableEntity.hpp"
#include "../player/Player.hpp"
#include "../../item/ItemStack.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// FollowOwnerGoal
// ============================================================================

FollowOwnerGoal::FollowOwnerGoal(TameableEntity* entity, f64 speed, f32 minDistance, f32 maxDistance, f32 teleportDistance)
    : Goal(GoalFlags::MOVE)
    , m_entity(entity)
    , m_speed(speed)
    , m_minDistance(minDistance)
    , m_maxDistance(maxDistance)
    , m_teleportDistance(teleportDistance)
{
    MC_ASSERT(entity != nullptr);
}

bool FollowOwnerGoal::shouldExecute() {
    // 只有驯服且未坐下时才跟随
    if (!m_entity->isTamed() || m_entity->isSitting()) {
        return false;
    }

    // 检查是否有主人
    auto ownerId = m_entity->getOwnerId();
    if (!ownerId.has_value()) {
        return false;
    }

    // TODO: 从世界获取主人玩家
    // 暂时返回false，等待PlayerManager实现
    return false;
}

bool FollowOwnerGoal::shouldContinueExecuting() {
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

void FollowOwnerGoal::startExecuting() {
    m_timeToRecalcPath = 0;
}

void FollowOwnerGoal::resetTask() {
    m_owner = nullptr;
    m_entity->clearNavigation();
}

void FollowOwnerGoal::tick() {
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
            teleportToOwner();
            return;
        }

        // 否则尝试移动到主人身边
        m_entity->tryMoveTo(m_owner->x(), m_owner->y(), m_owner->z(), m_speed);
    }
}

bool FollowOwnerGoal::canFollowOwner() const {
    if (!m_entity->isTamed() || m_entity->isSitting()) {
        return false;
    }

    auto ownerId = m_entity->getOwnerId();
    if (!ownerId.has_value()) {
        return false;
    }

    return true;
}

bool FollowOwnerGoal::teleportToOwner() {
    if (!m_owner) {
        return false;
    }

    // 在主人附近找一个安全的传送位置
    // TODO: 实现传送逻辑
    // 需要找到安全的传送位置并检查方块碰撞

    return false;
}

// ============================================================================
// SitGoal
// ============================================================================

SitGoal::SitGoal(TameableEntity* entity)
    : Goal(GoalFlags::STAY)
    , m_entity(entity)
{
    MC_ASSERT(entity != nullptr);
}

bool SitGoal::shouldExecute() {
    // 驯服状态下执行坐下
    return m_entity->isTamed() && m_entity->isSitting();
}

bool SitGoal::shouldContinueExecuting() {
    return m_entity->isTamed() && m_entity->isSitting();
}

void SitGoal::startExecuting() {
    // 坐下时停止移动
    m_entity->clearNavigation();
}

void SitGoal::resetTask() {
    // 站起时无需特殊处理
}

// ============================================================================
// BegGoal
// ============================================================================

BegGoal::BegGoal(TameableEntity* entity, f32 maxDistance)
    : Goal(GoalFlags::LOOK)
    , m_entity(entity)
    , m_maxDistance(maxDistance)
{
    MC_ASSERT(entity != nullptr);
}

bool BegGoal::shouldExecute() {
    // TODO: 找到最近的手持食物的玩家
    // 暂时返回false，等待世界查询实现
    return false;
}

bool BegGoal::shouldContinueExecuting() {
    if (!m_targetPlayer) {
        return false;
    }

    // 检查玩家是否仍然在范围内且手持食物
    if (m_entity->distanceTo(*m_targetPlayer) > m_maxDistance) {
        return false;
    }

    return isPlayerHoldingFood(m_targetPlayer);
}

void BegGoal::startExecuting() {
    m_begAngle = 0.0f;
}

void BegGoal::resetTask() {
    m_targetPlayer = nullptr;
}

void BegGoal::tick() {
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

bool BegGoal::isPlayerHoldingFood(const Player* player) const {
    if (!player) {
        return false;
    }

    // TODO: 检查玩家手持物品是否是此动物的食物
    // 需要调用 TameableEntity::isFoodItem()
    return false;
}

} // namespace mc::entity::ai::goal
