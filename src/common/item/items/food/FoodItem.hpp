#pragma once

#include "../../core/Item.hpp"
#include "../../food/Food.hpp"
#include "../../core/UseAction.hpp"

namespace mc {

class Player;
class IWorld;

namespace item::items {

/**
 * @brief 食物物品基类
 *
 * 负责处理可食用物品的基础行为。
 */
class FoodItem : public Item {
public:
    /**
     * @brief 构造食物物品
     * @param food 食物属性
     * @param properties 物品属性
     */
    FoodItem(const food::Food* food, ItemProperties properties);

    /**
     * @brief 是否为食物
     */
    [[nodiscard]] bool isFood() const override { return m_food != nullptr; }

    /**
     * @brief 获取食物属性
     */
    [[nodiscard]] const food::Food* getFood() const override { return m_food; }

    /**
     * @brief 获取使用时长
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 使用完成
     *
     * 玩家会在这里恢复饥饿值；其他实体保持无副作用。
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 是否可以食用
     */
    [[nodiscard]] bool canEat(const ItemStack& stack, const Player& player) const override;

protected:
    const food::Food* m_food;
};

} // namespace item::items
} // namespace mc
