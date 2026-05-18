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

#include "TargetGoals.hpp"
#include "../../../../../util/AxisAlignedBB.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/passive/basic/ChickenEntity.hpp"
#include "../../../../entities/passive/special/TurtleEntity.hpp"
#include "../../../../entities/passive/tamable/TameableEntity.hpp"
#include "../../../../entities/passive/special/FoxEntity.hpp"
#include "../../../../entities/passive/golem/IronGolemEntity.hpp"
#include "../../../../entities/monster/arthropod/EndermiteEntity.hpp"
#include "../../../../entities/monster/end/EndermanEntity.hpp"
#include "../../../../entities/monster/nether/NetherEntities.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "../../../../interfaces/IAngerable.hpp"
#include "../../../controller/LookController.hpp"
#include <cmath>
#include <limits>
#include <type_traits>

namespace mc::entity::ai::goal {

// ==================== TargetGoal ====================

TargetGoal::TargetGoal(MobEntity* mob, bool checkSight)
    : m_mob(mob)
    , m_checkSight(checkSight)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Target});
}

bool TargetGoal::shouldContinueExecuting()
{
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

void TargetGoal::startExecuting()
{
    if (m_mob && m_target) {
        m_mob->setAttackTarget(m_target);
    }
    m_unseenTicks = 0;
}

void TargetGoal::resetTask()
{
    m_target = nullptr;
    m_unseenTicks = 0;
    if (m_mob) {
        m_mob->setAttackTarget(nullptr);
    }
}

bool TargetGoal::isSuitableTarget(LivingEntity* target) const
{
    if (!target || !target->isAlive()) {
        return false;
    }

    // MC 1.16.5: 检查目标是否可以被攻击
    // 不能攻击自己
    if (target == m_mob) {
        return false;
    }

    // MC 1.16.5: 如果目标是玩家，检查游戏模式
    // 创造模式和观察者模式的玩家不能被作为目标
    Player* targetPlayer = dynamic_cast<Player*>(target);
    if (targetPlayer != nullptr) {
        if (targetPlayer->isCreative() || targetPlayer->isSpectator()) {
            return false;
        }
    }

    // MC 1.16.5: 检查团队关系
    // 未来工作：需要实现 Scoreboard 和 Team 系统
    // if (m_mob->isOnSameTeam(target)) {
    //     return false;
    // }

    return true;
}

bool TargetGoal::checkSight() const
{
    if (!m_mob || !m_target) return false;
    return m_mob->canSee(*m_target);
}

// ==================== NearestAttackableTargetGoal ====================

template <typename T>
NearestAttackableTargetGoal<T>::NearestAttackableTargetGoal(MobEntity* mob, bool checkSight, i32 chance)
    : TargetGoal(mob, checkSight)
    , m_chance(chance)
    , m_predicate(nullptr)
{
    static_assert(std::is_base_of<LivingEntity, T>::value,
        "NearestAttackableTargetGoal<T> requires T to be derived from LivingEntity");
}

template <typename T>
NearestAttackableTargetGoal<T>::NearestAttackableTargetGoal(
    MobEntity* mob, bool checkSight, i32 chance, TargetPredicate predicate)
    : TargetGoal(mob, checkSight)
    , m_chance(chance)
    , m_predicate(std::move(predicate))
{
    static_assert(std::is_base_of<LivingEntity, T>::value,
        "NearestAttackableTargetGoal<T> requires T to be derived from LivingEntity");
}

template <typename T>
bool NearestAttackableTargetGoal<T>::shouldExecute()
{
    if (!m_mob) return false;

    // MC 1.16.5: 概率检查
    if (m_chance > 0) {
        math::Random rng = m_mob->getRandom();
        if (rng.nextInt(m_chance) != 0) {
            return false;
        }
    }

    IWorld* world = m_mob->world();
    if (!world) return false;

    // MC 1.16.5: 使用跟踪范围作为搜索范围
    // 参考 Entity.getAttributeValue(Attributes.FOLLOW_RANGE)
    f64 followRange = m_mob->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
    f32 searchRange = static_cast<f32>(followRange);

    // 使用EntityUtils查找最近的目标
    T* nearestTarget = EntityUtils::findClosestEntity<T>(world,
        m_mob->position(),
        searchRange,
        m_mob, // 排除自己
        [this](T* candidate) {
            // 转换为LivingEntity进行目标检查
            LivingEntity* livingTarget = static_cast<LivingEntity*>(candidate);
            // 检查是否适合作为目标
            if (!isSuitableTarget(livingTarget)) {
                return false;
            }
            // 检查视线（如果需要）
            if (m_checkSight && !m_mob->canSee(*candidate)) {
                return false;
            }
            // 如果有自定义谓词，应用它
            if (m_predicate && !m_predicate(livingTarget)) {
                return false;
            }
            return true;
        });

    if (nearestTarget) {
        m_targetEntity = nearestTarget;
        m_target = static_cast<LivingEntity*>(nearestTarget);
        return true;
    }

    return false;
}

template <typename T>
void NearestAttackableTargetGoal<T>::startExecuting()
{
    TargetGoal::startExecuting();
}

// 显式实例化模板类
// T必须是LivingEntity的子类
template class NearestAttackableTargetGoal<LivingEntity>;
template class NearestAttackableTargetGoal<MobEntity>;
template class NearestAttackableTargetGoal<Player>;
template class NearestAttackableTargetGoal<ChickenEntity>;
template class NearestAttackableTargetGoal<TurtleEntity>;
template class NearestAttackableTargetGoal<FoxEntity>;
template class NearestAttackableTargetGoal<IronGolemEntity>;
template class NearestAttackableTargetGoal<AbstractPiglinEntity>;
template class NearestAttackableTargetGoal<entity::VillagerEntity>;
template class NearestAttackableTargetGoal<entity::AbstractVillagerEntity>;
template class NearestAttackableTargetGoal<EndermiteEntity>;

// ==================== HurtByTargetGoal ====================

HurtByTargetGoal::HurtByTargetGoal(MobEntity* mob, bool alertAllies)
    : TargetGoal(mob, true)
    , m_alertAllies(alertAllies)
{}

bool HurtByTargetGoal::shouldExecute()
{
    if (!m_mob) return false;

    // MC 1.16.5: 从LivingEntity获取最近攻击者
    LivingEntity* attacker = m_mob->getLastHurtBy();
    if (!attacker || !attacker->isAlive()) {
        return false;
    }

    // 检查时间戳，避免重复设置
    i32 timestamp = m_mob->lastHurtByTimestamp();
    if (timestamp == m_timestamp) {
        return false;
    }

    // 检查是否适合作为目标
    if (!isSuitableTarget(attacker)) {
        return false;
    }

    m_target = attacker;
    m_timestamp = timestamp;
    return true;
}

void HurtByTargetGoal::startExecuting()
{
    TargetGoal::startExecuting();

    // MC 1.16.5: 警醒盟友
    if (m_alertAllies && m_mob && m_target) {
        IWorld* world = m_mob->world();
        if (!world) return;

        // 获取实体的碰撞箱并扩展
        AxisAlignedBB alertBox = m_mob->boundingBox().expand(16.0, 4.0, 16.0);

        // 查找附近的同类型实体
        auto nearbyEntities = world->getEntitiesInAABB(alertBox, m_mob);
        for (Entity* entity : nearbyEntities) {
            // 只警醒同类型的生物
            MobEntity* ally = dynamic_cast<MobEntity*>(entity);
            if (!ally) continue;

            // 不能警醒目标本身
            if (ally == m_target) continue;

            // MC 1.16.5: 检查是否是同类型
            // 使用 typeId 比较
            if (ally->typeId() == m_mob->typeId()) {
                ally->setAttackTarget(m_target);
            }
        }
    }
}

void HurtByTargetGoal::resetTask()
{
    TargetGoal::resetTask();
    m_timestamp = 0;
}

// ==================== OwnerHurtByTargetGoal ====================

OwnerHurtByTargetGoal::OwnerHurtByTargetGoal(MobEntity* mob)
    : TargetGoal(mob, true)
{}

bool OwnerHurtByTargetGoal::shouldExecute()
{
    if (!m_mob) return false;

    // 检查是否是驯服动物
    TameableEntity* tameable = dynamic_cast<TameableEntity*>(m_mob);
    if (!tameable) return false;

    // 检查是否已驯服
    if (!tameable->isTamed()) return false;

    // 检查是否坐下（坐下的狼不攻击）
    if (tameable->isSitting()) return false;

    // 获取主人
    Player* owner = tameable->getOwner();
    if (!owner) return false;

    // MC 1.16.5: 检查主人是否有攻击者
    LivingEntity* attacker = owner->getLastHurtBy();
    if (!attacker || !attacker->isAlive()) return false;

    // 不能攻击自己或主人
    if (attacker == m_mob || attacker == owner) return false;

    // 检查是否适合作为目标
    if (!isSuitableTarget(attacker)) return false;

    // MC 1.16.5: 使用 shouldAttackEntity 检查（狼不应该攻击苦力怕、恶魂、其他驯服动物等）
    // 这里简化处理，直接设置目标
    m_target = attacker;
    return true;
}

void OwnerHurtByTargetGoal::startExecuting()
{
    TargetGoal::startExecuting();
}

// ==================== OwnerHurtTargetGoal ====================

OwnerHurtTargetGoal::OwnerHurtTargetGoal(MobEntity* mob)
    : TargetGoal(mob, true)
{}

bool OwnerHurtTargetGoal::shouldExecute()
{
    if (!m_mob) return false;

    // 检查是否是驯服动物
    TameableEntity* tameable = dynamic_cast<TameableEntity*>(m_mob);
    if (!tameable) return false;

    // 检查是否已驯服
    if (!tameable->isTamed()) return false;

    // 检查是否坐下（坐下的狼不攻击）
    if (tameable->isSitting()) return false;

    // 获取主人
    Player* owner = tameable->getOwner();
    if (!owner) return false;

    // MC 1.16.5: 检查主人正在攻击的目标
    LivingEntity* target = owner->getLastHurtTarget();
    if (!target || !target->isAlive()) return false;

    // 不能攻击自己或主人
    if (target == m_mob || target == owner) return false;

    // 检查是否适合作为目标
    if (!isSuitableTarget(target)) return false;

    // MC 1.16.5: 使用 shouldAttackEntity 检查
    m_target = target;
    return true;
}

void OwnerHurtTargetGoal::startExecuting()
{
    TargetGoal::startExecuting();
}

// ==================== NonTamedTargetGoal ====================

template <typename T>
NonTamedTargetGoal<T>::NonTamedTargetGoal(MobEntity* mob, bool checkSight)
    : TargetGoal(mob, checkSight)
    , m_predicate(nullptr)
{
    static_assert(
        std::is_base_of<LivingEntity, T>::value, "NonTamedTargetGoal<T> requires T to be derived from LivingEntity");
}

template <typename T>
NonTamedTargetGoal<T>::NonTamedTargetGoal(MobEntity* mob, bool checkSight, TargetPredicate predicate)
    : TargetGoal(mob, checkSight)
    , m_predicate(std::move(predicate))
{
    static_assert(
        std::is_base_of<LivingEntity, T>::value, "NonTamedTargetGoal<T> requires T to be derived from LivingEntity");
}

template <typename T>
bool NonTamedTargetGoal<T>::shouldExecute()
{
    if (!m_mob) return false;

    // 检查是否是驯服动物
    TameableEntity* tameable = dynamic_cast<TameableEntity*>(m_mob);
    if (!tameable) return false;

    // 已驯服的动物不执行此目标
    if (tameable->isTamed()) return false;

    IWorld* world = m_mob->world();
    if (!world) return false;

    // MC 1.16.5: 使用跟踪范围作为搜索范围
    f64 followRange = m_mob->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
    f32 searchRange = static_cast<f32>(followRange);

    // 使用EntityUtils查找最近的目标
    T* nearestTarget = EntityUtils::findClosestEntity<T>(world,
        m_mob->position(),
        searchRange,
        m_mob, // 排除自己
        [this](T* candidate) {
            // 转换为LivingEntity进行目标检查
            LivingEntity* livingTarget = static_cast<LivingEntity*>(candidate);
            // 检查是否适合作为目标
            if (!isSuitableTarget(livingTarget)) {
                return false;
            }
            // 检查视线（如果需要）
            if (m_checkSight && !m_mob->canSee(*candidate)) {
                return false;
            }
            // 如果有自定义谓词，应用它
            if (m_predicate && !m_predicate(livingTarget)) {
                return false;
            }
            return true;
        });

    if (nearestTarget) {
        m_targetEntity = nearestTarget;
        m_target = static_cast<LivingEntity*>(nearestTarget);
        return true;
    }

    return false;
}

template <typename T>
void NonTamedTargetGoal<T>::startExecuting()
{
    TargetGoal::startExecuting();
}

// ==================== ResetAngerGoal ====================

template <typename T>
ResetAngerGoal<T>::ResetAngerGoal(T* mob, bool alertOthers)
    : m_mob(mob)
    , m_alertOthers(alertOthers)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Target});
}

