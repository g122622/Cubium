#pragma once

#include "ProtectionEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 爆炸保护附魔
 *
 * 减少爆炸伤害。
 * 参考 MC 1.16.5 BlastProtectionEnchantment
 *
 * 效果:
 * - 对爆炸伤害有双倍保护效果
 * - 对其他伤害也有基础保护
 * - 最大 IV 级
 */
class BlastProtectionEnchantment : public ProtectionEnchantment {
public:
    BlastProtectionEnchantment()
        : ProtectionEnchantment(Type::Explosion)
    {}

    [[nodiscard]] std::string id() const override { return "minecraft:blast_protection"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.blast_protection";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }
};

} // namespace enchant
} // namespace item
} // namespace mc
