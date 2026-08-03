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
#include "../../../../core/EntityUtils.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/monster/MonsterEntity.hpp"
#include "../../../../entities/passive/golem/CopperGolemEntity.hpp"
#include "../../../../entities/passive/golem/IronGolemEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "../../../../tag/EntityTypeTags.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/tag/EntityTypeTag.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"

#include <algorithm>
#include <limits>

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
        math::Random& rng = m_golem->getRandom();
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

// ==================== OfferFlowerGoal ====================

OfferFlowerGoal::OfferFlowerGoal(IronGolemEntity* ironGolem)
    : m_ironGolem(ironGolem)
{
    MC_ASSERT_RELEASE(ironGolem != nullptr);
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool OfferFlowerGoal::shouldExecute()
{
    IWorld* world = m_ironGolem->world();
    if (!world) return false;

    // 只在室外明亮时执行（与 MC 1.21.11 OfferFlowerGoal.canUse() 中
    // level().isBrightOutside() 一致；雷暴天也会让天空变暗，从而禁止赠花）
    if (!world->isBrightOutside()) {
        return false;
    }

    // 概率检查 1/8000
    math::Random& rng = m_ironGolem->getRandom();
    if (rng.nextInt(CHANCE) != 0) {
        return false;
    }

    // 在铁傀儡附近的 AABB 内查找最近的可赠花候选实体
    LivingEntity* candidate = _findNearestCandidate();
    if (candidate) {
        m_target = candidate;
        return true;
    }

    return false;
}

bool OfferFlowerGoal::shouldContinueExecuting()
{
    return m_tick > 0;
}

void OfferFlowerGoal::startExecuting()
{
    m_tick = OFFER_TICKS;
    m_ironGolem->setHoldingRose(true);
}

void OfferFlowerGoal::resetTask()
{
    // 尝试将罂粟花赠予铜傀儡（仅在计时器自然结束时触发）
    _tryGiftFlowerToCopperGolem();

    m_ironGolem->setHoldingRose(false);
    m_target = nullptr;
}

void OfferFlowerGoal::tick()
{
    if (m_target) {
        auto* lookController = m_ironGolem->lookController();
        if (lookController) {
            lookController->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
        }
    }

    // 递减赠花计时器
    m_tick--;
}

AxisAlignedBB OfferFlowerGoal::_getGolemSearchBox() const
{
    // 对应 MC 1.21.11 OfferFlowerGoal.getGolemBoundingBox()：
    //   golem.getBoundingBox().inflate(6.0, 2.0, 6.0)
    return m_ironGolem->boundingBox().expand(SEARCH_EXPAND_XZ, SEARCH_EXPAND_Y, SEARCH_EXPAND_XZ);
}

LivingEntity* OfferFlowerGoal::_findNearestCandidate() const
{
    IWorld* world = m_ironGolem->world();
    if (!world) return nullptr;

    // 当标签系统尚未初始化（如单元测试环境）时，直接返回 nullptr，
    // 避免访问空数据。运行时由 MinecraftServer::initializeRegistries() 完成初始化。
    if (!EntityTypeTags::isInitialized()) {
        return nullptr;
    }

    const EntityTypeTag& candidateTag = EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT();
    const AxisAlignedBB searchBox = _getGolemSearchBox();

    // 查找搜索 AABB 内所有实体（排除铁傀儡自身）
    auto entities = world->getEntitiesInAABB(searchBox, m_ironGolem);

    LivingEntity* nearest = nullptr;
    f32 bestDistSq = std::numeric_limits<f32>::max();
    const Vector3 golemPos = m_ironGolem->position();

    for (Entity* entity : entities) {
        if (!entity || !entity->isAlive()) {
            continue;
        }

        // 仅考虑 LivingEntity（村民和铜傀儡都是 LivingEntity 子类）
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (!living) {
            continue;
        }

        // 通过 EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT 标签过滤候选实体。
        // 与 MC Java OfferFlowerGoal.canUse() 中
        // getNearestEntity(EntityTypeTags.CANDIDATE_FOR_IRON_GOLEM_GIFT, ...) 一致。
        if (!candidateTag.contains(living->getTypeId())) {
            continue;
        }

        const f32 distSq = golemPos.distanceSquared(living->position());
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            nearest = living;
        }
    }

    return nearest;
}

void OfferFlowerGoal::_tryGiftFlowerToCopperGolem()
{
    // 仅在计时器自然结束（m_tick 递减到 0）时赠花；
    // 若因被抢占中断（m_tick > 0），则不赠予。
    if (m_tick != 0) {
        return;
    }

    if (!m_target) {
        return;
    }

    // 目标必须是 MobEntity（铜傀儡是 MobEntity 子类，可访问装备槽 API）
    MobEntity* mob = dynamic_cast<MobEntity*>(m_target);
    if (!mob) {
        return;
    }

    // 当标签系统尚未初始化时无法判断是否可接受赠花，直接返回。
    if (!EntityTypeTags::isInitialized()) {
        return;
    }

    // 检查目标是否属于 ACCEPTS_IRON_GOLEM_GIFT 标签（铜傀儡）
    if (!EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT().contains(mob->getTypeId())) {
        return;
    }

    // 铜傀儡的天线槽必须为空（尚未持有花朵）
    if (!mob->getEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA).isEmpty()) {
        return;
    }

    // 铁傀儡搜索 AABB 必须与目标碰撞盒相交（对应 MC stop() 中的相交检查）
    if (!_getGolemSearchBox().intersects(mob->boundingBox())) {
        return;
    }

    // 装备罂粟花到天线槽，并标记为保整掉落，
    // 后续铜傀儡转雕像时由 dropPreservedEquipment() 自动掉落。
    mob->setEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA, Items::POPPY->getDefaultInstance());
    mob->setGuaranteedDrop(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA);
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
        math::Random& rng = m_golem->getRandom();
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
            const entity::EntityType* candidateType = candidate->entityType();
            if (candidateType == nullptr || !m_golem->canAttackType(*candidateType)) {
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
