#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 穿透附魔
 *
 * 箭矢可以穿透实体。
 * 参考 MC 1.16.5 PiercingEnchantment
 *
 * 效果:
 * - 每级增加一个穿透目标数
 * - I: 穿透1个, II: 穿透2个, III: 穿透3个, IV: 穿透4个
 * - 最大 IV 级
 * - 与多重射击互斥
 */
class PiercingEnchantment : public Enchantment {
public:
    PiercingEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:piercing"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.piercing";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Crossbow; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 4; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Common; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 1 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return 50; }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与多重射击互斥
        if (other.id() == "minecraft:multishot") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 获取穿透目标数
     * @param level 附魔等级
     * @return 可穿透的目标数
     */
    [[nodiscard]] static i32 getPiercingCount(i32 level) { return level; }
};

} // namespace enchant
} // namespace item
} // namespace mc
