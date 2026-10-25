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
 * @brief 迅捷潜行附魔（原版 1.21 的 swift_sneak）
 *
 * 提高潜行移动速度：每级 +15%，叠乘在潜行基础速度上。
 * 数据来自原版 `data/minecraft/enchantment/swift_sneak.json`：
 * 权重 1（VeryRare）、护腿槽位、最大 3 级、min_cost 25+25(n-1)、max_cost 75+25(n-1)。
 */
class SwiftSneakEnchantment : public Enchantment {
public:
    SwiftSneakEnchantment()
    {
        setAnvilCost(8);
    }

    [[nodiscard]] std::string id() const override
    {
        return "minecraft:swift_sneak";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.swift_sneak";
    }

    [[nodiscard]] EnchantmentType type() const override
    {
        return EnchantmentType::ArmorLegs;
    }

    [[nodiscard]] i32 minLevel() const noexcept override
    {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const noexcept override
    {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override
    {
        return EnchantmentRarity::VeryRare;
    }

    [[nodiscard]] bool isTreasure() const noexcept override
    {
        return true;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        return 25 + (level - 1) * 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        return 75 + (level - 1) * 25;
    }

    /// 每级提供的潜行速度加成（原版 linear: base 0.15, per_level_above_first 0.15）。
    [[nodiscard]] static f32 getSneakingSpeedBonus(i32 level) noexcept
    {
        return 0.15f * static_cast<f32>(level);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc