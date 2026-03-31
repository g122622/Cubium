#include "ElytraItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../armor/ArmorMaterial.hpp"

namespace mc {
namespace item::items {

ElytraItem::ElytraItem(ItemProperties properties)
    : Item(std::move(properties)) {
    // 鞘翅应该设置耐久度为432
}

ItemActionResult ElytraItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    // TODO: 实现鞘翅装备逻辑
    // 需要在 Player 类中添加 getArmorStack/setArmorStack 方法
    (void)world;
    (void)player;
    (void)hand;
    return ItemActionResult::pass(player.getHeldItem(hand));
}

void ElytraItem::inventoryTick(ItemStack& stack, IWorld& world, Entity& entity,
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
