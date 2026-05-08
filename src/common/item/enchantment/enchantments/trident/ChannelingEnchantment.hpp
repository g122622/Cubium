#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 引雷附魔
 *
 * 在雷暴天气中召唤闪电。
 * 参考 MC 1.16.5 ChannelingEnchantment
 *
 * 效果:
 * - 在雷暴天气中击中目标时召唤闪电
 * - 只有 I 级
 * - 与激流互斥
 */
class ChannelingEnchantment : public Enchantment {
public:
    ChannelingEnchantment() = default;

    [[nodiscard]] std::string id() const override {
        return "minecraft:channeling";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.channeling";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Trident;
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
        return 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        (void)level;
        return 50;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override {
        // 与激流互斥
        if (other.id() == "minecraft:riptide") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
