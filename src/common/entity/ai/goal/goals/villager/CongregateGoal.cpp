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

#include "CongregateGoal.hpp"

#include "VillagerGoalUtils.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

// ============================================================================
// CongregateGoal - 村民聚集目标
// ============================================================================

CongregateGoal::CongregateGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_targetVillagerId(0)
    , m_interactCooldown(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool CongregateGoal::shouldExecute()
{
    if (!m_villager || !m_villager->world()) return false;

    // 检查是否有会议点（从 Brain 的 MEETING_POINT 记忆获取）
    auto meetingPoint = m_villager->brain().getMemory<GlobalPos>(ai::brain::memory::MemoryModuleTypes::MEETING_POINT);
    if (!meetingPoint.has_value()) return false;

    // 检查是否在会议点附近
    BlockPos meetingPos = meetingPoint->getPos();
    f32 distSq = m_villager->distanceSqTo(meetingPos.x + 0.5f, static_cast<f32>(meetingPos.y), meetingPos.z + 0.5f);
    if (distSq > 16.0f * 16.0f) return false; // 超过16格

    // 小概率触发（1%）
    math::Random& rng = m_villager->getRandom();
    if (rng.nextInt(100) != 0) return false;

    // 查找附近的其他村民
    _findInteractionTarget();
    return m_targetVillagerId != 0;
}

bool CongregateGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 检查目标是否仍然有效
    if (m_targetVillagerId == 0) return false;

    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) {
        m_targetVillagerId = 0;
        return false;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_targetVillagerId = 0;
        return false;
    }

    return m_interactCooldown > 0;
}

void CongregateGoal::startExecuting()
{
    m_interactCooldown = INTERACTION_DURATION;

    // 设置交互目标
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (entity) {
        // 移动到目标
        m_villager->tryMoveTo(entity->x(), entity->y(), entity->z(), 0.3f);
    }
}

void CongregateGoal::resetTask()
{
    m_targetVillagerId = 0;
    m_interactCooldown = 0;

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void CongregateGoal::tick()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    m_interactCooldown--;

    // 获取目标村民
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) {
        m_targetVillagerId = 0;
        return;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_targetVillagerId = 0;
        return;
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);

    // 在交互距离内
    if (distSq <= INTERACTION_DISTANCE * INTERACTION_DISTANCE) {
        // 看向目标
        if (auto* lookCtrl = m_villager->lookController()) {
            lookCtrl->setLookPosition(target->x(), target->y() + target->eyeHeight(), target->z());
        }

        // 传播流言
        _spreadGossip();

        // 分享物品（农民分享食物）
        _shareItems();
    } else {
        // 继续移动到目标
        m_villager->tryMoveTo(target->x(), target->y(), target->z(), 0.3f);
    }
}

void CongregateGoal::_findInteractionTarget()
{
    if (!m_villager || !m_villager->world()) {
        m_targetVillagerId = 0;
        return;
    }

    m_targetVillagerId = 0;

    // 查找附近的其他村民
    static constexpr f32 SEARCH_RANGE = 32.0f;

    VillagerEntity* target = EntityUtils::findClosestEntity<VillagerEntity>(
        m_villager->world(), m_villager->position(), SEARCH_RANGE, m_villager, [](VillagerEntity* entity) {
            return entity && entity->isAlive();
        });

    if (target) {
        m_targetVillagerId = target->id();
    }
}

void CongregateGoal::_spreadGossip()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    // 获取目标村民
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) return;

    VillagerEntity* targetVillager = dynamic_cast<VillagerEntity*>(entity);
    if (!targetVillager) return;

    // 调用村民的流言传播方法
    m_villager->spreadGossipTo(targetVillager);
}

void CongregateGoal::_shareItems()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    // 只有农民职业会分享食物
    if (m_villager->profession() != VillagerProfession::Farmer) return;

    // 获取目标村民
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) return;

    VillagerEntity* targetVillager = dynamic_cast<VillagerEntity*>(entity);
    if (!targetVillager || !targetVillager->isAlive()) return;

    // 农民分享食物逻辑：食物过剩时分享食物，小麦超过半组时分享小麦
    IInventory& inventory = m_villager->inventory();

    // 1. 食物分享：农民有食物过剩时分享给目标
    //    农民无条件分享给任意村民；非农民只在目标需要食物时分享
    if (m_villager->hasExcessFood()) {
        villager::throwHalfStackToTarget(m_villager, inventory, VillagerEntity::foodPoints(), targetVillager);
        return;
    }

    // 2. 小麦分享：农民有超过半组小麦时，抛出一半
    i32 wheatCount = inventory.countItem(*Items::WHEAT);
    if (wheatCount > Items::WHEAT->maxStackSize() / 2) {
        std::unordered_map<const Item*, i32> wheatOnly = {{Items::WHEAT, 1}};
        villager::throwHalfStackToTarget(m_villager, inventory, wheatOnly, targetVillager);
    }
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
