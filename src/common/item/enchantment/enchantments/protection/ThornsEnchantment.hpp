#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 荆棘附魔
 *
 * 攻击者会受到反伤。
 * 参考 MC 1.16.5 ThornsEnchantment
 *
 * 效果:
 * - 每级增加触发概率和伤害
 * - I: 15%概率, 0.5-1.5伤害
 * - II: 30%概率, 0.5-2.5伤害
 * - III: 45%概率, 0.5-3.5伤害
 * - 最大 III 级
 */
class ThornsEnchantment : public Enchantment {
public:
    ThornsEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:thorns";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.thorns";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::ArmorChest;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::VeryRare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 10 + (level - 1) * 20;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 50;
    }

    /**
     * @brief 检查是否触发荆棘效果
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return true 如果触发
     */
    [[nodiscard]] static bool shouldTrigger(i32 level, math::Random& random);

    /**
     * @brief 获取荆棘反伤
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return 反伤点数
     */
    [[nodiscard]] static i32 getThornsDamage(i32 level, math::Random& random);

    /**
     * @brief 获取触发概率
     * @param level 附魔等级
     * @return 触发概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getTriggerChance(i32 level) {
        // 每级 15%
        return static_cast<f32>(level) * 0.15f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
