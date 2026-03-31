#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 冲击附魔
 *
 * 增加箭矢的击退距离。
 * 参考 MC 1.16.5 PunchEnchantment
 *
 * 效果:
 * - 每级增加 3 格击退距离
 * - 最大 II 级
 */
class PunchEnchantment : public Enchantment {
public:
    PunchEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:punch";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.punch";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Bow;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 2;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 12 + (level - 1) * 20;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 25;
    }

    /**
     * @brief 获取击退距离加成
     * @param level 附魔等级
     * @return 额外击退距离（格）
     */
    [[nodiscard]] static f32 getKnockbackBonus(i32 level) {
        // 每级增加 3 格击退
        return static_cast<f32>(level) * 3.0f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
