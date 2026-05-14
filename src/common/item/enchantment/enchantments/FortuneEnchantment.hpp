#pragma once

#include "../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 时运附魔
 *
 * 增加方块掉落物的数量。
 * 参考 MC 1.16.5 LootBonusEnchantment (Fortune 类型)
 *
 * 效果:
 * - 时运效果由战利品表的 ApplyBonusFunction 处理
 * - 本类只提供附魔定义和基础属性
 *
 * 适用物品:
 * - 镐、铲、斧、锄
 *
 * 不兼容: 精准采集
 *
 * 注意: 实际的时运计算使用 loot::ApplyBonusFunction 类，
 * 该类实现了三种时运公式：OreDrops、Uniform、Binomial。
 */
class FortuneEnchantment : public Enchantment {
public:
    FortuneEnchantment() = default;

    // ========== Enchantment 接口实现 ==========

    [[nodiscard]] std::string id() const override { return "minecraft:fortune"; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override
    {
        return 3; // Fortune I, II, III
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Digger; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与精准采集互斥
        if (other.id() == "minecraft:silk_touch") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        // 参考 MC 1.16.5: 15 + (level - 1) * 9
        return 15 + (level - 1) * 9;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        // MC 1.16.5: super.getMinEnchantability(level) + 50
        // 基类默认实现是 1 + (level - 1) * 10
        // 所以: 1 + (level - 1) * 10 + 50 = 51 + (level - 1) * 10
        return Enchantment::getMinCost(level) + 50;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
