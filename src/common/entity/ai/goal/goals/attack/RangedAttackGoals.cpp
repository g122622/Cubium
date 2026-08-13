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

#include "RangedAttackGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/projectile/ProjectileHelper.hpp"
#include "common/entity/interfaces/ICrossbowUser.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/BowItem.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/util/math/random/Random.hpp"

#include <algorithm>
#include <cmath>

namespace mc::entity::ai::goal {

// ==================== RangedAttackGoal ====================

RangedAttackGoal::RangedAttackGoal(
    MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax, f32 attackRadius)
    : m_mob(mob)
    , m_speed(speed)
    , m_attackIntervalMin(attackIntervalMin)
    , m_attackIntervalMax(attackIntervalMax)
    , m_attackRadius(attackRadius)
    , m_maxAttackDistanceSq(attackRadius * attackRadius)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool RangedAttackGoal::shouldExecute()
{
    if (!m_mob) return false;

    LivingEntity* target = m_mob->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    m_target = target;
    return true;
}

bool RangedAttackGoal::shouldContinueExecuting()
{
    return shouldExecute() || (m_mob && !m_mob->navigator()->noPath());
}

void RangedAttackGoal::startExecuting()
{
    // 初始值在构造函数中不需要设置
}

void RangedAttackGoal::resetTask()
{
    m_target = nullptr;
    m_seenTime = 0;
    m_attackTime = -1; // 重置为 -1
    if (m_mob) {
        m_mob->clearNavigation();
    }
}

void RangedAttackGoal::tick()
{
    if (!m_mob || !m_target) return;

    // 计算到目标的距离平方
    f64 distSq = m_mob->distanceSqTo(m_target->x(), m_target->y(), m_target->z());
    f32 dist = std::sqrt(static_cast<f32>(distSq));

    // 检查视线并更新 seenTime
    bool canSee = m_mob->canSee(*m_target);
    if (canSee) {
        m_seenTime++;
    } else {
        m_seenTime = 0;
    }

    // 距离判定和 seenTime 判定
    // 如果在攻击距离内 且 能看到目标 >= 5 ticks，停止移动
    if (distSq <= static_cast<f64>(m_maxAttackDistanceSq) && m_seenTime >= MIN_SEEN_TIME) {
        m_mob->clearNavigation();
    } else {
        // 否则向目标移动
        CreatureEntity* creature = dynamic_cast<CreatureEntity*>(m_mob);
        if (creature) {
            creature->tryMoveTo(m_target->x(), m_target->y(), m_target->z(), m_speed);
        }
    }

    // 使用 LookController 看向目标
    if (auto* lookCtrl = m_mob->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
    }

    // 攻击计时逻辑
    if (--m_attackTime == 0) {
        // 攻击时间到了，检查是否能看见目标
        if (!canSee) {
            return;
        }

        // 计算蓄力程度 = sqrt(distSq) / attackRadius
        f32 charge = dist / m_attackRadius;
        charge = std::clamp(charge, 0.1f, 1.0f);

        performAttack(m_target, charge);

        // 计算下一次攻击时间
        // floor(charge * (max - min) + min)
        m_attackTime =
            static_cast<i32>(std::floor(charge * static_cast<f32>(m_attackIntervalMax - m_attackIntervalMin) +
                static_cast<f32>(m_attackIntervalMin)));
    } else if (m_attackTime < 0) {
        // 初始化攻击时间
        f32 charge = dist / m_attackRadius;
        m_attackTime =
            static_cast<i32>(std::floor(charge * static_cast<f32>(m_attackIntervalMax - m_attackIntervalMin) +
                static_cast<f32>(m_attackIntervalMin)));
    }
}

void RangedAttackGoal::performAttack(LivingEntity* target, f32 charge)
{
    // 检查实体是否实现远程攻击接口
    IRangedAttackMob* rangedAttacker = dynamic_cast<IRangedAttackMob*>(m_mob);
    if (rangedAttacker) {
        rangedAttacker->attackEntityWithRangedAttack(target, charge);
    }
}

// ==================== RangedBowAttackGoal ====================

RangedBowAttackGoal::RangedBowAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax)
    : RangedAttackGoal(mob, speed, attackIntervalMin, attackIntervalMax, 15.0f)
    , m_strafingClockwise(false)
    , m_strafingBackwards(false)
    , m_strafingTime(-1)
{}

bool RangedBowAttackGoal::shouldExecute()
{
    if (!m_mob) return false;

    const ItemStack& mainHand = m_mob->getMainHandItem();
    const Item* item = mainHand.getItem();
    if (item == nullptr || item->getUseAction(mainHand) != UseAction::Bow) {
        return false;
    }

    return RangedAttackGoal::shouldExecute();
}

