#include "ElytraItem.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/inventory/PlayerInventory.hpp"
#include "../../../world/IWorld.hpp"
#include "../../armor/ArmorMaterial.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"

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
            damageElytra(stack, *living);
        }
    }

    (void)stack;
    (void)isSelected;
}

bool ElytraItem::isUsable(const ItemStack& stack)
{
    // MC 1.16.5: 差1点耐久时还能使用
    return !stack.isEmpty() && stack.isDamageable() && stack.getDamage() < stack.getMaxDamage() - 1;
}

bool ElytraItem::isGliding(const LivingEntity& entity)
{
    return entity.pose() == EntityPose::FallFlying || entity.hasFlag(EntityFlags::FallFlying);
}

void ElytraItem::damageElytra(ItemStack& stack, LivingEntity& entity)
{
    (void)entity;

    if (stack.isDamageable()) {
        stack.attemptDamageItem(1);
    }
}

} // namespace item::items
} // namespace mc
