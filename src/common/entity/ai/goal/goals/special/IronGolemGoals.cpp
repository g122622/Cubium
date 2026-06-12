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

#include "IronGolemGoals.hpp"
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../core/EntityTypeIdNumber.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/monster/MonsterEntity.hpp"
#include "../../../../entities/passive/golem/IronGolemEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"

namespace mc::entity::ai::goal {

// ==================== IronGolemAttackGoal ====================

IronGolemAttackGoal::IronGolemAttackGoal(IronGolemEntity* golem, f64 speed)
    : m_golem(golem)
    , m_speed(speed)
{
    MC_ASSERT_RELEASE(golem != nullptr);
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool IronGolemAttackGoal::shouldExecute()
{
    // 游戏时间节流 - 每20 tick检查一次
    u32 ticksExisted = m_golem->ticksExisted();
    if (ticksExisted - m_lastCheckTime < 20) {
        return false;
    }
    m_lastCheckTime = ticksExisted;

    // 获取攻击目标
    LivingEntity* target = m_golem->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    // 铁傀儡不攻击苦力怕，不攻击玩家创建者的逻辑在 IronGolemEntity.canAttack() 中处理
    m_attackTarget = target;

    // 尝试获取路径
    if (m_golem->navigator()) {
        if (m_golem->navigator()->moveTo(*target, 0)) {
            return true;
        }
    }

    // 如果路径失败，检查是否已经在攻击范围内
    f32 attackReachSq = getAttackReachSqr(target);
    f32 distSq = m_golem->distanceSqTo(*target);
    return distSq <= attackReachSq;
}

bool IronGolemAttackGoal::shouldContinueExecuting()
{
    if (!m_attackTarget) return false;

    // 检查目标是否存活
    if (!m_attackTarget->isAlive()) {
        return false;
    }

    // 玩家模式检查
    Player* player = dynamic_cast<Player*>(m_attackTarget);
    if (player && (player->isSpectator() || player->isCreative())) {
        return false;
    }

    // 检查距离
    f32 distSq = m_golem->distanceSqTo(*m_attackTarget);

    if (distSq > STOP_ATTACK_DISTANCE_SQ) {
        return false; // 目标太远，停止追踪
    }

    return true;
}

void IronGolemAttackGoal::startExecuting()
{
    m_attackCooldown = 0;
    m_pathRecalculateTimer = 0;
    m_targetX = 0.0;
    m_targetY = 0.0;
    m_targetZ = 0.0;

    m_golem->setAggroed(true);

    if (auto* nav = m_golem->navigator()) {
        if (m_attackTarget) {
            static_cast<void>(nav->moveTo(*m_attackTarget, m_speed));
        }
    }
}

void IronGolemAttackGoal::resetTask()
{
    m_attackTarget = nullptr;

    m_golem->clearNavigation();
    m_golem->setAggroed(false);
}

void IronGolemAttackGoal::tick()
{
    if (!m_attackTarget) return;

    // 使用 LookController 看向目标
    if (auto* lookCtrl = m_golem->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_attackTarget, 30.0f, 30.0f);
    }

    f64 distSq = m_golem->distanceSqTo(*m_attackTarget);

    // 路径重算逻辑
    m_pathRecalculateTimer = std::max(m_pathRecalculateTimer - 1, 0);

    bool shouldRecalcPath = false;

    // 检查是否需要重新计算路径
    if (m_golem->canSee(*m_attackTarget) && m_pathRecalculateTimer <= 0 &&
        ((m_targetX == 0.0 && m_targetY == 0.0 && m_targetZ == 0.0) ||
            m_attackTarget->distanceSqTo(m_targetX, m_targetY, m_targetZ) >= 1.0 ||
            m_golem->getRandom().nextFloat() < 0.05f)) {

        shouldRecalcPath = true;
    }

    if (shouldRecalcPath) {
        // 更新目标位置
        m_targetX = m_attackTarget->x();
        m_targetY = m_attackTarget->y();
        m_targetZ = m_attackTarget->z();

        // 随机重算间隔 (4-10)
        math::Random rng = m_golem->getRandom();
        m_pathRecalculateTimer = PATH_RECALC_BASE_MIN + rng.nextInt(PATH_RECALC_BASE_MAX - PATH_RECALC_BASE_MIN);

        // 根据距离调整重算间隔
        if (distSq > 1024.0) { // > 32格距离
            m_pathRecalculateTimer += 10;
        } else if (distSq > 256.0) { // > 16格距离
            m_pathRecalculateTimer += 5;
        }

        // 移动到目标
        if (m_golem->navigator()) {
            static_cast<void>(m_golem->navigator()->moveTo(*m_attackTarget, m_speed));
        }
    }

    // 减少攻击冷却
    m_attackCooldown = std::max(m_attackCooldown - 1, 0);

    // 检查是否可以攻击
    checkAndPerformAttack(m_attackTarget, distSq);
}

bool IronGolemAttackGoal::canAttack(LivingEntity* target) const
{
    if (!target) return false;

    // 检查是否在攻击范围内
    f32 attackReachSq = getAttackReachSqr(target);
    f32 distSq = m_golem->distanceSqTo(*target);

    return distSq <= attackReachSq;
}

void IronGolemAttackGoal::checkAndPerformAttack(LivingEntity* target, f64 distToEnemySqr)
{
    if (!target) return;

    f32 attackReachSq = getAttackReachSqr(target);

    if (distToEnemySqr <= static_cast<f64>(attackReachSq) && m_attackCooldown <= 0) {
        // 重置攻击冷却
        m_attackCooldown = ATTACK_COOLDOWN_TICKS;

        // 执行攻击
        attackTarget(target);
    }
}

void IronGolemAttackGoal::attackTarget(LivingEntity* target)
{
    if (!target) return;

    // 使用铁傀儡的统一攻击方法，包含伤害、击退、附魔效果和声音
    m_golem->attackEntityAsMob(*target);

    // 挥动手臂动画
    m_golem->swingArm();
}

f32 IronGolemAttackGoal::getAttackReachSqr(LivingEntity* target) const
{
    // 攻击范围：(this.attacker.getWidth() * 2.0F) * (this.attacker.getWidth() * 2.0F) + target.getWidth()
    f32 attackerWidth = m_golem->width();
    f32 targetWidth = target->width();
    f32 reachWidth = attackerWidth * 2.0f;
    return reachWidth * reachWidth + targetWidth;
}

// ==================== ShowVillagerFlowerGoal ====================

ShowVillagerFlowerGoal::ShowVillagerFlowerGoal(IronGolemEntity* ironGolem)
    : m_ironGolem(ironGolem)
{
    MC_ASSERT_RELEASE(ironGolem != nullptr);
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool ShowVillagerFlowerGoal::shouldExecute()
{
    IWorld* world = m_ironGolem->world();
    if (!world) return false;

    // 只在白天执行
    if (!world->isDaytime()) {
        return false;
    }

    // 概率检查 1/8000
    math::Random rng = m_ironGolem->getRandom();
    if (rng.nextInt(CHANCE) != 0) {
        return false;
    }

    // 在 6 格范围内搜索村民
    Vector3 pos = m_ironGolem->position();

    entity::VillagerEntity* nearestVillager = EntityUtils::findClosestEntity<entity::VillagerEntity>(
        world, pos, SEARCH_RANGE, m_ironGolem, [](entity::VillagerEntity* villager) -> bool {
            if (!villager) return false;
            return villager->isAlive();
        });

    if (nearestVillager) {
        m_villager = nearestVillager;
        return true;
    }

    return false;
}

bool ShowVillagerFlowerGoal::shouldContinueExecuting()
{
    return m_lookTime > 0;
}

void ShowVillagerFlowerGoal::startExecuting()
{
    m_lookTime = LOOK_DURATION;
    m_ironGolem->setHoldingRose(true);
}

void ShowVillagerFlowerGoal::resetTask()
{
    m_ironGolem->setHoldingRose(false);
    m_villager = nullptr;
}

void ShowVillagerFlowerGoal::tick()
{
    if (!m_villager) return;

    auto* lookController = m_ironGolem->lookController();
    if (lookController) {
        lookController->setLookPositionWithEntity(*m_villager, 30.0f, 30.0f);
    }

    // 递减看向时间
    m_lookTime--;
}

// ==================== DefendVillageTargetGoal ====================

DefendVillageTargetGoal::DefendVillageTargetGoal(IronGolemEntity* golem)
    : TargetGoal(golem, true)
    , m_golem(golem)
{
    MC_ASSERT_RELEASE(golem != nullptr);
}

bool DefendVillageTargetGoal::shouldExecute()
{
    IWorld* world = m_golem->world();
    if (!world) return false;

    // 搜索附近攻击村民的实体，找到最近的村民，然后找到攻击该村民的敌对生物
    // 首先找到附近的村民
    entity::VillagerEntity* nearestVillager = EntityUtils::findClosestEntity<entity::VillagerEntity>(
        world, m_golem->position(), 16.0f, nullptr, [](entity::VillagerEntity* villager) -> bool {
            return villager != nullptr && villager->isAlive();
        });

    if (!nearestVillager) {
        return false;
    }

    // 检查村民是否有攻击者
    LivingEntity* attacker = nearestVillager->getLastHurtBy();
    if (!attacker || !attacker->isAlive()) {
        return false;
    }

    // 检查攻击者是否适合作为目标
    // isSuitableTarget 已调用 canAttackType，会排除玩家创建的铁傀儡不攻击玩家等情况
    if (!isSuitableTarget(attacker)) {
        return false;
    }

    m_villageAggressor = attacker;
    m_target = attacker;
    return true;
}

void DefendVillageTargetGoal::startExecuting()
{
    TargetGoal::startExecuting();
    if (m_villageAggressor) {
        m_golem->setAttackTarget(m_villageAggressor);
    }
}

void DefendVillageTargetGoal::resetTask()
{
    TargetGoal::resetTask();
    m_villageAggressor = nullptr;
}

// ==================== IronGolemNearestAttackableTargetGoal ====================

IronGolemNearestAttackableTargetGoal::IronGolemNearestAttackableTargetGoal(IronGolemEntity* golem, i32 chance)
    : TargetGoal(golem, true)
    , m_golem(golem)
    , m_chance(chance)
{
    MC_ASSERT_RELEASE(golem != nullptr);
}

bool IronGolemNearestAttackableTargetGoal::shouldExecute()
{
    // 概率检查
    if (m_chance > 0) {
        math::Random rng = m_golem->getRandom();
        if (rng.nextInt(m_chance) != 0) {
            return false;
        }
    }

    IWorld* world = m_golem->world();
    if (!world) return false;

    // 搜索附近的敌对生物，但不包括苦力怕
    // canAttackType 检查排除苦力怕和玩家创建者
    MobEntity* nearestTarget = EntityUtils::findClosestEntity<MobEntity>(
        world, m_golem->position(), SEARCH_RANGE, m_golem, [this](MobEntity* candidate) -> bool {
            if (!candidate || !candidate->isAlive()) {
                return false;
            }

            // 只攻击敌对生物（MonsterEntity 类型）
            const MonsterEntity* monster = dynamic_cast<const MonsterEntity*>(candidate);
            if (!monster) {
                return false;
            }

            // 检查实体类型是否可攻击（canAttackType 排除苦力怕等）
            if (!m_golem->canAttackType(candidate->typeId())) {
                return false;
            }

            // 检查视线
            if (m_checkSight && !m_golem->canSee(*candidate)) {
                return false;
            }

            return true;
        });

    if (nearestTarget) {
        m_targetEntity = nearestTarget;
        m_target = nearestTarget;
        return true;
    }

    return false;
}

void IronGolemNearestAttackableTargetGoal::startExecuting()
{
    TargetGoal::startExecuting();
    if (m_targetEntity) {
        m_golem->setAttackTarget(m_targetEntity);
    }
}

} // namespace mc::entity::ai::goal
