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
 * @brief 时运附魔
 *
 * 增加方块掉落物的数量。
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

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override
    {
        return 3; // Fortune I, II, III
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Digger; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与精准采集互斥
        if (other.id() == "minecraft:silk_touch") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 15 + (level - 1) * 9; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return Enchantment::getMinCost(level) + 50; }
};

} // namespace enchant
} // namespace item
} // namespace mc
