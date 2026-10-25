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
 * @brief 突刺附魔（原版 1.21 新增的 lunge，用于长矛）
 *
 * 突刺攻击命中后给使用者一个向前的冲量，并额外消耗饥饿值；贴近原版
 * `data/minecraft/enchantment/lunge.json` 的数值：
 * 权重 5（Uncommon）、手下槽位、最大 3 级、min_cost 5+8(n-1)、max_cost 25+8(n-1)；
 * 每级冲量 0.458、消耗 4 点饥饿，并额外消耗 1 点物品耐久。
 */
class LungeEnchantment : public Enchantment {
public:
    LungeEnchantment()
    {
        setAnvilCost(2);
    }

    [[nodiscard]] std::string id() const override
    {
        return "minecraft:lunge";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.lunge";
    }

    [[nodiscard]] EnchantmentType type() const override
    {
        return EnchantmentType::Weapon;
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
        return EnchantmentRarity::Uncommon;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        return 5 + (level - 1) * 8;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        return 25 + (level - 1) * 8;
    }

    /// 每级提供的水平冲量（原版 linear: base 0.458, per_level_above_first 0.458）。
    [[nodiscard]] static f32 getImpulsePerLevel(i32 level) noexcept
    {
        return 0.458f * static_cast<f32>(level);
    }

    /// 每级额外消耗的饥饿值（原版 linear: base 4.0, per_level_above_first 4.0）。
    [[nodiscard]] static f32 getExhaustionPerLevel(i32 level) noexcept
    {
        return 4.0f * static_cast<f32>(level);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc