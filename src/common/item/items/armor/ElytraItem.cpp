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

#include "ElytraItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include <utility>

namespace mc {
namespace item::items {

ElytraItem::ElytraItem(ItemProperties properties)
    : Item(std::move(properties).maxDamage(MAX_DURABILITY))
{}

ItemActionResult ElytraItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    (void)world;

    ItemStack& heldStack = player.getHeldItem(hand);
    if (heldStack.isEmpty() || !isUsable(heldStack)) {
        return ItemActionResult::pass(heldStack);
    }

    PlayerInventory& inventory = player.inventory();
    if (!inventory.getChestplate().isEmpty()) {
        return ItemActionResult::pass(heldStack);
    }

    inventory.setChestplate(heldStack);
    heldStack = ItemStack();
    return ItemActionResult::consume(ItemStack());
}

void ElytraItem::inventoryTick(ItemStack& stack, IWorld& world, Entity& entity, i32 itemSlot, bool isSelected) const
{
    LivingEntity* living = dynamic_cast<LivingEntity*>(&entity);
    if (living != nullptr && itemSlot == InventorySlots::ARMOR_CHEST && isGliding(*living)) {
        if (world.currentTick() % 20 == 0) {
            damageElytra(stack, *living, EquipmentSlot::Chest);
        }
    }

    (void)stack;
    (void)isSelected;
}

bool ElytraItem::isUsable(const ItemStack& stack)
{
    // 差1点耐久时还能使用
    return !stack.isEmpty() && stack.isDamageable() && stack.getDamage() < stack.getMaxDamage() - 1;
}

bool ElytraItem::isGliding(const LivingEntity& entity)
{
    return entity.pose() == EntityPose::FallFlying || entity.hasFlag(EntityFlags::FallFlying);
}

void ElytraItem::damageElytra(ItemStack& stack, LivingEntity& entity, EquipmentSlot slot)
{
    if (stack.isDamageable()) {
        LivingEntity::hurtAndBreak(stack, 1, &entity, slot);
    }
}

} // namespace item::items
} // namespace mc
