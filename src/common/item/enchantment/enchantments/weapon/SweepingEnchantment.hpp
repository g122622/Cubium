#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 横扫之刃附魔
 *
 * 增加横扫攻击的伤害。
 * 参考 MC 1.16.5 SweepingEnchantment
 *
 * 效果:
 * - I: 50% 伤害传递
 * - II: 67% 伤害传递
 * - III: 75% 伤害传递
 * - 最大 III 级
 */
class SweepingEnchantment : public Enchantment {
public:
    SweepingEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:sweeping";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.sweeping";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Weapon;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 5 + (level - 1) * 9;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 15;  // MC 1.16.5: getMinEnchantability + 15
    }

    /**
     * @brief 获取横扫伤害比例
     * @param level 附魔等级
     * @return 伤害传递比例 (0.0-1.0)
     */
    [[nodiscard]] static f32 getSweepingDamageRatio(i32 level) {
        // I: 50%, II: 67%, III: 75%
        // 公式: 1 / (level + 1) 的补数
        return 1.0f - (1.0f / static_cast<f32>(level + 1));
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
