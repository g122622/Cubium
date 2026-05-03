#include "GuardianAttackGoal.hpp"
#include "../../../../entities/monster/ocean/GuardianEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../damage/DamageSource.hpp"
#include "../../../../../util/assert/AssertAll.hpp"
#include "common/network/packet/EntityPackets.hpp"

namespace mc::entity::ai::goal {

GuardianAttackGoal::GuardianAttackGoal(GuardianEntity* guardian)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_guardian(guardian)
{
    MC_ASSERT_RELEASE(guardian != nullptr);
}

bool GuardianAttackGoal::shouldExecute() {
    if (m_guardian == nullptr) {
        return false;
    }

    // 选择目标
    LivingEntity* target = selectTarget();
    if (target == nullptr) {
        return false;
    }

    m_target = target;
    return true;
}

bool GuardianAttackGoal::shouldContinueExecuting() {
    if (m_guardian == nullptr || m_target == nullptr) {
        return false;
    }

    // 检查目标是否仍然有效
    if (!isTargetValid(m_target)) {
        return false;
    }

    // 检查目标是否在范围内
    f64 distSq = m_guardian->distanceSqTo(*m_target);
    if (distSq > static_cast<f64>(ATTACK_RANGE * ATTACK_RANGE)) {
        // 目标太远，但可以继续追踪一小段时间
        return m_attackTime < COOLDOWN_DURATION * 2;
    }

    return true;
}

void GuardianAttackGoal::startExecuting() {
    if (m_guardian == nullptr) {
        return;
    }

    m_attackTime = 0;
    m_isCharging = true;
    m_guardian->setLaserCharging(true);
    m_guardian->setLaserChargeTime(0);

    // 设置目标实体ID
    if (m_target != nullptr) {
        m_guardian->setTargetEntity(m_target->id());
    }

    // 广播实体状态事件 (状态21 = GuardianAttack)
    // MC 1.16.5: 在开始充能时发送状态21触发客户端声音
    if (m_guardian->world()) {
        m_guardian->world()->broadcastEntityStatus(
            m_guardian->id(),
            static_cast<u8>(network::EntityStatusPacket::Status::GuardianAttack));
    }
}

void GuardianAttackGoal::resetTask() {
    m_target = nullptr;
    m_attackTime = 0;
    m_isCharging = false;

    if (m_guardian != nullptr) {
        m_guardian->setLaserCharging(false);
        m_guardian->setLaserChargeTime(0);
        m_guardian->setTargetEntity(0);
    }
}

void GuardianAttackGoal::tick() {
    if (m_guardian == nullptr || m_target == nullptr) {
        return;
    }

    // 看向目标
    m_guardian->lookController()->setLookPosition(
        m_target->x(),
        m_target->y() + m_target->eyeHeight(),
        m_target->z(),
        30.0f,  // 头部最大转动角度
        30.0f   // 身体最大转动角度
    );

    m_attackTime++;

    // 更新激光攻击
    updateLaserAttack();
}

LivingEntity* GuardianAttackGoal::selectTarget() const {
    if (m_guardian == nullptr || m_guardian->world() == nullptr) {
        return nullptr;
    }

    // MC 1.16.5: 守卫者攻击玩家和鱿鱼
    // 简化实现：先搜索玩家
    // TODO: 实现搜索最近实体的逻辑
    // 当前返回 nullptr，等待目标选择器系统完善
    return nullptr;
}

bool GuardianAttackGoal::isTargetValid(LivingEntity* target) const {
    if (target == nullptr) {
        return false;
    }

    // 检查目标是否存活
    if (!target->isAlive()) {
        return false;
    }

    // 检查目标是否在同一世界
    if (target->world() != m_guardian->world()) {
        return false;
    }

    return true;
}

void GuardianAttackGoal::updateLaserAttack() {
    if (m_guardian == nullptr || m_target == nullptr) {
        return;
    }

    // 计算距离
    f64 distSq = m_guardian->distanceSqTo(*m_target);
    f32 attackRangeSqr = ATTACK_RANGE * ATTACK_RANGE;

    // 更新充能时间
    if (m_isCharging) {
        m_guardian->setLaserChargeTime(m_attackTime);

        // 充能完成
        if (m_attackTime >= CHARGE_DURATION) {
            // 检查目标是否仍在范围内
            if (distSq <= static_cast<f64>(attackRangeSqr)) {
                // 发射激光
                performLaserAttack(m_target);
            }

            // 重置充能状态
            m_isCharging = false;
            m_guardian->setLaserCharging(false);
            m_attackTime = 0;
        }
    } else {
        // 冷却阶段
        if (m_attackTime >= COOLDOWN_DURATION) {
            // 重新开始充能
            m_isCharging = true;
            m_guardian->setLaserCharging(true);
            m_guardian->setLaserChargeTime(0);
            m_attackTime = 0;
        }
    }
}

void GuardianAttackGoal::performLaserAttack(LivingEntity* target) {
    if (target == nullptr || m_guardian == nullptr) {
        return;
    }

    // 造成魔法伤害
    // MC 1.16.5: DamageSource.causeIndirectMagicDamage
    auto source = DamageSources::magic();
    target->hurt(source, LASER_DAMAGE);
}

} // namespace mc::entity::ai::goal
