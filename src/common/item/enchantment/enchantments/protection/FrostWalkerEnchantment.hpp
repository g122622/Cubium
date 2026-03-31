#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 冰霜行者附魔
 *
 * 在水面上行走时创建霜冰。
 * 参考 MC 1.16.5 FrostWalkerEnchantment
 *
 * 效果:
 * - 每级增加冰霜范围
 * - I: 半径 2 格, II: 半径 3 格
 * - 最大 II 级
 * - 与深海探索者互斥
 */
class FrostWalkerEnchantment : public Enchantment {
public:
    FrostWalkerEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:frost_walker";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.frost_walker";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::ArmorFeet;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 2;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] bool isTreasure() const override {
        return true;  // 只能从箱子或交易获得
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 10 + (level - 1) * 10;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 40;
    }

    /**
     * @brief 获取冰霜半径
     * @param level 附魔等级
     * @return 半径（格）
     */
    [[nodiscard]] static i32 getFrostRadius(i32 level) {
        // 每级半径 +1
        return level + 1;
    }

    /**
     * @brief 检查是否与深海探索者互斥
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
