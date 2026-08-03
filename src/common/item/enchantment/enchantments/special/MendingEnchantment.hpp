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
 * @brief 经验修补附魔
 *
 * 用经验球修复物品耐久。
 *
 * 效果:
 * - 拾取经验球时修复物品（每2点经验修复1点耐久）
 * - 只有 I 级
 * - 属于宝藏附魔
 * - 与无限互斥
 */
class MendingEnchantment : public Enchantment {
public:
    MendingEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:mending"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.mending";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Breakable; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] bool isTreasure() const noexcept override
    {
        return true; // 只能从钓鱼、交易或宝箱获得
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 75;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与无限互斥
        if (other.id() == "minecraft:infinity") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 计算经验修复的耐久值
     * @param xp 经验值
     * @return 修复的耐久值
     */
    [[nodiscard]] static i32 calculateDurabilityRepair(i32 xp)
    {
        // 每2点经验修复1点耐久
        return xp / 2;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
