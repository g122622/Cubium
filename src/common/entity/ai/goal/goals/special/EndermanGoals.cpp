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

#include "EndermanGoals.hpp"

#include "../../../../entities/monster/end/EndermanEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// EndermanStareGoal 实现
// ============================================================================

EndermanStareGoal::EndermanStareGoal(EndermanEntity* enderman)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look, GoalFlag::Move})
    , m_enderman(enderman)
{
    MC_ASSERT(enderman != nullptr);
}

bool EndermanStareGoal::shouldExecute()
{
    // MC 1.16.5: EndermanEntity.StareGoal.shouldExecute()
    // 获取攻击目标
    m_targetPlayer = m_enderman->getAttackTarget();

    // 检查是否是玩家
    if (m_targetPlayer == nullptr || m_targetPlayer->legacyType() != LegacyEntityType::Player) {
        return false;
    }

    // 检查距离是否在 16 格内
    f64 distSq = m_enderman->distanceSqTo(*m_targetPlayer);
    if (distSq > STARE_RANGE_SQ) {
        return false;
    }

    // 检查玩家是否正在注视末影人
    Player* player = dynamic_cast<Player*>(m_targetPlayer);
    if (player == nullptr) {
        return false;
    }
    return m_enderman->shouldAttackPlayer(*player);
}

void EndermanStareGoal::startExecuting()
{
    // MC 1.16.5: 清除导航路径
    m_enderman->navigator()->clearPath();
}

void EndermanStareGoal::resetTask()
{
    m_targetPlayer = nullptr;
}

void EndermanStareGoal::tick()
{
    // MC 1.16.5: 注视目标玩家的眼睛位置
    if (m_targetPlayer != nullptr) {
        m_enderman->lookController()->setLookPosition(
            m_targetPlayer->x(),
            m_targetPlayer->y() + m_targetPlayer->eyeHeight(),
            m_targetPlayer->z());
    }
}

// ============================================================================
// EndermanFindPlayerGoal 实现
// ============================================================================

EndermanFindPlayerGoal::EndermanFindPlayerGoal(EndermanEntity* enderman)
    : TargetGoal(enderman, true)
    , m_enderman(enderman)
{
    MC_ASSERT(enderman != nullptr);
}

bool EndermanFindPlayerGoal::shouldExecute()
{
    // MC 1.16.5: EndermanEntity.FindPlayerGoal.shouldExecute()
    // 在附近查找正在注视末影人的玩家
    IWorld* world = m_enderman->world();
    if (world == nullptr) {
        return false;
    }

    // 获取附近的所有实体
    std::vector<Entity*> entities = world->getEntitiesInRange(
        m_enderman->position(),
        static_cast<f32>(TARGET_DISTANCE),
        m_enderman);

    // 遍历寻找最近符合条件的玩家
    Player* closestPlayer = nullptr;
    f64 closestDistSq = std::numeric_limits<f64>::max();

    for (Entity* entity : entities) {
        if (!entity->isAlive()) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 创造模式和观察者模式玩家不被攻击
        if (player->isCreative() || player->isSpectator()) {
            continue;
        }

        // 检查玩家是否正在注视末影人
        if (!m_enderman->shouldAttackPlayer(*player)) {
            continue;
        }

        // 检查距离
        f64 distSq = m_enderman->distanceSqTo(*player);
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestPlayer = player;
        }
    }

    m_targetPlayer = closestPlayer;
    return m_targetPlayer != nullptr;
}

void EndermanFindPlayerGoal::startExecuting()
{
    // MC 1.16.5: 设置激怒计时器并设置愤怒状态
    m_aggroTime = AGGRO_DURATION;
    m_teleportTime = 0;

    // 设置被注视状态
    m_enderman->setScreaming(true);
}

void EndermanFindPlayerGoal::resetTask()
{
    m_targetPlayer = nullptr;
    TargetGoal::resetTask();
}

bool EndermanFindPlayerGoal::shouldContinueExecuting()
{
    // MC 1.16.5: EndermanEntity.FindPlayerGoal.shouldContinueExecuting()
    if (m_targetPlayer != nullptr) {
        // 如果玩家不再注视末影人，停止
        if (!m_enderman->shouldAttackPlayer(*m_targetPlayer)) {
            return false;
        }

        // 继续注视玩家
        m_enderman->lookController()->setLookPositionWithEntity(*m_targetPlayer, 10.0f, 10.0f);
        return true;
    }

    // 如果有攻击目标，检查是否还能看到
    if (m_target != nullptr) {
        // 使用 TargetGoal 的默认视线检查
        if (m_checkSight && m_unseenTicks > MAX_UNSEEN_TICKS) {
            return false;
        }
        return m_target->isAlive();
    }

    return TargetGoal::shouldContinueExecuting();
}

void EndermanFindPlayerGoal::tick()
{
    // MC 1.16.5: EndermanEntity.FindPlayerGoal.tick()
    if (m_enderman->getAttackTarget() == nullptr) {
        // 还没有被激怒，继续注视玩家
        m_target = nullptr;
    }

    if (m_targetPlayer != nullptr) {
        // 倒计时激怒时间
        if (--m_aggroTime <= 0) {
            // 激怒末影人，设置攻击目标
            m_target = m_targetPlayer;
            m_targetPlayer = nullptr;
            TargetGoal::startExecuting();
        }
    } else {
        // 已经被激怒，处理攻击逻辑
        if (m_target != nullptr && !m_enderman->isRiding()) {
            Player* playerTarget = dynamic_cast<Player*>(m_target);

            if (playerTarget != nullptr && m_enderman->shouldAttackPlayer(*playerTarget)) {
                // 玩家仍在注视末影人
                f64 distSq = m_enderman->distanceSqTo(*m_target);

                // 近距离瞬移躲避
                if (distSq < TELEPORT_NEAR_DISTANCE_SQ) {
                    m_enderman->teleport();
                    m_teleportTime = 0;
                }
            } else if (m_target->distanceSqTo(*m_enderman) > TELEPORT_FAR_DISTANCE_SQ) {
                // 远距离瞬移接近
                if (m_teleportTime++ >= TELEPORT_COOLDOWN_TICKS) {
                    if (m_enderman->teleportToTarget()) {
                        m_teleportTime = 0;
                    }
                }
            }
        }

        TargetGoal::tick();
    }
}

bool EndermanFindPlayerGoal::shouldAttackPlayer(Player* player) const
{
    if (player == nullptr) {
        return false;
    }
    return m_enderman->shouldAttackPlayer(*player);
}

} // namespace entity::ai::goal
} // namespace mc
