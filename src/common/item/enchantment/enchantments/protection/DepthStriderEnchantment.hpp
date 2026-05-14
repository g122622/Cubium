#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 深海探索者附魔
 *
 * 增加水下移动速度。
 * 参考 MC 1.16.5 DepthStriderEnchantment
 *
 * 效果:
 * - 每级减少水下移动惩罚
 * - I: 1/3 减免, II: 2/3 减免, III: 完全减免
 * - 最大 III 级
 */
class DepthStriderEnchantment : public Enchantment {
public:
    DepthStriderEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:depth_strider"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.depth_strider";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::ArmorFeet; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const override
    {
        return EnchantmentRarity::Rare; // MC 1.16.5: RARE
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        return getMinCost(level) + 15; // MC 1.16.5: getMinEnchantability + 15
    }

    /**
     * @brief 获取水下移动速度乘数
     * @param level 附魔等级
     * @return 速度乘数 (0.0-1.0)
     */
    [[nodiscard]] static f32 getWaterSpeedMultiplier(i32 level)
    {
        // 每级减少 1/3 的水下移动惩罚
        return static_cast<f32>(level) / 3.0f;
    }

    /**
     * @brief 检查是否与冰霜行者互斥
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
