#pragma once

#include "ThrowablePotionItem.hpp"

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
 * 继承ThrowablePotionItem以获得药水物品的共享行为。
 *
 * 参考: net.minecraft.item.LingeringPotionItem
 */
class LingeringPotionItem : public ThrowablePotionItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit LingeringPotionItem(const ItemProperties& properties);

    ~LingeringPotionItem() override = default;

    // ========== ThrowableItem 接口重写 ==========

    /**
     * @brief 创建投掷实体
     * @return 药水实体（滞留型）
     */
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world,
        Player& player,
        const ItemStack& stack) const override;

protected:
    /**
     * @brief 获取基础翻译键
     */
    [[nodiscard]] String getBaseTranslationKey() const override {
        return String("item.minecraft.lingering_potion");
    }

    /**
     * @brief 获取带效果后缀的翻译键前缀
     */
    [[nodiscard]] String getEffectTranslationKeyPrefix() const override {
        return String("item.minecraft.lingering_potion.effect.");
    }
};

} // namespace item
} // namespace mc