template <typename T>
bool ResetAngerGoal<T>::shouldExecute()
{
    if (!m_mob) return false;

    // MC 1.16.5: 检查 UNIVERSAL_ANGER 游戏规则和复仇条件
    // 由于当前项目没有实现 UNIVERSAL_ANGER 游戏规则，简化实现
    // 原版逻辑：return this.field_241383_a_.world.getGameRules().getBoolean(GameRules.UNIVERSAL_ANGER) && this.shouldGetRevengeOnPlayer();
    // 当前简化：直接检查是否应该复仇
    return shouldGetRevengeOnPlayer();
}

template <typename T>
bool ResetAngerGoal<T>::shouldGetRevengeOnPlayer() const
{
    if (!m_mob) return false;

    // MC 1.16.5: 检查复仇目标是否是玩家，并且复仇计时器更新
    LivingEntity* revengeTarget = m_mob->getRevengeTarget();
    if (!revengeTarget) return false;

    // 检查复仇目标是否是玩家
    Player* player = dynamic_cast<Player*>(revengeTarget);
    if (!player) return false;

    // 检查复仇计时器是否更新（避免重复触发）
    i32 revengeTimer = m_mob->getRevengeTimer();
    return revengeTimer > m_revengeTimer;
}

template <typename T>
std::vector<T*> ResetAngerGoal<T>::getNearbySameTypeEntities() const
{
    std::vector<T*> result;
    if (!m_mob) return result;

    IWorld* world = m_mob->world();
    if (!world) return result;

    // MC 1.16.5: 获取 FOLLOW_RANGE 属性作为搜索范围
    f64 followRange = m_mob->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);

    // 扩展碰撞箱
    AxisAlignedBB searchBox = m_mob->boundingBox().expand(followRange, 10.0, followRange);

    // 获取范围内所有实体
    auto entities = world->getEntitiesInAABB(searchBox, nullptr);
    for (Entity* entity : entities) {
        // 跳过自己
        if (entity == m_mob) continue;

        // 检查是否是同类型
        T* sameType = dynamic_cast<T*>(entity);
        if (sameType) {
            result.push_back(sameType);
        }
    }

    return result;
}

