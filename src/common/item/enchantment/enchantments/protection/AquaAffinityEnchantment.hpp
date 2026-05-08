#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 水下速掘附魔
 *
 * 水下挖掘速度不减慢。
 * 参考 MC 1.16.5 AquaAffinityEnchantment
 *
 * 效果:
 * - 水下挖掘速度与陆地相同
 * - 只有 I 级
 */
class AquaAffinityEnchantment : public Enchantment {
public:
    AquaAffinityEnchantment() = default;

    [[nodiscard]] std::string id() const override {
        return "minecraft:aqua_affinity";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.aqua_affinity";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::ArmorHead;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 1;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        (void)level;
        return 1;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        (void)level;
        return 41;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
