#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 消失诅咒附魔
 *
 * 死亡时物品消失。
 * 参考 MC 1.16.5 VanishingCurseEnchantment
 *
 * 效果:
 * - 玩家死亡时物品消失而不是掉落
 * - 只有 I 级
 * - 属于诅咒附魔
 */
class VanishingCurseEnchantment : public Enchantment {
public:
    VanishingCurseEnchantment() = default;

    [[nodiscard]] std::string id() const override {
        return "minecraft:vanishing_curse";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.vanishing_curse";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Vanishable;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 1;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::VeryRare;
    }

    [[nodiscard]] bool isCurse() const override {
        return true;
    }

    [[nodiscard]] bool isTreasure() const override {
        return true;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        (void)level;
        return 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        (void)level;
        return 50;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
