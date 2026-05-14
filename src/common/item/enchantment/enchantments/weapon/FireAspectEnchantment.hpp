#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 火焰附加附魔
 *
 * 使被击中的目标燃烧。
 * 参考 MC 1.16.5 FireAspectEnchantment
 *
 * 效果:
 * - 每级增加 4 秒燃烧时间
 * - 最大 II 级
 */
class FireAspectEnchantment : public Enchantment {
public:
    FireAspectEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:fire_aspect"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.fire_aspect";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Weapon; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 2; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + (level - 1) * 20; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 获取燃烧时间（tick）
     * @param level 附魔等级
     * @return 燃烧时间（tick）
     */
    [[nodiscard]] static i32 getFireDuration(i32 level)
    {
        // 每级 4 秒 = 80 tick
        return level * 80;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
