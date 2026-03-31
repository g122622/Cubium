#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 耐久附魔
 *
 * 减少物品耐久度消耗的概率。
 * 参考 MC 1.16.5 UnbreakingEnchantment
 *
 * 效果:
 * - 每级有 (level + 1) / (level + 50) 的概率不消耗耐久
 * - 对于盔甲，概率为 (level + 1) / (level + 100)
 * - 最大 III 级
 */
class UnbreakingEnchantment : public Enchantment {
public:
    UnbreakingEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:unbreaking";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.unbreaking";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Breakable;
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
        return 5 + (level - 1) * 8;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 50;
    }

    /**
     * @brief 计算耐久保护的物品是否应该消耗耐久
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return true 如果应该消耗耐久，false 如果不消耗
     */
    [[nodiscard]] static bool shouldConsumeDurability(i32 level, math::Random& random);

    /**
     * @brief 计算盔甲的耐久保护
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return true 如果应该消耗耐久，false 如果不消耗
     */
    [[nodiscard]] static bool shouldArmorConsumeDurability(i32 level, math::Random& random);

    /**
     * @brief 获取耐久保护概率（非盔甲）
     * @param level 附魔等级
     * @return 不消耗耐久的概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getDurabilityProtectionChance(i32 level) {
        // 公式: 1 - (level + 1) / (level + 50)
        // I: ~4%, II: ~6%, III: ~7.5%
        return 1.0f - (static_cast<f32>(level + 1) / static_cast<f32>(level + 50));
    }

    /**
     * @brief 获取盔甲耐久保护概率
     * @param level 附魔等级
     * @return 不消耗耐久的概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getArmorDurabilityProtectionChance(i32 level) {
        // 公式: 1 - (level + 1) / (level + 100)
        // I: ~1%, II: ~2%, III: ~3%
        return 1.0f - (static_cast<f32>(level + 1) / static_cast<f32>(level + 100));
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
