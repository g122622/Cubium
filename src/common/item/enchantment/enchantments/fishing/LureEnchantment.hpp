#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 饵钓附魔
 *
 * 减少鱼上钩的等待时间。
 * 参考 MC 1.16.5 LureEnchantment
 *
 * 效果:
 * - 每级减少 5 秒等待时间
 * - 最大 III 级
 */
class LureEnchantment : public Enchantment {
public:
    LureEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:lure"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.lure";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::FishingRod; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 15 + (level - 1) * 9; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 获取等待时间减少（tick）
     * @param level 附魔等级
     * @return 减少的等待时间（tick）
     */
    [[nodiscard]] static i32 getWaitTimeReduction(i32 level)
    {
        // 每级减少 5 秒 = 100 tick
        return level * 100;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
