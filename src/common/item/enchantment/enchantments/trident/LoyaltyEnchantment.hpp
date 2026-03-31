#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 忠诚附魔
 *
 * 三叉戟投掷后自动返回。
 * 参考 MC 1.16.5 LoyaltyEnchantment
 *
 * 效果:
 * - 每级减少返回时间
 * - 最大 III 级
 * - 在末地无效
 */
class LoyaltyEnchantment : public Enchantment {
public:
    LoyaltyEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:loyalty";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.loyalty";
    }

    [[nodiscard]] EnchantmentType type() const override {
        // 使用自定义类型（需要在 EnchantmentType 中添加 Trident）
        return static_cast<EnchantmentType>(100);  // 临时方案
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Uncommon;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 5 + (level - 1) * 7;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 50;
    }

    /**
     * @brief 获取返回速度
     * @param level 附魔等级
     * @return 返回速度乘数
     */
    [[nodiscard]] static f32 getReturnSpeed(i32 level) {
        // 每级增加返回速度
        return 0.5f + static_cast<f32>(level) * 0.5f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
