#pragma once

#include "ProtectionEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 火焰保护附魔
 *
 * 减少火焰伤害。
 * 参考 MC 1.16.5 FireProtectionEnchantment
 *
 * 效果:
 * - 对火焰伤害（燃烧、岩浆、火）有双倍保护效果
 * - 对其他伤害也有基础保护
 * - 最大 IV 级
 */
class FireProtectionEnchantment : public ProtectionEnchantment {
public:
    FireProtectionEnchantment() : ProtectionEnchantment(Type::Fire) {}

    [[nodiscard]] String id() const override {
        return "minecraft:fire_protection";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.fire_protection";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Uncommon;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
