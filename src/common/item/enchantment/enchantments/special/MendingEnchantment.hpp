#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 经验修补附魔
 *
 * 用经验球修复物品耐久。
 * 参考 MC 1.16.5 MendingEnchantment
 *
 * 效果:
 * - 拾取经验球时修复物品（每2点经验修复1点耐久）
 * - 只有 I 级
 * - 属于宝藏附魔
 * - 与无限互斥
 */
class MendingEnchantment : public Enchantment {
public:
    MendingEnchantment() = default;

    [[nodiscard]] std::string id() const override {
        return "minecraft:mending";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override {
        (void)level;
        return "enchantment.minecraft.mending";
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Breakable;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 1;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] bool isTreasure() const override {
        return true;  // 只能从钓鱼、交易或宝箱获得
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        (void)level;
        return 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        (void)level;
        return 75;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override {
        // 与无限互斥
        if (other.id() == "minecraft:infinity") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 计算经验修复的耐久值
     * @param xp 经验值
     * @return 修复的耐久值
     */
    [[nodiscard]] static i32 calculateDurabilityRepair(i32 xp) {
        // 每2点经验修复1点耐久
        return xp / 2;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
