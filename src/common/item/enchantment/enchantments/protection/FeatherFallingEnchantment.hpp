#pragma once

#include "ProtectionEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 摔落保护附魔（羽毛落地）
 *
 * 减少摔落伤害。
 * 参考 MC 1.16.5 FeatherFallingEnchantment
 *
 * 效果:
 * - 对摔落伤害有三倍保护效果
 * - 可以与其他保护类附魔共存
 * - 最大 IV 级
 */
class FeatherFallingEnchantment : public ProtectionEnchantment {
public:
    FeatherFallingEnchantment() : ProtectionEnchantment(Type::Fall) {}

    [[nodiscard]] String id() const override {
        return "minecraft:feather_falling";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.feather_falling";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Uncommon;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
