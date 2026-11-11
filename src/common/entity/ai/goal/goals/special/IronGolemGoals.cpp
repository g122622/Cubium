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
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/tag/EntityTypeTag.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

#include <algorithm>
#include <limits>

namespace mc::entity::ai::goal {

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
    if (!world) {
        return false;
    }

    // 只在室外明亮时执行（与 MC 1.21.11 OfferFlowerGoal.canUse() 中
    // level().isBrightOutside() 一致；雷暴天也会让天空变暗，从而禁止赠花）
    if (!world->isBrightOutside()) {
        return false;
    }

    // 概率检查 1/8000
    math::IRandom& rng = m_ironGolem->getRandom();
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
    if (m_target.get()) {
        // 移除或析构的引用会自动失效；此处同时处理尚未移除的死亡目标。
        if (!m_target->isAlive()) {
            m_target = nullptr;
        } else {
            auto* lookController = m_ironGolem->lookController();
            if (lookController) {
                lookController->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);
            }
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
    if (!world) {
        return nullptr;
    }

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
    MobEntity* mob = dynamic_cast<MobEntity*>(m_target.get());
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
    if (!world) {
        return false;
    }

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
    if (m_villageAggressor.get()) {
        m_golem->setAttackTarget(m_villageAggressor.get());
    }
}

void DefendVillageTargetGoal::resetTask()
{
    TargetGoal::resetTask();
    m_villageAggressor = nullptr;
}

} // namespace mc::entity::ai::goal
