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
     * @brief 获取箭矢伤害加成
     * @param level 附魔等级 (0 表示无附魔)
     * @return 伤害加成值 (0.0 = 无加成)
     *
     * MC 1.16.5 公式: 0.25 * (level + 1)
     * - I: 0.5 (箭矢伤害 +50%)
     * - II: 0.75 (箭矢伤害 +75%)
     * - III: 1.0 (箭矢伤害 +100%)
     * - IV: 1.25 (箭矢伤害 +125%)
     * - V: 1.5 (箭矢伤害 +150%)
     */
    [[nodiscard]] static f32 getArrowDamageBonus(i32 level) {
        if (level <= 0) return 0.0f;
        return 0.25f * static_cast<f32>(level + 1);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
