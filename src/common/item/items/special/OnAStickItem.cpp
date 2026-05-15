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

#include "../../../entity/core/Entity.hpp"
#include "../../../entity/interfaces/IRideable.hpp"
#include "../../../world/IWorld.hpp"
#include "../../Items.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../core/Types.hpp"

namespace mc {
namespace item {

OnAStickItem::OnAStickItem(
    const ItemProperties& properties,
    const std::string& entityId,
    i32 durabilityCost)
    : Item(properties)
    , m_entityId(entityId)
    , m_durabilityCost(durabilityCost)
{
}

ItemActionResult OnAStickItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // MC 1.16.5: OnAStickItem.onItemRightClick()
    // 1. 检查玩家是否正在骑乘
    // 2. 检查骑乘的实体是否是目标类型
    // 3. 检查实体是否实现了 IRideable 接口
    // 4. 触发加速效果
    // 5. 消耗耐久度
    // 6. 如果耐久度耗尽，返回钓鱼竿

    // 获取玩家当前手持物品
    ItemStack heldItem = player.getHeldItem(hand);

    // 检查玩家是否正在骑乘
    EntityId vehicleId = player.getVehicle();
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
    // MC 1.16.5: entity.getType() == this.field_234680_a_
    // 使用实体类型ID进行比较
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

    // 消耗耐久度
    // MC 1.16.5: itemstack.damageItem(this.field_234681_b_, player, ...)
    bool damaged = heldItem.attemptDamageItem(m_durabilityCost, &player);
    MC_UNUSED(damaged);

    // 检查物品是否损坏
    if (heldItem.isEmpty()) {
        // MC 1.16.5: 返回钓鱼竿
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
