#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 灵魂疾行附魔
 *
 * 在灵魂沙和灵魂土上增加移动速度。
 * 参考 MC 1.16.5 SoulSpeedEnchantment
 *
 * 效果:
 * - 每级增加灵魂沙/土上的移动速度
 * - I: +40%, II: +60%, III: +80%
 * - 最大 III 级
 * - 属于宝藏附魔
 */
class SoulSpeedEnchantment : public Enchantment {
public:
    SoulSpeedEnchantment() = default;

    [[nodiscard]] String id() const override {
        return "minecraft:soul_speed";
    }

    [[nodiscard]] String getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.soul_speed";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::ArmorFeet;
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

    [[nodiscard]] bool isTreasure() const override {
        return true;  // 只能从猪灵交易获得
    }

    [[nodiscard]] bool canVillagerTrade() const override {
        return false;  // MC 1.16.5: 不能通过村民交易获得
    }

    [[nodiscard]] bool canGenerateInLoot() const override {
        return false;  // MC 1.16.5: 不能在战利品表中生成
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 10 + (level - 1) * 10;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 15;
    }

    /**
     * @brief 获取灵魂沙/土上的移动速度乘数
     * @param level 附魔等级
     * @return 速度乘数
     */
    [[nodiscard]] static f32 getSoulSpeedMultiplier(i32 level) {
        // I: 1.4, II: 1.6, III: 1.8
        return 1.0f + 0.2f + static_cast<f32>(level - 1) * 0.2f;
    }

    /**
     * @brief 获取耐久消耗概率
     * @param level 附魔等级
     * @return 每tick消耗耐久的概率
     */
    [[nodiscard]] static f32 getDurabilityConsumeChance(i32 level) {
        // 每级 4% 概率消耗耐久
        return 0.04f / static_cast<f32>(level);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
