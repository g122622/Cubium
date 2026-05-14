#pragma once

#include "ProtectionEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 全保护附魔
 *
 * 减少所有类型的伤害。
 * 参考 MC 1.16.5 ProtectionEnchantment (ALL type)
 *
 * 效果:
 * - 每级提供 4% 的伤害减免（EPF = level）
 * - 最大 IV 级
 * - 与其他保护类附魔互斥（摔落保护除外）
 */
class AllProtectionEnchantment : public ProtectionEnchantment {
public:
    AllProtectionEnchantment()
        : ProtectionEnchantment(Type::All)
    {}

    [[nodiscard]] std::string id() const override { return "minecraft:protection"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.protection";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Common; }
};

} // namespace enchant
} // namespace item
} // namespace mc
