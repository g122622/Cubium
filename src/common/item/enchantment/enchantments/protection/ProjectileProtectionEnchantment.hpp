#pragma once

#include "ProtectionEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 弹射物保护附魔
 *
 * 减少弹射物伤害。
 * 参考 MC 1.16.5 ProjectileProtectionEnchantment
 *
 * 效果:
 * - 对弹射物伤害（箭、恶魂火球等）有双倍保护效果
 * - 对其他伤害也有基础保护
 * - 最大 IV 级
 */
class ProjectileProtectionEnchantment : public ProtectionEnchantment {
public:
    ProjectileProtectionEnchantment() : ProtectionEnchantment(Type::Projectile) {}

    [[nodiscard]] String id() const override {
        return "minecraft:projectile_protection";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.projectile_protection";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Uncommon;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
