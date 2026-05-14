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
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../controller/LookController.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../GoalConstants.hpp"
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

bool TemptGoal::shouldExecute()
{
    if (!m_creature) return false;

    // MC 1.16.5: 检查冷却
    if (m_delayTemptCounter > 0) {
        m_delayTemptCounter--;
        return false;
    }

    // MC 1.16.5: 寻找手持诱惑物品的玩家
    // 使用 EntityPredicate.setDistance(10.0D).allowInvulnerable().allowFriendlyFire()
    //               .setSkipAttackChecks().setLineOfSiteRequired()
    m_temptingPlayer = findTemptingPlayer();
    return m_temptingPlayer != nullptr;
}

bool TemptGoal::shouldContinueExecuting()
{
    if (!m_creature || !m_temptingPlayer) return false;

    // MC 1.16.5: 检查玩家是否存活
    if (!m_temptingPlayer->isAlive()) return false;

    // MC 1.16.5: 检查玩家手持物品
    const ItemStack& mainHand = m_temptingPlayer->getHeldItem(Hand::MainHand);
    const ItemStack& offHand = m_temptingPlayer->getHeldItem(Hand::OffHand);
    if (!isTempting(mainHand) && !isTempting(offHand)) {
        return false;
    }

    // MC 1.16.5: 检查距离
    f64 distSq = m_creature->distanceSqTo(*m_temptingPlayer);
    if (distSq > TEMPT_RANGE * TEMPT_RANGE) {
        return false;
    }

    // MC 1.16.5: 检查是否被玩家移动吓跑
    if (m_scaredByMovement) {
        // MC 1.16.5: 使用 36.0D（6*6）作为惊吓距离检测
        if (distSq < TEMPT_SCARE_DISTANCE_SQ) {
            // 检查玩家是否移动
            f64 playerDx = m_temptingPlayer->x() - m_targetX;
            f64 playerDy = m_temptingPlayer->y() - m_targetY;
            f64 playerDz = m_temptingPlayer->z() - m_targetZ;
            f64 playerDistSq = playerDx * playerDx + playerDy * playerDy + playerDz * playerDz;

            if (playerDistSq > MOVEMENT_THRESHOLD) {
                return false; // 玩家移动了，停止
            }

            // MC 1.16.5: 检查玩家视角变化
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

    // MC 1.16.5: 记录玩家初始位置和视角
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

    // MC 1.16.5: 设置冷却（100 tick）
    m_delayTemptCounter = TEMPT_COOLDOWN;
}

void TemptGoal::tick()
{
    if (!m_creature || !m_temptingPlayer) return;

    // MC 1.16.5: 使用 getHorizontalFaceSpeed() + 20 和 getVerticalFaceSpeed()
    f32 deltaYaw = static_cast<f32>(m_creature->getHorizontalFaceSpeed() + 20.0);
    f32 deltaPitch = static_cast<f32>(m_creature->getVerticalFaceSpeed());

    // MC 1.16.5: 看向玩家（使用 LookController）
    if (auto* lookCtrl = m_creature->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_temptingPlayer, deltaYaw, deltaPitch);
    }

    // MC 1.16.5: 使用 6.25D（2.5*2.5）作为近距离阈值
    f64 distSq = m_creature->distanceSqTo(*m_temptingPlayer);

    if (distSq < TEMPT_CLOSE_DISTANCE_SQ) {
        // 距离太近，停止移动
        m_creature->clearNavigation();
    } else {
        // MC 1.16.5: 跟随玩家
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

    // MC 1.16.5: 使用 EntityPredicate 搜索玩家
    // EntityPredicate.setDistance(10.0D).allowInvulnerable().allowFriendlyFire()
    //               .setSkipAttackChecks().setLineOfSiteRequired()
    return EntityUtils::findClosestEntity<Player>(
        m_creature->world(), m_creature->position(), TEMPT_RANGE, m_creature, [this](Player* playerEntity) {
            const ItemStack& mainHand = playerEntity->getHeldItem(Hand::MainHand);
            const ItemStack& offHand = playerEntity->getHeldItem(Hand::OffHand);
            return isTempting(mainHand) || isTempting(offHand);
        });
}

} // namespace mc::entity::ai::goal
