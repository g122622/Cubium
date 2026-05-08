#pragma once

#include "ThrowablePotionItem.hpp"

namespace mc {

namespace potion {
class Potion;
}

namespace item {

/**
 * @brief 喷溅药水物品
 *
 * 可投掷的药水，落地时在区域内应用效果。
 * 继承ThrowablePotionItem以获得药水物品的共享行为。
 *
 * 参考: net.minecraft.item.SplashPotionItem
 */
class SplashPotionItem : public ThrowablePotionItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit SplashPotionItem(const ItemProperties& properties);

    ~SplashPotionItem() override = default;

    // ========== ThrowableItem 接口重写 ==========

    /**
     * @brief 创建投掷实体
     * @return 药水实体（喷溅型）
     */
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world,
        Player& player,
        const ItemStack& stack) const override;

protected:
    /**
     * @brief 获取基础翻译键
     */
    [[nodiscard]] std::string getBaseTranslationKey() const override {
        return std::string("item.minecraft.splash_potion");
    }

    /**
     * @brief 获取带效果后缀的翻译键前缀
     */
    [[nodiscard]] std::string getEffectTranslationKeyPrefix() const override {
        return std::string("item.minecraft.splash_potion.effect.");
    }
};

} // namespace item
} // namespace mc
