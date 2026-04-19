#include "TemptGoal.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../GoalConstants.hpp"
#include "../../controller/LookController.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

TemptGoal::TemptGoal(CreatureEntity* creature, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement)
    : m_creature(creature)
    , m_speed(speed)
    , m_itemPredicate(std::move(itemPredicate))
    , m_scaredByMovement(scaredByMovement)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool TemptGoal::shouldExecute() {
    if (!m_creature) return false;

    // 检查冷却
    if (m_delayTemptCounter > 0) {
        m_delayTemptCounter--;
        return false;
    }

    // 寻找手持诱惑物品的玩家
    m_temptingPlayer = findTemptingPlayer();
    return m_temptingPlayer != nullptr;
}

bool TemptGoal::shouldContinueExecuting() {
    if (!m_creature || !m_temptingPlayer) return false;

    // 检查玩家是否存活
    if (!m_temptingPlayer->isAlive()) return false;

    const ItemStack& mainHand = m_temptingPlayer->getHeldItem(Hand::MainHand);
    const ItemStack& offHand = m_temptingPlayer->getHeldItem(Hand::OffHand);
    if (!isTempting(mainHand) && !isTempting(offHand)) {
        return false;
    }

    if (m_creature->distanceSqTo(*m_temptingPlayer) > TEMPT_RANGE * TEMPT_RANGE) {
        return false;
    }

    // 检查是否被玩家移动吓跑
    if (m_scaredByMovement) {
        f32 distSq = m_creature->distanceSqTo(*m_temptingPlayer);

        if (distSq < TEMPT_SCARE_DISTANCE_SQ) {
            // 检查玩家是否移动
            f32 playerDx = m_temptingPlayer->x() - m_targetX;
            f32 playerDy = m_temptingPlayer->y() - m_targetY;
            f32 playerDz = m_temptingPlayer->z() - m_targetZ;
            f32 playerDistSq = playerDx * playerDx + playerDy * playerDy + playerDz * playerDz;

            if (playerDistSq > 0.01f) {
                return false; // 玩家移动了，停止
            }

            // 检查玩家视角变化
            f32 pitchDiff = std::abs(m_temptingPlayer->pitch() - m_prevPitch);
            f32 yawDiff = std::abs(m_temptingPlayer->yaw() - m_prevYaw);

            if (pitchDiff > VIEW_CHANGE_THRESHOLD || yawDiff > VIEW_CHANGE_THRESHOLD) {
                return false; // 玩家视角变化，停止
            }
        } else {
            // 更新目标位置
            m_targetX = m_temptingPlayer->x();
            m_targetY = m_temptingPlayer->y();
            m_targetZ = m_temptingPlayer->z();
        }

        m_prevPitch = m_temptingPlayer->pitch();
        m_prevYaw = m_temptingPlayer->yaw();
    }

    return shouldExecute();
}

void TemptGoal::startExecuting() {
    if (!m_temptingPlayer) return;

    m_targetX = m_temptingPlayer->x();
    m_targetY = m_temptingPlayer->y();
    m_targetZ = m_temptingPlayer->z();
    m_prevPitch = m_temptingPlayer->pitch();
    m_prevYaw = m_temptingPlayer->yaw();
    m_isRunning = true;
}

void TemptGoal::resetTask() {
    m_temptingPlayer = nullptr;
    m_isRunning = false;

    if (m_creature) {
        m_creature->clearNavigation();
    }

    m_delayTemptCounter = TEMPT_COOLDOWN;
}

void TemptGoal::tick() {
    if (!m_creature || !m_temptingPlayer) return;

    // 看向玩家
    m_creature->lookAt(*m_temptingPlayer);

    // 计算与玩家的距离
    f32 distSq = m_creature->distanceSqTo(*m_temptingPlayer);

    // 如果距离太近，停止移动
    if (distSq < TEMPT_CLOSE_DISTANCE_SQ) {
        m_creature->clearNavigation();
    } else {
        // 跟随玩家
        m_creature->tryMoveTo(m_temptingPlayer->x(), m_temptingPlayer->y(), m_temptingPlayer->z(), m_speed);
    }
}

bool TemptGoal::isTempting(const ItemStack& stack) const {
    return m_itemPredicate(stack);
}

bool TemptGoal::isScaredByPlayerMovement() const {
    return m_scaredByMovement;
}

Player* TemptGoal::findTemptingPlayer() {
    if (!m_creature || !m_creature->world()) return nullptr;

    return EntityUtils::findClosestEntity<Player>(
        m_creature->world(),
        m_creature->position(),
        TEMPT_RANGE,
        m_creature,
        [this](Player* playerEntity) {
            const ItemStack& mainHand = playerEntity->getHeldItem(Hand::MainHand);
            const ItemStack& offHand = playerEntity->getHeldItem(Hand::OffHand);
            return isTempting(mainHand) || isTempting(offHand);
        });
}

} // namespace mc::entity::ai::goal
