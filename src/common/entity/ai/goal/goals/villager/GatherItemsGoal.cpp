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

#include "GatherItemsGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

GatherItemsGoal::GatherItemsGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_targetItem(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool GatherItemsGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 查找附近的物品
    _findNearbyItems();
    return m_targetItem != 0;
}

bool GatherItemsGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 物品已被拾取或消失
    if (m_targetItem == 0) return false;

    // 检查物品是否仍然有效
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetItem) : nullptr;
    if (!entity) return false;

    ItemEntity* item = dynamic_cast<ItemEntity*>(entity);
    if (!item || !item->isAlive() || !item->canBePickedUp()) {
        return false;
    }

    // 检查物品是否仍在范围内
    f32 distSq = m_villager->distanceSqTo(*item);
    if (distSq > PICKUP_RANGE * PICKUP_RANGE) {
        return false;
    }

    return true;
}

void GatherItemsGoal::startExecuting()
{
    // 已在shouldExecute中找到目标
}

void GatherItemsGoal::resetTask()
{
    m_targetItem = 0;
    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void GatherItemsGoal::tick()
{
    if (!m_villager || m_targetItem == 0) return;

    // 移动到物品
    _moveToItem();

    // 尝试拾取
    _pickupItem();
}

void GatherItemsGoal::_findNearbyItems()
{
    if (!m_villager || !m_villager->world()) {
        m_targetItem = 0;
        return;
    }

    m_targetItem = 0;

    // 使用 EntityUtils 查找最近的 ItemEntity
    ItemEntity* item = EntityUtils::findClosestEntity<ItemEntity>(m_villager->world(),
        m_villager->position(),
        PICKUP_RANGE,
        m_villager, // 排除自己（虽然村民不是 ItemEntity）
        [this](ItemEntity* itemEntity) {
            // 检查物品实体是否有效
            if (!itemEntity || !itemEntity->isAlive()) return false;

            // 检查是否可以被拾取（拾取延迟等）
            if (!itemEntity->canBePickedUp()) return false;

            // 检查村民是否想要这个物品
            const ItemStack& stack = itemEntity->getItemStack();
            if (stack.isEmpty()) return false;

            // 使用 VillagerEntity::canPickUpItem 检查是否是村民可拾取的物品
            return m_villager->canPickUpItem(stack);
        });

    if (item) {
        m_targetItem = item->id();
    }
}

void GatherItemsGoal::_moveToItem()
{
    if (!m_villager || m_targetItem == 0) return;

    // 从世界获取实体
    Entity* entity = m_villager->world()->getEntity(m_targetItem);
    if (!entity) {
        m_targetItem = 0;
        return;
    }

    ItemEntity* item = dynamic_cast<ItemEntity*>(entity);
    if (!item || !item->isAlive()) {
        m_targetItem = 0;
        return;
    }

    // 检查物品是否还能被拾取
    if (!item->canBePickedUp()) {
        m_targetItem = 0;
        return;
    }

    // 移动到物品位置
    // 村民移动速度约 0.5
    m_villager->tryMoveTo(item->x(), item->y(), item->z(), 0.5);
}

void GatherItemsGoal::_pickupItem()
{
    if (!m_villager || m_targetItem == 0) return;

    // 从世界获取实体
    Entity* entity = m_villager->world()->getEntity(m_targetItem);
    if (!entity) {
        m_targetItem = 0;
        return;
    }

    ItemEntity* item = dynamic_cast<ItemEntity*>(entity);
    if (!item || !item->isAlive() || !item->canBePickedUp()) {
        m_targetItem = 0;
        return;
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*item);
    if (distSq > PICKUP_DISTANCE * PICKUP_DISTANCE) {
        return; // 还没到拾取距离
    }

    // 获取物品堆
    ItemStack stack = item->getItemStack();
    if (stack.isEmpty()) {
        m_targetItem = 0;
        return;
    }

    // 再次确认村民可以拾取这个物品
    if (!m_villager->canPickUpItem(stack)) {
        m_targetItem = 0;
        return;
    }

    // 将物品添加到村民库存
    IInventory& inventory = m_villager->inventory();
    ItemStack remaining = inventory.addItem(stack);

    // 如果有剩余物品（库存满了），更新物品实体的数量
    if (!remaining.isEmpty()) {
        item->setItemStack(remaining);
    } else {
        // 完全拾取，移除物品实体
        item->remove();
    }

    m_targetItem = 0; // 清除目标
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
