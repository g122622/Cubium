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

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 消失诅咒附魔
 *
 * 死亡时物品消失。
 * 参考 MC 1.16.5 VanishingCurseEnchantment
 *
 * 效果:
 * - 玩家死亡时物品消失而不是掉落
 * - 只有 I 级
 * - 属于诅咒附魔
 */
class VanishingCurseEnchantment : public Enchantment {
public:
    VanishingCurseEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:vanishing_curse"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.vanishing_curse";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Vanishable; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] bool isCurse() const override { return true; }

    [[nodiscard]] bool isTreasure() const override { return true; }

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
