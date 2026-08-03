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

#include "ShareItemsGoal.hpp"

#include "VillagerGoalUtils.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/Items.hpp"
#include "common/world/IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

// ============================================================================
// ShareItemsGoal - 分享物品目标
// ============================================================================

ShareItemsGoal::ShareItemsGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_targetVillagerId(0)
    , m_shareCooldown(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool ShareItemsGoal::shouldExecute()
{
    if (!m_villager || !m_villager->world()) return false;

    // 只有农民职业会分享食物
    if (m_villager->profession() != VillagerProfession::Farmer) return false;

    // 冷却时间
    if (m_shareCooldown > 0) return false;

    // 检查是否有多余的食物可以分享（食物点数 >= 24 或小麦超过半组）
    if (!_canAbandonItems()) return false;

    // 查找附近需要食物的村民
    // 农民只要有食物过剩就会分享给任意村民（无论对方是否需要），
    // 但也可以优先选择需要食物的村民
    static constexpr f32 SEARCH_RANGE = 8.0f;

    // 先尝试找到需要食物的村民
    VillagerEntity* target = EntityUtils::findClosestEntity<VillagerEntity>(
        m_villager->world(), m_villager->position(), SEARCH_RANGE, m_villager, [this](VillagerEntity* entity) {
            return entity && entity->isAlive() && _targetNeedsFoodForTarget(entity);
        });

    // 如果没有需要食物的村民，农民也会分享给任意村民
    if (!target) {
        target = EntityUtils::findClosestEntity<VillagerEntity>(
            m_villager->world(), m_villager->position(), SEARCH_RANGE, m_villager, [](VillagerEntity* entity) {
                return entity && entity->isAlive();
            });
    }

    if (target) {
        m_targetVillagerId = target->id();
        return true;
    }

    return false;
}

bool ShareItemsGoal::shouldContinueExecuting()
{
    if (!m_villager || m_targetVillagerId == 0) return false;

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

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);
    return distSq <= SHARE_DISTANCE * SHARE_DISTANCE * 4.0f;
}

void ShareItemsGoal::startExecuting()
{
    m_shareCooldown = SHARE_COOLDOWN;

    // 移动到目标
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (entity) {
        m_villager->tryMoveTo(entity->x(), entity->y(), entity->z(), 0.5f);
    }
}

void ShareItemsGoal::resetTask()
{
    m_targetVillagerId = 0;

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void ShareItemsGoal::tick()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    if (m_shareCooldown > 0) {
        m_shareCooldown--;
    }

    // 获取目标
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

    // 看向目标
    if (auto* lookCtrl = m_villager->lookController()) {
        lookCtrl->setLookPosition(target->x(), target->y() + target->eyeHeight(), target->z());
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);
    if (distSq <= SHARE_DISTANCE * SHARE_DISTANCE) {
        // 分享食物
        _shareFoodWithTarget();
    } else {
        // 继续移动
        m_villager->tryMoveTo(target->x(), target->y(), target->z(), 0.5f);
    }
}

bool ShareItemsGoal::_canAbandonItems() const
{
    if (!m_villager) return false;

    // 1. 如果村民有食物过剩（食物点数 >= 24），则可以分享食物
    // 2. 如果是农民且小麦超过半组（>32），则可以分享小麦
    if (m_villager->hasExcessFood()) {
        return true;
    }

    // 农民特殊检查：小麦超过半组时也愿意分享
    if (m_villager->profession() == VillagerProfession::Farmer) {
        IInventory& inventory = m_villager->inventory();
        i32 wheatCount = inventory.countItem(*Items::WHEAT);
        if (wheatCount > Items::WHEAT->maxStackSize() / 2) {
            return true;
        }
    }

    return false;
}

bool ShareItemsGoal::_targetNeedsFoodForTarget(VillagerEntity* target) const
{
    // 检查目标村民是否需要食物（食物点数 < 12）
    if (!target) return false;
    return target->wantsMoreFood();
}

void ShareItemsGoal::_shareFoodWithTarget()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    IWorld* world = m_villager->world();
    if (!world) return;

    // 获取目标
    Entity* entity = world->getEntity(m_targetVillagerId);
    if (!entity) {
        m_targetVillagerId = 0;
        m_shareCooldown = SHARE_COOLDOWN;
        return;
    }

    VillagerEntity* targetVillager = dynamic_cast<VillagerEntity*>(entity);
    if (!targetVillager) {
        m_targetVillagerId = 0;
        m_shareCooldown = SHARE_COOLDOWN;
        return;
    }

    // 物品分享逻辑：按优先级依次检查食物分享 -> 小麦分享

    IInventory& inventory = m_villager->inventory();
    bool shared = false;

    // 1. 食物分享：如果有食物过剩，向目标抛出一半食物
    if (m_villager->hasExcessFood()) {
        shared = villager::throwHalfStackToTarget(m_villager, inventory, VillagerEntity::foodPoints(), targetVillager);
    }

    // 2. 小麦分享：农民有超过半组小麦时，抛出一半
    if (!shared && m_villager->profession() == VillagerProfession::Farmer) {
        i32 wheatCount = inventory.countItem(*Items::WHEAT);
        i32 halfStack = Items::WHEAT->maxStackSize() / 2;
        if (wheatCount > halfStack) {
            std::unordered_map<const Item*, i32> wheatOnly = {{Items::WHEAT, 1}};
            shared = villager::throwHalfStackToTarget(m_villager, inventory, wheatOnly, targetVillager);
        }
    }

    // 重置目标
    m_targetVillagerId = 0;
    m_shareCooldown = SHARE_COOLDOWN;
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
