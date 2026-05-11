#include "GuardianAttackGoal.hpp"
#include "../../../../entities/monster/ocean/GuardianEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../attribute/Attributes.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../damage/DamageSource.hpp"
#include "../../../../../util/assert/AssertAll.hpp"
#include "../../../../../core/Types.hpp"

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

    // MC 1.16.5 GuardianEntity.AttackGoal.shouldExecute():
    // 优先使用 targetSelector 设置的攻击目标
    // LivingEntity livingentity = this.guardian.getAttackTarget();
    // return livingentity != null && livingentity.isAlive();

    LivingEntity* target = m_guardian->attackTarget();

    // 如果没有攻击目标，尝试自己搜索
    if (target == nullptr) {
        target = selectTarget();
    }

    if (target == nullptr) {
        return false;
    }

    // 检查目标是否存活
    if (!target->isAlive()) {
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
    // TODO: 实现 broadcastEntityStatus 方法
    // if (m_guardian->world()) {
    //     m_guardian->world()->broadcastEntityStatus(
    //         m_guardian->id(),
    //         static_cast<u8>(network::EntityStatusPacket::Status::GuardianAttack));
    // }
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
    // 参考 GuardianEntity.registerGoals() 中的 TargetPredicate
    // 目标类型筛选: 只攻击 Player 或 Squid
    // 距离筛选: 必须距离大于 3 格（距离平方 > 9.0）
    //
    // 注意：主要的目标选择逻辑应该由 targetSelector 中的 NearestAttackableTargetGoal 提供
    // 此方法作为备用逻辑，直接搜索最近的有效目标

    IWorld* world = m_guardian->world();

    // 获取跟随范围（搜索范围）
    f64 followRange = m_guardian->getAttributeValue(
        entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
    f32 searchRange = static_cast<f32>(followRange);

    // 使用 EntityUtils 搜索最近的 LivingEntity
    LivingEntity* nearestTarget = EntityUtils::findClosestEntity<LivingEntity>(
        world,
        m_guardian->position(),
        searchRange,
        m_guardian,  // 排除自己
        [this](LivingEntity* candidate) {
            // 1. 检查目标是否存活
            if (!candidate || !candidate->isAlive()) {
                return false;
            }

            // 2. 类型筛选: 只攻击玩家或鱿鱼
            auto type = candidate->legacyType();
            bool isPlayer = (type == LegacyEntityType::Player);
            bool isSquid = (type == LegacyEntityType::Squid);
            if (!isPlayer && !isSquid) {
                return false;
            }

            // 3. 玩家特殊检查: 创造模式和观察者模式不能被攻击
            if (isPlayer) {
                Player* player = dynamic_cast<Player*>(candidate);
                if (player != nullptr && (player->isCreative() || player->isSpectator())) {
                    return false;
                }
            }

            // 4. 距离筛选: 必须距离 > 3 格
            // 参考 MC 1.16.5 GuardianEntity.TargetPredicate.test()
            f64 distSq = m_guardian->distanceSqTo(*candidate);
            if (distSq <= 9.0) {  // 3.0 * 3.0 = 9.0
                return false;
            }

            // 5. 视线检查
            if (!m_guardian->canSee(*candidate)) {
                return false;
            }

            return true;
        }
    );

    return nearestTarget;
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
