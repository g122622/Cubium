#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 快速装填附魔
 *
 * 减少弩的装填时间。
 * 参考 MC 1.16.5 QuickChargeEnchantment
 *
 * 效果:
 * - 每级减少 0.25 秒装填时间
 * - 基础装填时间 1.25 秒
 * - I: 1.0秒, II: 0.75秒, III: 0.5秒
 * - 最大 III 级
 */
class QuickChargeEnchantment : public Enchantment {
public:
    QuickChargeEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:quick_charge";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.quick_charge";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Crossbow;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Uncommon;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 12 + (level - 1) * 20;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 25;
    }

    /**
     * @brief 获取装填时间（tick）
     * @param level 附魔等级
     * @return 装填时间（tick）
     */
    [[nodiscard]] static i32 getChargeTime(i32 level) {
        // 基础 1.25 秒 = 25 tick, 每级减少 0.25 秒 = 5 tick
        return 25 - level * 5;
    }

    /**
     * @brief 获取装填时间（秒）
     * @param level 附魔等级
     * @return 装填时间（秒）
     */
    [[nodiscard]] static f32 getChargeTimeSeconds(i32 level) {
        return static_cast<f32>(getChargeTime(level)) / 20.0f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
