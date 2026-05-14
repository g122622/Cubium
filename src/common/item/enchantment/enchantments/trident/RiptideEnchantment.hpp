#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 激流附魔
 *
 * 允许在水中或雨天发射三叉戟时携带玩家。
 * 参考 MC 1.16.5 RiptideEnchantment
 *
 * 效果:
 * - 在水中或雨天可以发射三叉戟携带玩家飞行
 * - 每级增加飞行距离
 * - 最大 III 级
 * - 与忠诚和引雷互斥
 */
class RiptideEnchantment : public Enchantment {
public:
    RiptideEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:riptide"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.riptide";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Trident; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }

    // MC 1.16.5: getMinEnchantability = 10 + enchantmentLevel * 7
    // 等级1: 17, 等级2: 24, 等级3: 31
    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + level * 7; }

    // MC 1.16.5: getMaxEnchantability = 50 (固定值)
    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与忠诚和引雷互斥
        if (other.id() == "minecraft:loyalty" || other.id() == "minecraft:channeling") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 获取飞行距离
     * @param level 附魔等级
     * @return 飞行距离（格）
     */
    [[nodiscard]] static f32 getFlightDistance(i32 level)
    {
        // 每级增加飞行距离
        return static_cast<f32>(level) * 5.0f + 3.0f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
