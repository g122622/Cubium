#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 绑定诅咒附魔
 *
 * 防止物品被取下。
 * 参考 MC 1.16.5 BindingCurseEnchantment
 *
 * 效果:
 * - 装备后无法取下（除非物品损坏或死亡）
 * - 只有 I 级
 * - 属于诅咒附魔
 */
class BindingCurseEnchantment : public Enchantment {
public:
    BindingCurseEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:binding_curse"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.binding_curse";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Wearable; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] bool isCurse() const override { return true; }

    [[nodiscard]] bool isTreasure() const override { return true; }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
