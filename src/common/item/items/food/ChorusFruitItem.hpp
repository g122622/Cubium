#pragma once

#include "FoodItem.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 紫颂果物品
 *
 * 紫颂果是一种特殊食物，食用后随机传送到附近位置。
 * 参考: net.minecraft.item.ChorusFruitItem
 *
 * 特性：
 * 1. 食用后随机传送（最大16次尝试）
 * 2. 传送范围：以玩家为中心，水平方向±8格，垂直方向±8格
 * 3. 播放传送音效
 * 4. 冷却时间20 ticks（1秒）
 */
class ChorusFruitItem : public FoodItem {
public:
    /**
     * @brief 构造紫颂果
     * @param food 食物属性
     * @param properties 物品属性
     */
    ChorusFruitItem(const food::Food* food, ItemProperties properties);

    /**
     * @brief 使用完成
     *
     * 覆盖父类方法以实现：
     * 1. 随机传送
     * 2. 播放传送音效
     * 3. 设置冷却时间
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @return 使用后的物品堆
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;
};

} // namespace item::items
} // namespace mc