void RangedBowAttackGoal::startExecuting()
{
    RangedAttackGoal::startExecuting();
    m_strafingClockwise = false;
    m_strafingBackwards = false;
    m_strafingTime = -1;
    // 设置激怒状态
    if (m_mob) {
        m_mob->setAggroed(true);
    }
}

void RangedBowAttackGoal::resetTask()
{
    RangedAttackGoal::resetTask();
    m_strafingClockwise = false;
    m_strafingBackwards = false;
    m_strafingTime = -1;
    // 清除激怒状态并停止使用弓
    if (m_mob) {
        m_mob->setAggroed(false);
        m_mob->stopActiveHand();
    }
}

void RangedBowAttackGoal::tick()
{
    if (!m_mob || !m_target) return;

    // 计算到目标的距离平方
    f64 distSq = m_mob->distanceSqTo(m_target->x(), m_target->y(), m_target->z());

    // 检查视线并更新 seeTime
    // 原版逻辑：看得到++，看不到--，而不是简单重置
    bool canSee = m_mob->canSee(*m_target);
    bool wasSeeing = m_seenTime > 0;
    if (canSee != wasSeeing) {
        m_seenTime = 0;
    }
    if (canSee) {
        ++m_seenTime;
    } else {
        --m_seenTime;
    }

    // 看向目标
    if (auto* lookCtrl = m_mob->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
    }

    // 在攻击范围内且能看到目标足够久，停止移动并开始走位
    if (distSq <= static_cast<f64>(m_maxAttackDistanceSq) && m_seenTime >= 20) {
        // 在攻击范围内 - 停止寻路
        m_mob->clearNavigation();
        ++m_strafingTime;
    } else {
        // 不在攻击范围内 - 向目标移动
        CreatureEntity* creature = dynamic_cast<CreatureEntity*>(m_mob);
        if (creature) {
            creature->tryMoveTo(m_target->x(), m_target->y(), m_target->z(), m_speed);
        }
        m_strafingTime = -1;
    }

    // 走位方向变化
    if (m_strafingTime >= 20) {
        math::Random& rng = m_mob->getRandom();
        // 30% 概率改变顺时针/逆时针
        if (rng.nextFloat() < 0.3f) {
            m_strafingClockwise = !m_strafingClockwise;
        }
        // 30% 概率改变前进/后退
        if (rng.nextFloat() < 0.3f) {
            m_strafingBackwards = !m_strafingBackwards;
        }
        m_strafingTime = 0;
    }

    // 走位执行
    if (m_strafingTime > -1) {
        // 根据距离调整前进/后退
        if (distSq > static_cast<f64>(m_maxAttackDistanceSq) * 0.75) {
            m_strafingBackwards = false;
        } else if (distSq < static_cast<f64>(m_maxAttackDistanceSq) * 0.25) {
            m_strafingBackwards = true;
        }

        // 执行走位
        f32 forward = m_strafingBackwards ? -0.5f : 0.5f;
        f32 strafe = m_strafingClockwise ? 0.5f : -0.5f;
        if (auto* moveCtrl = m_mob->moveController()) {
            moveCtrl->strafe(forward, strafe);
        }
    }

    // 弓蓄力和发射逻辑
    // 检查实体是否正在使用物品（蓄力中）
    if (m_mob->isUsingItem()) {
        // 正在蓄力
        if (!canSee && m_seenTime < -60) {
            // 看不到目标太久了，取消蓄力
            m_mob->stopActiveHand();
        } else if (canSee) {
            // 检查蓄力时间
            // getItemInUseMaxCount() = getUseDuration() - getItemInUseCount()
            const ItemStack& mainHand = m_mob->getMainHandItem();
            const Item* item = mainHand.getItem();
            i32 useDuration = item ? item->getUseDuration(mainHand) : 0;
            i32 timeUsed = useDuration - m_mob->getItemInUseCount();
            if (timeUsed >= 20) {
                // 蓄力完成，发射
                m_mob->stopActiveHand();
                performAttack(m_target, item::BowItem::getArrowVelocity(timeUsed));
                m_attackTime = m_attackIntervalMin;
            }
        }
    } else if (--m_attackTime <= 0 && m_seenTime >= -60) {
        // 开始蓄力
        // 使用 getWeaponHoldingHand 找到持有弓的手
        Hand bowHand = getWeaponHoldingHand(*m_mob, Items::BOW);
        m_mob->setActiveHand(bowHand);
    }
}

