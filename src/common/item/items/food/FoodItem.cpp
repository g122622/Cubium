#include "FoodItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/IWorld.hpp"

namespace mc {
namespace item::items {

FoodItem::FoodItem(const food::Food* food, ItemProperties properties)
    : Item(std::move(properties))
    , m_food(food) {
}

i32 FoodItem::getUseDuration(const ItemStack& /*stack*/) const {
    if (m_food != nullptr && m_food->isFastEat()) {
        return 16;  // 快速食用：16 ticks
    }
    return 32;  // 普通食用：32 ticks
}

UseAction FoodItem::getUseAction(const ItemStack& /*stack*/) const {
    if (m_food != nullptr && m_food->isMeat()) {
        return UseAction::Eat;
    }
    return UseAction::Drink;  // 非肉类使用饮用动作（如药水）
}

ItemActionResult FoodItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand) {
    // 检查是否可以食用
    ItemStack stack = player.getHeldItem(hand);
    if (canEat(stack, player)) {
        // TODO: 设置玩家正在使用物品
        // player.setActiveHand(hand);
        (void)hand;
        return ItemActionResult::consume(stack);
    }
    return ItemActionResult::pass(stack);
}

ItemStack FoodItem::onItemUseFinish(ItemStack& stack, IWorld& /*world*/, LivingEntity& entity) {
    if (m_food == nullptr) {
        return stack;
    }

    // 如果是玩家，应用食物效果
    // TODO: 实现 Player::getFoodStats() 和 FoodStats::eat()
    // TODO: 实现 LivingEntity::isPlayer() 和类型转换
    (void)entity;

    // 减少物品数量
    stack.shrink(1);

    // 返回容器物品（如碗、玻璃瓶）
    if (hasContainerItem()) {
        return ItemStack(containerItem(), 1);
    }

    return stack;
}

bool FoodItem::canEat(const ItemStack& /*stack*/, const Player& /*player*/) const {
    if (m_food == nullptr) {
        return false;
    }

    // 如果总是可食用，直接返回true
    if (m_food->canAlwaysEat()) {
        return true;
    }

    // 否则检查玩家是否饥饿
    // TODO: 实现 Player::canEat()
    // return player.getFoodStats().needFood();
    return true;  // 临时返回true
}

} // namespace item::items
} // namespace mc
