#pragma once

#include "../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 精准采集附魔
 *
 * 允许采集方块本身而不是其掉落物。
 * 参考 MC 1.16.5 SilkTouchEnchantment
 *
 * 效果:
 * - 采集矿石时掉落矿石本身而非矿物
 * - 采集玻璃时掉落玻璃而非无
 * - 采集草方块时掉落草方块而非泥土
 * - 采集树叶时掉落树叶
 *
 * 适用物品:
 * - 镐、铲、斧、锄、剪刀
 *
 * 不兼容: 时运
 */
class SilkTouchEnchantment : public Enchantment {
public:
    SilkTouchEnchantment() = default;

    // ========== Enchantment 接口实现 ==========

    [[nodiscard]] std::string id() const override {
        return "minecraft:silk_touch";
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 1;  // 精准采集只有 I 级
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Digger;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::VeryRare;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override {
        // 与时运互斥
        if (other.id() == "minecraft:fortune") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        // 参考 MC 1.16.5: 15
        (void)level;
        return 15;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        // 参考 MC 1.16.5: super.getMinEnchantability(level) + 50
        // = 1 + level * 10 + 50 = 61 (等级1)
        return 1 + level * 10 + 50;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
