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
 * @brief 力量附魔
 *
 * 增加弓箭的伤害。
 *
 * 效果:
 * - I: +25% 伤害, II: +50%, III: +75%, IV: +100%, V: +150%
 * - 最大 V 级
 */
class PowerEnchantment : public Enchantment {
public:
    PowerEnchantment() = default;

    [[nodiscard]] std::string id() const noexcept override { return "minecraft:power"; }

    [[nodiscard]] std::string getNameKey(i32 level) const noexcept override
    {
        (void)level;
        return "enchantment.minecraft.power";
    }

    [[nodiscard]] EnchantmentType type() const noexcept override { return EnchantmentType::Bow; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 5; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Common; }

    [[nodiscard]] i32 getMinCost(i32 level) const noexcept override { return 1 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const noexcept override { return getMinCost(level) + 15; }

    /**
     * @brief 获取箭矢伤害加成
     * @param level 附魔等级 (0 表示无附魔)
     * @return 伤害加成值 (0.0 = 无加成)
     *
     * 公式: 0.25 * (level + 1)
     * - I: 0.5 (箭矢伤害 +50%)
     * - II: 0.75 (箭矢伤害 +75%)
     * - III: 1.0 (箭矢伤害 +100%)
     * - IV: 1.25 (箭矢伤害 +125%)
     * - V: 1.5 (箭矢伤害 +150%)
     */
    [[nodiscard]] static f32 getArrowDamageBonus(i32 level) noexcept
    {
        if (level <= 0) return 0.0f;
        return 0.25f * static_cast<f32>(level + 1);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
