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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "OnAStickItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IRideable.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include <string>

namespace mc {
namespace item {

OnAStickItem::OnAStickItem(const ItemProperties& properties, const std::string& entityId, i32 durabilityCost)
    : Item(properties)
    , m_entityId(entityId)
    , m_durabilityCost(durabilityCost)
{}

ItemActionResult OnAStickItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 获取玩家当前手持物品
    ItemStack& heldItem = player.getHeldItem(hand);

    // 检查玩家是否正在骑乘
    EntityInstanceId vehicleId = player.getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        // 玩家没有骑乘任何实体
        return ItemActionResult::pass(heldItem);
    }

    // 获取骑乘的实体
    Entity* vehicleEntity = world.getEntity(vehicleId);
    if (vehicleEntity == nullptr) {
        return ItemActionResult::pass(heldItem);
    }

    // 检查实体类型是否匹配
    if (vehicleEntity->getTypeId() != m_entityId) {
        return ItemActionResult::pass(heldItem);
    }

    // 尝试将实体转换为 IRideable 接口
    entity::IRideable* rideable = dynamic_cast<entity::IRideable*>(vehicleEntity);
    if (rideable == nullptr) {
        return ItemActionResult::pass(heldItem);
    }

    // 触发加速效果
    bool boosted = rideable->boost();
    if (!boosted) {
        // 加速失败（可能已经在加速中，或者没有装备鞍）
        return ItemActionResult::pass(heldItem);
    }

    // 消耗耐久度，若物品损坏则触发 onEquippedItemBroken 回调
    EquipmentSlot slot = LivingEntity::handToEquipmentSlot(hand);
    bool broken = LivingEntity::hurtAndBreak(heldItem, m_durabilityCost, &player, slot);

    // 检查物品是否损坏
    if (broken) {
        // 如果物品耐久度耗尽，转换为钓鱼竿
        if (Items::FISHING_ROD != nullptr) {
            ItemStack fishingRod(*Items::FISHING_ROD, 1);
            return ItemActionResult::success(fishingRod);
        }
        return ItemActionResult::success(ItemStack::EMPTY);
    }

    return ItemActionResult::success(heldItem);
}

} // namespace item
} // namespace mc
