#pragma once

#include "../weapon/ThrowableItem.hpp"

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
 * 继承ThrowableItem以获得投掷行为。
 *
 * 参考: net.minecraft.item.LingeringPotionItem
 */
class LingeringPotionItem : public ThrowableItem {
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

    /**
     * @brief 播放投掷音效
     */
    void playThrowSound(Player& player) const override;

    // ========== Item 接口重写 ==========

    /**
     * @brief 是否有附魔光效
     * @param stack 物品堆
     * @return 如果药水有效果则返回true
     */
    [[nodiscard]] bool hasEffect(const ItemStack& stack) const override;

    /**
     * @brief 获取翻译键
     * @param stack 物品堆
     * @return 带药水类型的翻译键
     */
    [[nodiscard]] String getTranslationKey(const ItemStack& stack) const override;

    /**
     * @brief 获取投掷速度
     * MC 1.16.5: 药水投掷速度为 0.5
     */
    [[nodiscard]] f32 getThrowVelocity() const override { return 0.5f; }

    /**
     * @brief 获取投掷偏移
     * MC 1.16.5: 药水投掷偏移为 0.0f
     */
    [[nodiscard]] f32 getThrowInaccuracy() const override { return 0.0f; }
};

} // namespace item
} // namespace mc
