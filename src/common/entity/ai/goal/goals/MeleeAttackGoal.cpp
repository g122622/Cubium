#include "MeleeAttackGoal.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../GoalConstants.hpp"
#include "../../controller/LookController.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

MeleeAttackGoal::MeleeAttackGoal(CreatureEntity* creature, f64 speed, bool useLongMemory)
    : m_creature(creature)
    , m_speed(speed)
    , m_useLongMemory(useLongMemory)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool MeleeAttackGoal::shouldExecute() {
    if (!m_creature) return false;

    // 获取攻击目标
    LivingEntity* target = m_creature->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    m_attackTarget = target;
    return true;
}

bool MeleeAttackGoal::shouldContinueExecuting() {
    if (!m_creature || !m_attackTarget) return false;

    // 检查目标是否存活
    if (!m_attackTarget->isAlive()) {
        return false;
    }

    // 检查距离
    f32 distSq = m_creature->distanceSqTo(*m_attackTarget);

    if (distSq > MELEE_ATTACK_STOP_DISTANCE_SQ) {
        return false; // 目标太远，停止追踪
    }

    // 如果使用长期记忆，继续追踪
    if (m_useLongMemory) {
        return true;
    }

    return shouldExecute();
}

void MeleeAttackGoal::startExecuting() {
    m_attackCooldown = 0;
    m_pathRecalculateTimer = 0;
}

void MeleeAttackGoal::resetTask() {
    m_attackTarget = nullptr;
    if (m_creature) {
        m_creature->clearNavigation();
    }
}

void MeleeAttackGoal::tick() {
    if (!m_creature || !m_attackTarget) return;

    // 看向目标
    m_creature->lookAt(*m_attackTarget);

    // 减少攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 检查并更新路径
    checkPath();

    // 检查是否可以攻击
    if (canAttack(m_attackTarget) && m_attackCooldown <= 0) {
        attackTarget(m_attackTarget);
        m_attackCooldown = MELEE_ATTACK_COOLDOWN;
    }
}

bool MeleeAttackGoal::canAttack(LivingEntity* target) const {
    if (!m_creature || !target) return false;

    // 检查距离
    f32 distSq = m_creature->distanceSqTo(*target);

    // 检查是否在攻击范围内
    return distSq <= MELEE_ATTACK_REACH_SQ;
}

void MeleeAttackGoal::attackTarget(LivingEntity* target) {
    if (!m_creature || !target) return;

    // 获取攻击者的攻击伤害属性
    using namespace mc::entity::attribute;
    f32 damage = static_cast<f32>(m_creature->getAttributeValue(Attributes::ATTACK_DAMAGE, 1.0));

    // 创建伤害来源
    EntityDamageSource damageSource(DamageType::MobAttack, m_creature);

    // 应用伤害
    target->hurt(damageSource, damage);

    // 应用击退
    f32 knockbackStrength = static_cast<f32>(
        m_creature->getAttributeValue(Attributes::ATTACK_KNOCKBACK, 1.0));

    if (knockbackStrength > 0.0f) {
        // 计算击退方向（使用距离平方避免sqrt）
        f32 dx = target->x() - m_creature->x();
        f32 dz = target->z() - m_creature->z();
        f32 distSq = dx * dx + dz * dz;

        if (distSq > 0.000001f) {
            // TODO: 使用快速逆平方根近似
            f32 invDist = 1.0f / std::sqrt(distSq);
            dx *= invDist;
            dz *= invDist;
        }

        // 应用击退速度
        f32 knockbackX = dx * knockbackStrength * 0.5f;
        f32 knockbackY = 0.1f * knockbackStrength;
        f32 knockbackZ = dz * knockbackStrength * 0.5f;

        target->addVelocity(knockbackX, knockbackY, knockbackZ);
    }
}

void MeleeAttackGoal::checkPath() {
    if (!m_creature || !m_attackTarget) return;

    m_pathRecalculateTimer--;

    if (m_pathRecalculateTimer <= 0) {
        m_pathRecalculateTimer = PATH_RECALCULATE_INTERVAL;

        // 移动到目标
        m_creature->tryMoveTo(
            m_attackTarget->x(),
            m_attackTarget->y(),
            m_attackTarget->z(),
            m_speed
        );
    }
}

} // namespace mc::entity::ai::goal
