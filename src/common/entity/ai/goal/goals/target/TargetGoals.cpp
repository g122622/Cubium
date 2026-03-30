#include "TargetGoals.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../entities/passive/tamable/TameableEntity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ==================== TargetGoal ====================

TargetGoal::TargetGoal(MobEntity* mob, bool checkSight)
    : m_mob(mob)
    , m_checkSight(checkSight)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Target});
}

bool TargetGoal::shouldContinueExecuting() {
    if (!m_mob || !m_target) return false;

    // 检查目标是否存活
    if (!m_target->isAlive()) {
        return false;
    }

    // 检查视线
    if (m_checkSight) {
        if (!checkSight()) {
            m_unseenTicks++;
            if (m_unseenTicks > MAX_UNSEEN_TICKS) {
                return false;
            }
        } else {
            m_unseenTicks = 0;
        }
    }

    return true;
}

void TargetGoal::startExecuting() {
    if (m_mob && m_target) {
        m_mob->setAttackTarget(m_target);
    }
    m_unseenTicks = 0;
}

void TargetGoal::resetTask() {
    m_target = nullptr;
    m_unseenTicks = 0;
    if (m_mob) {
        m_mob->setAttackTarget(nullptr);
    }
}

bool TargetGoal::isSuitableTarget(LivingEntity* target) const {
    if (!target || !target->isAlive()) {
        return false;
    }

    // 检查目标是否可以被攻击
    // TODO: 检查团队关系、游戏规则等

    return true;
}

bool TargetGoal::checkSight() const {
    if (!m_mob || !m_target) return false;
    return m_mob->canSee(*m_target);
}

// ==================== NearestAttackableTargetGoal ====================

// 显式实例化模板类
template class NearestAttackableTargetGoal<LivingEntity>;
template class NearestAttackableTargetGoal<class Player>;
template class NearestAttackableTargetGoal<MobEntity>;

template<typename T>
NearestAttackableTargetGoal<T>::NearestAttackableTargetGoal(MobEntity* mob, bool checkSight, i32 chance)
    : TargetGoal(mob, checkSight)
    , m_chance(chance)
{
}

template<typename T>
bool NearestAttackableTargetGoal<T>::shouldExecute() {
    if (!m_mob) return false;

    // 概率检查
    if (m_chance > 0) {
        math::Random rng = m_mob->getRandom();
        if (rng.nextInt(m_chance) != 0) {
            return false;
        }
    }

    // 寻找最近的目标
    // TODO: 使用世界接口搜索附近实体
    // IWorld* world = m_mob->world();
    // if (!world) return false;
    //
    // AABB searchBox = m_mob->getBoundingBox().grow(16.0, 16.0, 16.0);
    // std::vector<T*> targets = world->getEntitiesWithinAABB<T>(searchBox);
    //
    // T* nearestTarget = nullptr;
    // f64 nearestDistSq = std::numeric_limits<f64>::max();
    //
    // for (T* target : targets) {
    //     if (!isSuitableTarget(target)) continue;
    //     if (m_checkSight && !m_mob->canSee(*target)) continue;
    //
    //     f64 distSq = m_mob->distanceSqTo(*target);
    //     if (distSq < nearestDistSq) {
    //         nearestDistSq = distSq;
    //         nearestTarget = target;
    //     }
    // }
    //
    // if (nearestTarget) {
    //     m_targetEntity = nearestTarget;
    //     m_target = nearestTarget;
    //     return true;
    // }

    return false;
}

template<typename T>
void NearestAttackableTargetGoal<T>::startExecuting() {
    TargetGoal::startExecuting();
}

// ==================== HurtByTargetGoal ====================

HurtByTargetGoal::HurtByTargetGoal(MobEntity* mob, bool alertAllies)
    : TargetGoal(mob, true)
    , m_alertAllies(alertAllies)
{
}

bool HurtByTargetGoal::shouldExecute() {
    if (!m_mob) return false;

    // 检查是否有最近受伤来源
    // TODO: 从LivingEntity获取最近攻击者
    // LivingEntity* attacker = m_mob->getLastHurtBy();
    // if (!attacker || !attacker->isAlive()) {
    //     return false;
    // }
    //
    // // 检查时间戳，避免重复设置
    // i32 timestamp = m_mob->getLastHurtByTimestamp();
    // if (timestamp == m_timestamp) {
    //     return false;
    // }
    //
    // m_target = attacker;
    // m_timestamp = timestamp;
    // return isSuitableTarget(attacker);

    return false;
}

void HurtByTargetGoal::startExecuting() {
    TargetGoal::startExecuting();

    // 警醒盟友
    if (m_alertAllies) {
        // TODO: 通知附近的盟友
        // IWorld* world = m_mob->world();
        // if (world) {
        //     AABB alertBox = m_mob->getBoundingBox().grow(16.0, 4.0, 16.0);
        //     std::vector<MobEntity*> allies = world->getEntitiesWithinAABB<MobEntity>(alertBox);
        //     for (MobEntity* ally : allies) {
        //         if (ally != m_mob && ally->isAlliedWith(m_mob)) {
        //             ally->setAttackTarget(m_target);
        //         }
        //     }
        // }
    }
}

void HurtByTargetGoal::resetTask() {
    TargetGoal::resetTask();
    m_timestamp = 0;
}

// ==================== OwnerHurtByTargetGoal ====================

OwnerHurtByTargetGoal::OwnerHurtByTargetGoal(MobEntity* mob)
    : TargetGoal(mob, true)
{
}

bool OwnerHurtByTargetGoal::shouldExecute() {
    if (!m_mob) return false;

    // 检查是否是驯服动物
    TameableEntity* tameable = dynamic_cast<TameableEntity*>(m_mob);
    if (!tameable) return false;

    // 检查是否已驯服
    if (!tameable->isTamed()) return false;

    // 获取主人
    // TODO: 从TameableEntity获取主人
    // Player* owner = tameable->getOwner();
    // if (!owner) return false;
    //
    // // 检查主人是否有攻击者
    // LivingEntity* attacker = owner->getLastHurtBy();
    // if (!attacker || !attacker->isAlive()) return false;
    //
    // // 不能攻击主人自己
    // if (attacker == owner) return false;
    //
    // m_target = attacker;
    // return isSuitableTarget(attacker);

    return false;
}

void OwnerHurtByTargetGoal::startExecuting() {
    TargetGoal::startExecuting();
}

// ==================== OwnerHurtTargetGoal ====================

OwnerHurtTargetGoal::OwnerHurtTargetGoal(MobEntity* mob)
    : TargetGoal(mob, true)
{
}

bool OwnerHurtTargetGoal::shouldExecute() {
    if (!m_mob) return false;

    // 检查是否是驯服动物
    TameableEntity* tameable = dynamic_cast<TameableEntity*>(m_mob);
    if (!tameable) return false;

    // 检查是否已驯服
    if (!tameable->isTamed()) return false;

    // 获取主人
    // TODO: 从TameableEntity获取主人
    // Player* owner = tameable->getOwner();
    // if (!owner) return false;
    //
    // // 检查主人正在攻击的目标
    // LivingEntity* target = owner->getLastHurtTarget();
    // if (!target || !target->isAlive()) return false;
    //
    // // 不能攻击主人自己
    // if (target == owner) return false;
    //
    // m_target = target;
    // return isSuitableTarget(target);

    return false;
}

void OwnerHurtTargetGoal::startExecuting() {
    TargetGoal::startExecuting();
}

} // namespace mc::entity::ai::goal
