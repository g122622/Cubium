#include "FoodItem.hpp"

#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 构造食物物品
 */
FoodItem::FoodItem(const food::Food* food, ItemProperties properties)
    : Item(std::move(properties))
    , m_food(food) {
}

/**
 * @brief 获取使用时长
 */
i32 FoodItem::getUseDuration(const ItemStack& /*stack*/) const {
    if (m_food != nullptr && m_food->isFastEat()) {
        return 16;
    }
    return 32;
}

/**
 * @brief 获取使用动作
 */
UseAction FoodItem::getUseAction(const ItemStack& /*stack*/) const {
    if (m_food != nullptr && m_food->isMeat()) {
        return UseAction::Eat;
    }
    return UseAction::Drink;
}

/**
 * @brief 右键使用物品
 */
ItemActionResult FoodItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand) {
    const ItemStack stack = player.getHeldItem(hand);
    if (canEat(stack, player)) {
        return ItemActionResult::consume(stack);
    }
    return ItemActionResult::pass(stack);
}

/**
 * @brief 使用完成
 */
ItemStack FoodItem::onItemUseFinish(ItemStack& stack, IWorld& /*world*/, Entity& entity) {
    if (m_food == nullptr) {
        return stack;
    }

    if (auto* player = dynamic_cast<Player*>(&entity)) {
        player->foodStats().addStats(m_food->getHunger(), m_food->getSaturation());
        player->foodStats().foodTimer = 0;
    }

    stack.shrink(1);

    if (hasContainerItem()) {
        return ItemStack(containerItem(), 1);
    }

    return stack;
}

/**
 * @brief 是否可以食用
 */
bool FoodItem::canEat(const ItemStack& /*stack*/, const Player& player) const {
    if (m_food == nullptr) {
        return false;
    }

    if (m_food->canAlwaysEat()) {
        return true;
    }

    return player.foodStats().needsFood();
}

} // namespace item::items
} // namespace mc
