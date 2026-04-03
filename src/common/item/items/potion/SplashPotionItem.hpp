#pragma once

#include "../../core/Item.hpp"

namespace mc {

namespace potion {
class Potion;
}

namespace item {

/**
 * @brief 喷溅药水物品
 *
 * 可投掷的药水，落地时在区域内应用效果。
 *
 * 参考: net.minecraft.item.SplashPotionItem
 */
class SplashPotionItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit SplashPotionItem(const ItemProperties& properties);

    /**
     * @brief 右键使用物品
     *
     * 喷溅药水被投掷而非饮用。
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

private:
    /**
     * @brief 应用药水效果到区域内的实体
     * @param potion 药水类型
     * @param world 世界
     * @param pos 位置
     * @param radius 半径
     */
    void applySplashEffects(const potion::Potion* potion, IWorld& world,
                           const BlockPos& pos, f32 radius);
};

} // namespace item
} // namespace mc
