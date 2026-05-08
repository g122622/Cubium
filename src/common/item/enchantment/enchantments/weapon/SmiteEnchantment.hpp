#pragma once

#include "DamageEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 亡灵杀手附魔
 *
 * 增加对亡灵生物的伤害。
 * 参考 MC 1.16.5 SmiteEnchantment
 *
 * 效果:
 * - 对亡灵生物每级增加 2.5 点伤害
 * - 亡灵生物包括：僵尸、骷髅、凋灵、幻翼等
 * - 最大 V 级
 * - 与锋利、节肢杀手互斥
 */
class SmiteEnchantment : public DamageEnchantment {
public:
    SmiteEnchantment() : DamageEnchantment(Type::Undead) {}

    [[nodiscard]] std::string id() const override {
        return "minecraft:smite";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.smite";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Uncommon;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