void RangedBowAttackGoal::performAttack(LivingEntity* target, f32 charge)
{
    // 调用基类实现攻击
    RangedAttackGoal::performAttack(target, charge);

    // 设置攻击冷却（弓箭专用逻辑）
    math::Random& rng = m_mob->getRandom();
    // 防御性 clamp：setMinAttackInterval 已保 max>=min 不变量，但此处兜底确保 bound>=1，
    // 避免任何意外路径（子类直接改 m_attackIntervalMin 等）触发 nextInt 的
    // MC_ASSERT_RELEASE(bound>0) 断言崩溃。
    i32 bound = std::max(1, m_attackIntervalMax - m_attackIntervalMin + 1);
    m_attackTime = m_attackIntervalMin + rng.nextInt(bound);
}

// ==================== RangedCrossbowAttackGoal ====================

RangedCrossbowAttackGoal::RangedCrossbowAttackGoal(MobEntity* mob, f64 speed, f32 attackRadius)
    : m_mob(mob)
    , m_speed(speed)
    , m_attackRadius(attackRadius)
    , m_attackRadiusSq(attackRadius * attackRadius)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool RangedCrossbowAttackGoal::shouldExecute()
{
    if (!m_mob) return false;

    // 检查是否持有弩
    if (!_isHoldingCrossbow()) return false;

    LivingEntity* target = m_mob->attackTarget();
    if (!target || !target->isAlive()) return false;

    m_target = target;
    return true;
}

bool RangedCrossbowAttackGoal::shouldContinueExecuting()
{
    // 继续执行条件
    if (!m_mob || !m_target) return false;

    // 目标仍然有效
    if (!m_target->isAlive()) return false;

    // 仍在追踪范围内
    f64 distSq = m_mob->distanceSqTo(m_target->x(), m_target->y(), m_target->z());
    if (distSq > static_cast<f64>(m_attackRadiusSq * 4.0f)) return false; // 2倍追踪距离

    // 仍持有弩
    return _isHoldingCrossbow();
}

void RangedCrossbowAttackGoal::startExecuting()
{
    m_crossbowState = CrossbowState::Uncharged;
    m_seenTime = 0;
    m_chargeTime = 0;
    m_cooldownTime = 0;
    m_moveCooldown = 0;

    // 设置激怒状态
    if (m_mob) {
        m_mob->setAggroed(true);
    }
}

void RangedCrossbowAttackGoal::resetTask()
{
    m_target = nullptr;
    m_crossbowState = CrossbowState::Uncharged;
    m_seenTime = 0;
    m_chargeTime = 0;
    m_cooldownTime = 0;
    m_moveCooldown = 0;

    if (m_mob) {
        m_mob->setAggroed(false);
        m_mob->stopActiveHand();

        // 重置弩装填状态
        entity::ICrossbowUser* crossbowUser = dynamic_cast<entity::ICrossbowUser*>(m_mob);
        if (crossbowUser) {
            crossbowUser->setChargingCrossbow(false);
        }

        m_mob->clearNavigation();
    }
}

void RangedCrossbowAttackGoal::tick()
{
    if (!m_mob || !m_target) return;

    // 更新视线时间
    _updateSeenTime();

    // 看向目标
    if (auto* lookCtrl = m_mob->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
    }

    // 计算到目标的距离
    f64 distSq = m_mob->distanceSqTo(m_target->x(), m_target->y(), m_target->z());

    // 移动逻辑
    bool shouldMove =
        (distSq > static_cast<f64>(m_attackRadiusSq) || m_seenTime < MIN_SEEN_TIME) && m_moveCooldown == 0;

    if (shouldMove && m_crossbowState != CrossbowState::Charging) {
        // 向目标移动
        CreatureEntity* creature = dynamic_cast<CreatureEntity*>(m_mob);
        if (creature) {
            creature->tryMoveTo(m_target->x(), m_target->y(), m_target->z(), m_speed);
        }
        // 设置移动冷却
        math::Random& rng = m_mob->getRandom();
        m_moveCooldown = MOVE_COOLDOWN_MIN + rng.nextInt(MOVE_COOLDOWN_MAX - MOVE_COOLDOWN_MIN + 1);
    } else if (distSq <= static_cast<f64>(m_attackRadiusSq) && m_seenTime >= MIN_SEEN_TIME) {
        // 在攻击范围内，停止移动
        m_mob->clearNavigation();
    }

    // 更新移动冷却
    if (m_moveCooldown > 0) {
        --m_moveCooldown;
    }

    // 状态机处理
    switch (m_crossbowState) {
        case CrossbowState::Uncharged:
            _handleUnchargedState();
            break;
        case CrossbowState::Charging:
            _handleChargingState();
            break;
        case CrossbowState::Charged:
            _handleChargedState();
            break;
        case CrossbowState::ReadyToAttack:
            _handleReadyToAttackState();
            break;
    }
}

