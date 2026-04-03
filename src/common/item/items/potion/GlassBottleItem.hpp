#pragma once

#include "../../core/Item.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {
namespace item {

/**
 * @brief 玻璃瓶物品
 *
 * 可以从水源或炼药锅中装水，变为水瓶。
 * 水瓶是酿造的基础材料。
 *
 * 参考: net.minecraft.item.GlassBottleItem
 */
class GlassBottleItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit GlassBottleItem(const ItemProperties& properties);

    /**
     * @brief 右键使用
     *
     * 对水源使用：装水变为水瓶
     * 对炼药锅使用：装水变为水瓶
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;
};

} // namespace item
} // namespace mc
