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

#include "TemptGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "entity/ai/controller/LookController.hpp"
#include "entity/ai/goal/GoalConstants.hpp"
#include "entity/ai/pathfinding/PathNavigator.hpp"
#include "entity/core/CreatureEntity.hpp"
#include "entity/core/EntityUtils.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/entities/player/Player.hpp"

#include <cmath>
#include <utility>

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

bool TemptGoal::shouldExecute()
{
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

bool TemptGoal::shouldContinueExecuting()
{
    if (!m_creature || !m_temptingPlayer) return false;

    // 检查玩家是否存活
    if (!m_temptingPlayer->isAlive()) return false;

    // 检查玩家手持物品
    const ItemStack& mainHand = m_temptingPlayer->getHeldItem(Hand::MainHand);
    const ItemStack& offHand = m_temptingPlayer->getHeldItem(Hand::OffHand);
    if (!isTempting(mainHand) && !isTempting(offHand)) {
        return false;
    }

    // 检查距离
    f64 distSq = m_creature->distanceSqTo(*m_temptingPlayer);
    if (distSq > TEMPT_RANGE_SQ) {
        return false;
    }

    // 检查是否被玩家移动吓跑
    if (m_scaredByMovement) {
        if (distSq < TEMPT_SCARE_DISTANCE_SQ) {
            // 检查玩家是否移动
            f64 playerDx = m_temptingPlayer->x() - m_targetX;
            f64 playerDy = m_temptingPlayer->y() - m_targetY;
            f64 playerDz = m_temptingPlayer->z() - m_targetZ;
            f64 playerDistSq = playerDx * playerDx + playerDy * playerDy + playerDz * playerDz;

            if (playerDistSq > MOVEMENT_THRESHOLD) {
                return false; // 玩家移动了，停止
            }

            // 检查玩家视角变化
            f64 pitchDiff = std::abs(static_cast<f64>(m_temptingPlayer->pitch()) - m_prevPitch);
            f64 yawDiff = std::abs(static_cast<f64>(m_temptingPlayer->yaw()) - m_prevYaw);

            if (pitchDiff > VIEW_CHANGE_THRESHOLD || yawDiff > VIEW_CHANGE_THRESHOLD) {
                return false; // 玩家视角变化，停止
            }
        } else {
            // 更新目标位置
            m_targetX = m_temptingPlayer->x();
            m_targetY = m_temptingPlayer->y();
            m_targetZ = m_temptingPlayer->z();
        }

        m_prevPitch = static_cast<f64>(m_temptingPlayer->pitch());
        m_prevYaw = static_cast<f64>(m_temptingPlayer->yaw());
    }

    return shouldExecute();
}

void TemptGoal::startExecuting()
{
    if (!m_temptingPlayer) return;

    // 记录玩家初始位置和视角
    m_targetX = m_temptingPlayer->x();
    m_targetY = m_temptingPlayer->y();
    m_targetZ = m_temptingPlayer->z();
    m_prevPitch = static_cast<f64>(m_temptingPlayer->pitch());
    m_prevYaw = static_cast<f64>(m_temptingPlayer->yaw());
    m_isRunning = true;
}

void TemptGoal::resetTask()
{
    m_temptingPlayer = nullptr;
    m_isRunning = false;

    if (m_creature) {
        m_creature->clearNavigation();
    }

    // 设置冷却
    m_delayTemptCounter = TEMPT_COOLDOWN;
}

void TemptGoal::tick()
{
    if (!m_creature || !m_temptingPlayer) return;

    // 计算头部旋转速度
    f32 deltaYaw = static_cast<f32>(m_creature->getHorizontalFaceSpeed() + 20.0);
    f32 deltaPitch = static_cast<f32>(m_creature->getVerticalFaceSpeed());

    // 看向玩家
    if (auto* lookCtrl = m_creature->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_temptingPlayer, deltaYaw, deltaPitch);
    }

    // 检查距离，决定是否移动
    f64 distSq = m_creature->distanceSqTo(*m_temptingPlayer);

    if (distSq < TEMPT_CLOSE_DISTANCE_SQ) {
        // 距离太近，停止移动
        m_creature->clearNavigation();
    } else {
        // 跟随玩家
        if (auto* nav = m_creature->navigator()) {
            static_cast<void>(nav->moveTo(*m_temptingPlayer, m_speed));
        }
    }
}

bool TemptGoal::isTempting(const ItemStack& stack) const
{
    return m_itemPredicate(stack);
}

bool TemptGoal::isScaredByPlayerMovement() const
{
    return m_scaredByMovement;
}

Player* TemptGoal::findTemptingPlayer()
{
    if (!m_creature || !m_creature->world()) return nullptr;

    // 搜索附近手持诱惑物品的玩家
    return EntityUtils::findClosestEntity<Player>(
        m_creature->world(), m_creature->position(), TEMPT_RANGE, m_creature, [this](Player* playerEntity) {
            const ItemStack& mainHand = playerEntity->getHeldItem(Hand::MainHand);
            const ItemStack& offHand = playerEntity->getHeldItem(Hand::OffHand);
            return isTempting(mainHand) || isTempting(offHand);
        });
}

} // namespace mc::entity::ai::goal
