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
#include <cmath>

namespace mc::entity::ai::goal {

// ==================== RangedAttackGoal ====================

RangedAttackGoal::RangedAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax, f32 attackRadius)
    : m_mob(mob)
    , m_speed(speed)
    , m_attackIntervalMin(attackIntervalMin)
    , m_attackIntervalMax(attackIntervalMax)
    , m_attackRadius(attackRadius)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool RangedAttackGoal::shouldExecute() {
    if (!m_mob) return false;

    // 获取攻击目标
    LivingEntity* target = m_mob->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    m_target = target;
    return true;
}

bool RangedAttackGoal::shouldContinueExecuting() {
    if (!m_mob || !m_target) return false;

    // 检查目标是否存活
    if (!m_target->isAlive()) {
        return false;
    }

    // 检查距离
    f64 distSq = m_mob->distanceSqTo(*m_target);
    f64 maxDistSq = (m_attackRadius * 2.0) * (m_attackRadius * 2.0);

    if (distSq > maxDistSq) {
        return false;
    }

    // 检查是否还能看到目标
    if (m_seenTime > MAX_SEEN_TIME) {
        return false;
    }

    return true;
}

void RangedAttackGoal::startExecuting() {
    m_attackTime = 0;
    m_seenTime = 0;
}

void RangedAttackGoal::resetTask() {
    m_target = nullptr;
    m_seenTime = 0;
    if (m_mob) {
        m_mob->clearNavigation();
    }
}

void RangedAttackGoal::tick() {
    if (!m_mob || !m_target) return;

    // 更新攻击冷却
    if (m_attackTime > 0) {
        m_attackTime--;
    }

    // 计算到目标的距离
    f64 distSq = m_mob->distanceSqTo(*m_target);
    f64 dist = std::sqrt(distSq);

    // 看向目标
    m_mob->lookAt(*m_target);

    // 检查视线
    bool canSee = m_mob->canSee(*m_target);
    if (canSee) {
        m_seenTime = 0;
    } else {
        m_seenTime++;
    }

    // 判断是否在攻击范围内
    if (isWithinAttackDistance(dist)) {
        // 在攻击范围内，停止移动并攻击
        m_mob->clearNavigation();

        // 攻击
        if (m_attackTime <= 0 && canSee) {
            // 计算蓄力程度（距离越远蓄力越满）
            f32 charge = static_cast<f32>(dist / m_attackRadius);
            charge = std::clamp(charge, 0.1f, 1.0f);

            performAttack(m_target, charge);

            // 设置攻击间隔
            math::Random rng = m_mob->getRandom();
            m_attackTime = m_attackIntervalMin + rng.nextInt(m_attackIntervalMax - m_attackIntervalMin);
        }
    } else {
        // 不在攻击范围内，向目标移动
        // 尝试转换为CreatureEntity以使用tryMoveTo
        CreatureEntity* creature = dynamic_cast<CreatureEntity*>(m_mob);
        if (creature) {
            creature->tryMoveTo(m_target->x(), m_target->y(), m_target->z(), m_speed);
        }

        // 随机移动（环绕）以避免站桩
        if (dist < m_attackRadius * 0.5) {
            // 太近，后退
            if (!m_strafingBackwards) {
                m_strafingBackwards = true;
                m_strafingClockwise = m_mob->getRandom().nextBoolean();
            }
        } else {
            m_strafingBackwards = false;
        }
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
{
}

bool RangedBowAttackGoal::shouldExecute() {
    if (!m_mob) return false;

    // TODO: 检查是否持有弓
    // ItemStack mainHand = m_mob->getMainHandItem();
    // if (!mainHand.getItem().isBow()) return false;

    return RangedAttackGoal::shouldExecute();
}

void RangedBowAttackGoal::tick() {
    if (!m_mob || !m_target) return;

    // 计算到目标的距离
    f64 distSq = m_mob->distanceSqTo(*m_target);
    f64 dist = std::sqrt(distSq);

    // 看向目标
    m_mob->lookAt(*m_target);

    // 检查是否在攻击范围内
    if (isWithinAttackDistance(dist)) {
        // 在攻击范围内
        if (m_isBowCharging) {
            // 正在蓄力
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

            // 停止移动
            m_mob->clearNavigation();
        }
    } else {
        // 不在攻击范围内，向目标移动
        if (m_isBowCharging) {
            // 取消蓄力
            m_isBowCharging = false;
            m_chargeTime = 0;
        }

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
    m_attackTime = m_attackIntervalMin + rng.nextInt(m_attackIntervalMax - m_attackIntervalMin);
}

} // namespace mc::entity::ai::goal
