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

#include "../Enchantment.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 精准采集附魔
 *
 * 允许采集方块本身而不是其掉落物。这是一个单级附魔，只能附魔到 I 级。
 *
 * 效果：
 * - 采集矿石时掉落矿石本身而非矿物
 * - 采集玻璃时掉落玻璃而非无
 * - 采集草方块时掉落草方块而非泥土
 * - 采集树叶时掉落树叶
 * - 采集书架时掉落书架而非书
 * - 采集萤石时掉落萤石而非萤石粉
 *
 * 适用物品：
 * - 镐、铲、斧、锄、剪刀
 *
 * 不兼容附魔：时运（Fortune）
 *
 * 稀有度：非常稀有（VeryRare）
 */
class SilkTouchEnchantment : public Enchantment {
public:
    SilkTouchEnchantment() = default;

    // ========== Enchantment 接口实现 ==========

    [[nodiscard]] std::string id() const override { return "minecraft:silk_touch"; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override
    {
        return 1; // 精准采集只有 I 级
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Digger; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与时运互斥
        if (other.id() == "minecraft:fortune") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 15;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return 1 + level * 10 + 50; }
};

} // namespace enchant
} // namespace item
} // namespace mc
