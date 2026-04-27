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
    , m_isBowCharging(false)
    , m_chargeTime(0)
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
    m_isBowCharging = false;
    m_chargeTime = 0;
    m_strafingClockwise = false;
    m_strafingBackwards = false;
    m_strafingTime = -1;
}

void RangedBowAttackGoal::resetTask() {
    RangedAttackGoal::resetTask();
    m_isBowCharging = false;
    m_chargeTime = 0;
    m_strafingClockwise = false;
    m_strafingBackwards = false;
    m_strafingTime = -1;
}

void RangedBowAttackGoal::tick() {
    if (!m_mob || !m_target) return;

    // 计算到目标的距离
    f64 distSq = m_mob->distanceSqTo(m_target->x(), m_target->y(), m_target->z());
    f32 dist = std::sqrt(static_cast<f32>(distSq));

    // 看向目标
    if (auto* lookCtrl = m_mob->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
    }

    // MC 1.16.5: 走位逻辑
    // 走位时间计数器递增
    m_strafingTime++;

    // MC 1.16.5: 每隔 STRAFE_THRESHOLD(20) tick 有概率改变走位方向
    if (m_strafingTime >= STRAFE_THRESHOLD) {
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

    // 检查是否在攻击范围内
    if (dist <= m_attackRadius && dist >= m_attackRadius * 0.5f) {
        // 在攻击范围内 - 走位模式
        if (m_strafingTime > -1) {
            // MC 1.16.5: 使用 MovementController 的 strafe 方法
            f32 forward = m_strafingBackwards ? -0.5f : 0.5f;
            f32 strafe = m_strafingClockwise ? 0.5f : -0.5f;

            if (auto* moveCtrl = m_mob->moveController()) {
                moveCtrl->strafe(forward, strafe);
            }
        }

        // 蓄力和发射逻辑
        if (m_isBowCharging) {
            m_chargeTime++;

            if (m_chargeTime >= BOW_CHARGE_TIME) {
                // 蓄力完成，发射
                performAttack(m_target, 1.0f);
                m_isBowCharging = false;
                m_chargeTime = 0;
            }
        } else if (m_attackTime <= 0) {
            // 开始蓄力
            m_isBowCharging = true;
            m_chargeTime = 0;
        }
    } else {
        // 不在攻击范围内，取消蓄力和走位，向目标移动
        if (m_isBowCharging) {
            m_isBowCharging = false;
            m_chargeTime = 0;
        }
        m_strafingTime = -1;

        // 尝试转换为CreatureEntity以使用tryMoveTo
        CreatureEntity* creature = dynamic_cast<CreatureEntity*>(m_mob);
        if (creature) {
            creature->tryMoveTo(m_target->x(), m_target->y(), m_target->z(), m_speed);
        }
    }
}

void RangedBowAttackGoal::performAttack(LivingEntity* target, f32 charge) {
    // 检查实体是否实现远程攻击接口
    IRangedAttackMob* rangedAttacker = dynamic_cast<IRangedAttackMob*>(m_mob);
    if (rangedAttacker) {
        rangedAttacker->attackEntityWithRangedAttack(target, charge);
    }

    // 设置攻击冷却
    math::Random rng = m_mob->getRandom();
    m_attackTime = m_attackIntervalMin + rng.nextInt(m_attackIntervalMax - m_attackIntervalMin + 1);
}

} // namespace mc::entity::ai::goal
