#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 多重射击附魔
 *
 * 一次射出三支箭。
 * 参考 MC 1.16.5 MultishotEnchantment
 *
 * 效果:
 * - 一次射出三支箭（中间一支正常，左右各一支偏移）
 * - 只消耗一支箭
 * - 只有 I 级
 * - 与穿透互斥
 */
class MultishotEnchantment : public Enchantment {
public:
    MultishotEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:multishot"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.multishot";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Crossbow; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 20;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与穿透互斥
        if (other.id() == "minecraft:piercing") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 获取箭矢数量
     * @return 箭矢数量（固定为3）
     */
    [[nodiscard]] static i32 getArrowCount() { return 3; }
};

} // namespace enchant
} // namespace item
} // namespace mc
