#pragma once

#include "FoodItem.hpp"
#include "../../../entity/effect/EffectType.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 蜂蜜瓶物品
 *
 * 蜂蜜瓶是一种特殊的食物，具有以下特性：
 * 1. 食用后清除中毒效果
 * 2. 返回玻璃瓶
 * 3. 使用时间40 ticks（比普通食物慢）
 *
 * 参考: net.minecraft.item.HoneyBottleItem
 */
class HoneyBottleItem : public FoodItem {
public:
    /**
     * @brief 构造蜂蜜瓶
     * @param food 食物属性
     * @param properties 物品属性
     */
    HoneyBottleItem(const food::Food* food, ItemProperties properties);

    /**
     * @brief 使用完成
     *
     * 覆盖父类方法以实现：
     * 1. 清除中毒效果
     * 2. 返回玻璃瓶
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @return 使用后的物品堆
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 获取使用时长
     *
     * 蜂蜜瓶使用时间为40 ticks（2秒）。
     *
     * @param stack 物品堆
     * @return 使用时长（ticks）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     *
     * 蜂蜜瓶返回 Drink 动作。
     *
     * @param stack 物品堆
     * @return 使用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;
};

} // namespace item::items
} // namespace mc
