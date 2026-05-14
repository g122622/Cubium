#pragma once

#include "DamageEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 锋利附魔
 *
 * 增加对所有生物的伤害。
 * 参考 MC 1.16.5 SharpnessEnchantment
 *
 * 效果:
 * - 每级增加 0.5-1.0 点伤害（平均 0.75 * level）
 * - 最大 V 级
 * - 与亡灵杀手、节肢杀手互斥
 */
class SharpnessEnchantment : public DamageEnchantment {
public:
    SharpnessEnchantment()
        : DamageEnchantment(Type::All)
    {}

    [[nodiscard]] std::string id() const override { return "minecraft:sharpness"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.sharpness";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Common; }
};

} // namespace enchant
} // namespace item
} // namespace mc
