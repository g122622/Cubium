#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 力量附魔
 *
 * 增加弓箭的伤害。
 * 参考 MC 1.16.5 PowerEnchantment
 *
 * 效果:
 * - I: +25% 伤害, II: +50%, III: +75%, IV: +100%, V: +150%
 * - 最大 V 级
 */
class PowerEnchantment : public Enchantment {
public:
    PowerEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:power";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.power";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Bow;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 5;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Common;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 1 + (level - 1) * 10;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 15;
    }

    /**
     * @brief 获取伤害加成
     * @param level 附魔等级
     * @return 伤害乘数
     */
    [[nodiscard]] static f32 getDamageMultiplier(i32 level) {
        // I: +25%, II: +50%, III: +75%, IV: +100%, V: +150%
        if (level == 5) {
            return 2.5f;  // +150%
        }
        return 1.0f + static_cast<f32>(level) * 0.25f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
