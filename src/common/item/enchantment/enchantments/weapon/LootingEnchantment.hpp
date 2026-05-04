#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 抢夺附魔
 *
 * 增加怪物掉落物的数量和稀有度。
 * 参考 MC 1.16.5 LootBonusEnchantment (Looting 类型)
 *
 * 效果:
 * - 抢夺效果由战利品表的 LootingEnchantBonusFunction 处理
 * - 本类只提供附魔定义和基础属性
 *
 * 适用物品:
 * - 剑
 *
 * 最大 III 级
 *
 * 注意: 实际的抢夺计算使用 loot::LootingEnchantBonusFunction 类。
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
        // MC 1.16.5: super.getMinEnchantability(level) + 50
        // 基类默认实现是 1 + (level - 1) * 10
        // 所以: 1 + (level - 1) * 10 + 50 = 51 + (level - 1) * 10
        return Enchantment::getMinCost(level) + 50;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
