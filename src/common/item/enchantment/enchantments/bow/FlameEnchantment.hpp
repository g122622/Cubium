#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 火矢附魔
 *
 * 箭矢点燃目标。
 * 参考 MC 1.16.5 FlameEnchantment
 *
 * 效果:
 * - 箭矢点燃目标 5 秒（100 tick）
 * - 只有 I 级
 */
class FlameEnchantment : public Enchantment {
public:
    FlameEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:flame"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.flame";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Bow; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 20;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }

    /**
     * @brief 获取燃烧时间（tick）
     * @return 燃烧时间（tick）
     */
    [[nodiscard]] static i32 getFireDuration()
    {
        return 100; // 5 秒
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
