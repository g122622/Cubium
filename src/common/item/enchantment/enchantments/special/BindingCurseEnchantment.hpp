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

#include "../../Enchantment.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 绑定诅咒附魔
 *
 * 防止物品被取下。
 *
 * 效果:
 * - 装备后无法取下（除非物品损坏或死亡）
 * - 只有 I 级
 * - 属于诅咒附魔
 */
class BindingCurseEnchantment : public Enchantment {
public:
    BindingCurseEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:binding_curse"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.binding_curse";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Wearable; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] bool isCurse() const noexcept override { return true; }

    [[nodiscard]] bool isTreasure() const noexcept override { return true; }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
