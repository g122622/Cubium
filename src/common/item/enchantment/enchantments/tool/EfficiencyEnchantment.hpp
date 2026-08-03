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
 * @brief 效率附魔
 *
 * 增加挖掘速度。
 *
 * 效果:
 * - 每级增加挖掘速度
 * - I: +2, II: +3, III: +4, IV: +5, V: +6
 * - 最大 V 级
 */
class EfficiencyEnchantment : public Enchantment {
public:
    EfficiencyEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:efficiency"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.efficiency";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Digger; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 5; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Common; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 1 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 获取挖掘速度加成
     * @param level 附魔等级
     * @return 挖掘速度加成
     */
    [[nodiscard]] static i32 getMiningSpeedBonus(i32 level)
    {
        // 公式: level^2 + 1
        // I: 2, II: 5, III: 10, IV: 17, V: 26
        return level * level + 1;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
