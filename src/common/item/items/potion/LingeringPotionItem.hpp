#pragma once

#include "../../core/Item.hpp"

namespace mc {

namespace potion {
class Potion;
}

namespace item {

/**
 * @brief 滞留药水物品
 *
 * 投掷后产生滞留区域，在区域内的实体会获得效果。
 * 持续约30秒，每秒应用一次效果。
 *
 * 参考: net.minecraft.item.LingeringPotionItem
 */
class LingeringPotionItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit LingeringPotionItem(const ItemProperties& properties);

    /**
     * @brief 右键使用物品
     *
     * 滞留药水被投掷而非饮用。
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 是否有附魔光效
     * @param stack 物品堆
     * @return 如果药水有效果则返回true
     */
    [[nodiscard]] bool hasEffect(const ItemStack& stack) const;

    /**
     * @brief 获取翻译键
     * @param stack 物品堆
     * @return 带药水类型的翻译键
     */
    [[nodiscard]] String getTranslationKey(const ItemStack& stack) const override;
};

} // namespace item
} // namespace mc
