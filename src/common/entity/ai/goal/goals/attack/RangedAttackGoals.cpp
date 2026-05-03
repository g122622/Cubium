#include "RangedAttackGoals.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../core/CreatureEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../interfaces/IRangedAttackMob.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../controller/MovementController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../../attribute/Attributes.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../item/core/Item.hpp"
#include "../../../../../item/core/ItemStack.hpp"
#include "../../../../../item/core/UseAction.hpp"
#include "../../../../../item/items/weapon/BowItem.hpp"
#include "../../../../../core/Types.hpp"
#include <algorithm>
#include <cmath>

namespace mc::entity::ai::goal {

// ==================== RangedAttackGoal ====================

RangedAttackGoal::RangedAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax, f32 attackRadius)
    : m_mob(mob)
    , m_speed(speed)
    , m_attackIntervalMin(attackIntervalMin)
    , m_attackIntervalMax(attackIntervalMax)
    , m_attackRadius(attackRadius)
    , m_maxAttackDistanceSq(attackRadius * attackRadius)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool RangedAttackGoal::shouldExecute() {
    if (!m_mob) return false;

    LivingEntity* target = m_mob->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    m_target = target;
    return true;
}

bool RangedAttackGoal::shouldContinueExecuting() {
    // MC 1.16.5: shouldExecute() || !noPath()
    return shouldExecute() || (m_mob && !m_mob->navigator()->noPath());
}

void RangedAttackGoal::startExecuting() {
    // MC 1.16.5: 初始值在构造函数中不需要设置
}

void RangedAttackGoal::resetTask() {
    m_target = nullptr;
    m_seenTime = 0;
    m_attackTime = -1;  // MC 1.16.5: 重置为 -1
    if (m_mob) {
        m_mob->clearNavigation();
    }
}

void RangedAttackGoal::tick() {
    if (!m_mob || !m_target) return;

    // MC 1.16.5: 计算到目标的距离平方
    f64 distSq = m_mob->distanceSqTo(m_target->x(), m_target->y(), m_target->z());
    f32 dist = std::sqrt(static_cast<f32>(distSq));

    // MC 1.16.5: 检查视线并更新 seenTime
    bool canSee = m_mob->canSee(*m_target);
    if (canSee) {
        m_seenTime++;
    } else {
        m_seenTime = 0;
    }

    // MC 1.16.5: 距离判定和 seenTime 判定
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

    // MC 1.16.5: 使用 LookController 看向目标，deltaYaw=30, deltaPitch=30
    if (auto* lookCtrl = m_mob->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
    }

    // MC 1.16.5: 攻击计时逻辑
    if (--m_attackTime == 0) {
        // 攻击时间到了，检查是否能看见目标
        if (!canSee) {
            return;
        }

        // MC 1.16.5: 计算蓄力程度 = sqrt(distSq) / attackRadius
        f32 charge = dist / m_attackRadius;
        charge = std::clamp(charge, 0.1f, 1.0f);

        performAttack(m_target, charge);

        // MC 1.16.5: 计算下一次攻击时间
        // floor(charge * (max - min) + min)
        m_attackTime = static_cast<i32>(std::floor(charge * static_cast<f32>(m_attackIntervalMax - m_attackIntervalMin) + static_cast<f32>(m_attackIntervalMin)));
    } else if (m_attackTime < 0) {
        // MC 1.16.5: 初始化攻击时间
        f32 charge = dist / m_attackRadius;
        m_attackTime = static_cast<i32>(std::floor(charge * static_cast<f32>(m_attackIntervalMax - m_attackIntervalMin) + static_cast<f32>(m_attackIntervalMin)));
    }
}

void RangedAttackGoal::performAttack(LivingEntity* target, f32 charge) {
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
{
}

bool RangedBowAttackGoal::shouldExecute() {
    if (!m_mob) return false;

    const ItemStack& mainHand = m_mob->getMainHandItem();
    const Item* item = mainHand.getItem();
    if (item == nullptr || item->getUseAction(mainHand) != UseAction::Bow) {
        return false;
    }

    return RangedAttackGoal::shouldExecute();
}

void RangedBowAttackGoal::startExecuting() {
    RangedAttackGoal::startExecuting();
    m_strafingClockwise = false;
    m_strafingBackwards = false;
    m_strafingTime = -1;
    // MC 1.16.5: 设置激怒状态
    if (m_mob) {
        m_mob->setAggroed(true);
    }
}

void RangedBowAttackGoal::resetTask() {
    RangedAttackGoal::resetTask();
    m_strafingClockwise = false;
    m_strafingBackwards = false;
    m_strafingTime = -1;
    // MC 1.16.5: 清除激怒状态并停止使用弓
    if (m_mob) {
        m_mob->setAggroed(false);
        m_mob->stopActiveHand();
    }
}

void RangedBowAttackGoal::tick() {
    if (!m_mob || !m_target) return;

    // 计算到目标的距离平方
    f64 distSq = m_mob->distanceSqTo(m_target->x(), m_target->y(), m_target->z());

    // MC 1.16.5: 检查视线并更新 seeTime
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

    // MC 1.16.5: 看向目标
    if (auto* lookCtrl = m_mob->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
    }

    // MC 1.16.5: 在攻击范围内且能看到目标足够久，停止移动并开始走位
    if (!(distSq > static_cast<f64>(m_maxAttackDistanceSq)) && m_seenTime >= 20) {
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

    // MC 1.16.5: 走位方向变化
    if (m_strafingTime >= 20) {
        math::Random rng = m_mob->getRandom();
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

    // MC 1.16.5: 走位执行
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

    // MC 1.16.5: 弓蓄力和发射逻辑
    // 检查实体是否正在使用物品（蓄力中）
    if (m_mob->isUsingItem()) {
        // 正在蓄力
        if (!canSee && m_seenTime < -60) {
            // 看不到目标太久了，取消蓄力
            m_mob->stopActiveHand();
        } else if (canSee) {
            // 检查蓄力时间
            // MC 1.16.5: getItemInUseMaxCount() = getUseDuration() - getItemInUseCount()
            i32 useDuration = m_mob->getMainHandItem().getItem()->getUseDuration(m_mob->getMainHandItem());
            i32 timeUsed = useDuration - m_mob->getItemInUseCount();
            if (timeUsed >= 20) {
                // 蓄力完成，发射
                m_mob->stopActiveHand();
                performAttack(m_target, item::BowItem::getArrowVelocity(timeUsed));
                m_attackTime = m_attackIntervalMin;
            }
        }
    } else if (--m_attackTime <= 0 && m_seenTime >= -60) {
        // MC 1.16.5: 开始蓄力
        // ProjectileHelper.getHandWith(entity, Items.BOW)
        // 找到持有弓的手
        Hand bowHand = (m_mob->getMainHandItem().getItem() != nullptr &&
                        m_mob->getMainHandItem().getItem()->getUseAction(m_mob->getMainHandItem()) == UseAction::Bow)
                           ? Hand::MainHand
                           : Hand::OffHand;
        m_mob->setActiveHand(bowHand);
    }
}

void RangedBowAttackGoal::performAttack(LivingEntity* target, f32 charge) {
    // 调用基类实现攻击
    RangedAttackGoal::performAttack(target, charge);

    // 设置攻击冷却（弓箭专用逻辑）
    math::Random rng = m_mob->getRandom();
    m_attackTime = m_attackIntervalMin + rng.nextInt(m_attackIntervalMax - m_attackIntervalMin + 1);
}

} // namespace mc::entity::ai::goal