template <typename T>
void ResetAngerGoal<T>::startExecuting()
{
    if (!m_mob) return;

    // 更新复仇计时器
    m_revengeTimer = m_mob->getRevengeTimer();

    // MC 1.16.5: func_241355_J__() 是 makeAngry() 或类似方法
    // 重置愤怒时间并开始愤怒
    m_mob->setAngry(false);
    m_mob->setAngerTime(0);
    m_mob->setAttackTarget(nullptr);

    // 如果需要警醒其他同类实体
    if (m_alertOthers) {
        auto nearbyEntities = getNearbySameTypeEntities();
        for (T* other : nearbyEntities) {
            // 重置其他实体的愤怒
            other->setAngry(false);
            other->setAngerTime(0);
            other->setAttackTarget(nullptr);
        }
    }

    Goal::startExecuting();
}

// ==================== 显式实例化模板类 ====================

// NonTamedTargetGoal（NearestAttackableTargetGoal 已在文件中间实例化）
template class NonTamedTargetGoal<LivingEntity>;
template class NonTamedTargetGoal<MobEntity>;
template class NonTamedTargetGoal<TurtleEntity>;

// ResetAngerGoal 用于实现了 IAngerable 接口的 MobEntity 子类
template class ResetAngerGoal<EndermanEntity>;

} // namespace mc::entity::ai::goal
