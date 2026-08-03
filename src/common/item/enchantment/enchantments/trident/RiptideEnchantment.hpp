/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 激流附魔
 *
 * 允许在水中或雨天发射三叉戟时携带玩家。
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

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    // 等级1: 17, 等级2: 24, 等级3: 31
    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + level * 7; }

    // 最大附魔成本固定为50
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
    [[nodiscard]] static f32 getFlightDistance(i32 level) noexcept
    {
        // 每级增加飞行距离
        return static_cast<f32>(level) * 5.0f + 3.0f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
