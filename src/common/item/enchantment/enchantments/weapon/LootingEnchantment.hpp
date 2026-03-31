#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 抢夺附魔
 *
 * 增加怪物掉落物的数量和稀有度。
 * 参考 MC 1.16.5 LootingEnchantment
 *
 * 效果:
 * - 每级增加 1 点额外掉落概率
 * - 增加稀有掉落（如凋灵骷髅头颅）的概率
 * - 最大 III 级
 */
class LootingEnchantment : public Enchantment {
public:
    LootingEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:looting";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.looting";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Weapon;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 15 + (level - 1) * 9;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 50;
    }

    /**
     * @brief 计算额外掉落概率
     * @param level 附魔等级
     * @return 额外掉落概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getExtraDropChance(i32 level) {
        // 每级 11% 概率增加额外掉落
        return static_cast<f32>(level) * 0.11f;
    }

    /**
     * @brief 计算稀有掉落概率倍率
     * @param level 附魔等级
     * @return 稀有掉落概率倍率
     */
    [[nodiscard]] static f32 getRareDropMultiplier(i32 level) {
        // 每级增加一倍概率
        return 1.0f + static_cast<f32>(level);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