bool RangedCrossbowAttackGoal::_isHoldingCrossbow() const
{
    if (!m_mob) return false;

    const ItemStack& mainHand = m_mob->getMainHandItem();
    const Item* item = mainHand.getItem();

    // 检查是否是弩
    return item != nullptr && item->getUseAction(mainHand) == UseAction::Crossbow;
}

void RangedCrossbowAttackGoal::_updateSeenTime()
{
    if (!m_mob || !m_target) return;

    bool canSee = m_mob->canSee(*m_target);

    // 递增/递减而不是重置
    if (canSee) {
        ++m_seenTime;
    } else {
        --m_seenTime;
        if (m_seenTime < 0) m_seenTime = 0;
    }
}

void RangedCrossbowAttackGoal::_handleUnchargedState()
{
    // 在攻击范围内且能看到目标时开始装填
    if (m_seenTime >= MIN_SEEN_TIME && m_moveCooldown == 0) {
        // 开始装填
        m_mob->setActiveHand(Hand::MainHand);
        m_crossbowState = CrossbowState::Charging;
        m_chargeTime = 0;

        // 设置装填状态
        entity::ICrossbowUser* crossbowUser = dynamic_cast<entity::ICrossbowUser*>(m_mob);
        if (crossbowUser) {
            crossbowUser->setChargingCrossbow(true);
        }
    }
}

void RangedCrossbowAttackGoal::_handleChargingState()
{
    // 检查装填进度
    const ItemStack& mainHand = m_mob->getMainHandItem();
    const Item* item = mainHand.getItem();

    if (item == nullptr || item->getUseAction(mainHand) != UseAction::Crossbow) {
        // 弩丢失，重置状态
        m_crossbowState = CrossbowState::Uncharged;
        return;
    }

    // 获取装填时间
    entity::ICrossbowUser* crossbowUser = dynamic_cast<entity::ICrossbowUser*>(m_mob);
    i32 chargeTimeRequired = crossbowUser ? crossbowUser->getCrossbowChargeTime() : 25;

    // 如果是玩家，需要考虑快速装填附魔
    const item::CrossbowItem* crossbowItem = dynamic_cast<const item::CrossbowItem*>(item);
    if (crossbowItem) {
        chargeTimeRequired = item::CrossbowItem::getChargeTime(mainHand);
    }

    ++m_chargeTime;

    // 检查是否装填完成
    if (m_chargeTime >= chargeTimeRequired) {
        // 装填完成
        m_mob->stopActiveHand();

        // 设置弩为已装填状态
        if (crossbowItem) {
            item::CrossbowItem::setCharged(m_mob->getMutableMainHandItem(), true);
        }

        // 调用装填完成回调
        if (crossbowUser) {
            crossbowUser->setChargingCrossbow(false);
            crossbowUser->onCrossbowLoadComplete(m_mob->getMutableMainHandItem());
        }

        m_crossbowState = CrossbowState::Charged;

        // 设置装填后等待时间
        math::Random& rng = m_mob->getRandom();
        m_cooldownTime = CHARGED_WAIT_MIN + rng.nextInt(CHARGED_WAIT_MAX - CHARGED_WAIT_MIN + 1);
    }
}

void RangedCrossbowAttackGoal::_handleChargedState()
{
    // 等待一段时间后进入攻击状态
    --m_cooldownTime;
    if (m_cooldownTime <= 0) {
        m_crossbowState = CrossbowState::ReadyToAttack;
    }
}

void RangedCrossbowAttackGoal::_handleReadyToAttackState()
{
    // 看到目标时发射
    bool canSee = m_mob->canSee(*m_target);
    if (!canSee) {
        // 看不到目标，重置到已装填状态等待
        m_crossbowState = CrossbowState::Charged;
        m_cooldownTime = CHARGED_WAIT_MIN;
        return;
    }

    // 发射弩箭
    ItemStack& mainHand = m_mob->getMutableMainHandItem();
    entity::ICrossbowUser* crossbowUser = dynamic_cast<entity::ICrossbowUser*>(m_mob);

    if (crossbowUser && _isHoldingCrossbow()) {
        // 发射（shootCrossbow 内部会根据弹药类型决定速度）
        crossbowUser->shootCrossbow(m_target, mainHand, 1.0f);

        // 清除装填状态
        const item::CrossbowItem* crossbowItem = dynamic_cast<const item::CrossbowItem*>(mainHand.getItem());
        if (crossbowItem) {
            item::CrossbowItem::setCharged(mainHand, false);
        }
    }

    // 重置状态
    m_crossbowState = CrossbowState::Uncharged;
}

} // namespace mc::entity::ai::goal
