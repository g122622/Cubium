#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 无限附魔
 *
 * 射箭不消耗普通箭矢。
 * 参考 MC 1.16.5 InfinityEnchantment
 *
 * 效果:
 * - 射箭时不消耗普通箭矢（需要至少一支箭）
 * - 药水箭和光灵箭仍然消耗
 * - 只有 I 级
 * - 与经验修补互斥
 */
class InfinityEnchantment : public Enchantment {
public:
    InfinityEnchantment() = default;

    [[nodiscard]] std::string id() const override {
        return "minecraft:infinity";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.infinity";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Bow;
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

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        (void)level;
        return 20;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        (void)level;
        return 50;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override {
        // 与经验修补互斥
        if (other.id() == "minecraft:mending") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
