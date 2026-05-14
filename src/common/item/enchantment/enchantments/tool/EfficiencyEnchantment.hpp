#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 效率附魔
 *
 * 增加挖掘速度。
 * 参考 MC 1.16.5 EfficiencyEnchantment
 *
 * 效果:
 * - 每级增加挖掘速度
 * - I: +2, II: +3, III: +4, IV: +5, V: +6
 * - 最大 V 级
 */
class EfficiencyEnchantment : public Enchantment {
public:
    EfficiencyEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:efficiency"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.efficiency";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Digger; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 5; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Common; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 1 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 获取挖掘速度加成
     * @param level 附魔等级
     * @return 挖掘速度加成
     */
    [[nodiscard]] static i32 getMiningSpeedBonus(i32 level)
    {
        // 公式: level^2 + 1
        // I: 2, II: 5, III: 10, IV: 17, V: 26
        return level * level + 1;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
