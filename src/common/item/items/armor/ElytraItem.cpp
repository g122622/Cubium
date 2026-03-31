#include "ElytraItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/World.hpp"

namespace mc {
namespace item::items {

ElytraItem::ElytraItem(ItemProperties properties)
    : Item(std::move(properties)) {
    // 鞘翅应该设置耐久度为432
}

ItemActionResult ElytraItem::onItemRightClick(World& world, Player& player, Hand hand) {
    // 检查胸甲槽位
    ItemStack chestItem = player.getArmorStack(static_cast<i32>(armor::ArmorSlot::Chest));

    if (chestItem.isEmpty()) {
        // 装备鞘翅
        ItemStack heldItem = player.getHeldItem(hand);
        heldItem.shrink(1);
        player.setArmorStack(static_cast<i32>(armor::ArmorSlot::Chest), heldItem.copy());

        // TODO: 播放装备音效
        (void)world;
        return ItemActionResult::consume(heldItem);
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

void ElytraItem::inventoryTick(ItemStack& stack, World& world, Entity& entity,
                                i32 itemSlot, bool isSelected) {
    // 检查是否是LivingEntity且正在滑翔
    LivingEntity* living = dynamic_cast<LivingEntity*>(&entity);
    if (living != nullptr && isGliding(*living)) {
        // 滑翔时每秒消耗1点耐久度（每20tick消耗1点）
        // TODO: 实现耐久度消耗
        // if (world.getGameTime() % 20 == 0) {
        //     stack.attemptDamageItem(1, living);
        // }
    }

    (void)stack;
    (void)world;
    (void)itemSlot;
    (void)isSelected;
}

bool ElytraItem::isUsable(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }
    i32 damage = stack.getDamage();
    i32 maxDamage = stack.getMaxDamage();
    return damage < maxDamage;
}

bool ElytraItem::isGliding(const LivingEntity& entity) {
    // TODO: 实现滑翔状态检查
    // return entity.isFallFlying();
    (void)entity;
    return false;
}

void ElytraItem::damageElytra(ItemStack& stack, LivingEntity& entity) {
    // 每滑翔1秒消耗1点耐久度
    // stack.attemptDamageItem(1, &entity);
    (void)stack;
    (void)entity;
}

} // namespace item::items
} // namespace mc
