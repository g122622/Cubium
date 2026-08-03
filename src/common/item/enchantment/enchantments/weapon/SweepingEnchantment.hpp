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
 * @brief 横扫之刃附魔
 *
 * 增加横扫攻击的伤害。
 *
 * 效果:
 * - I: 50% 伤害传递
 * - II: 67% 伤害传递
 * - III: 75% 伤害传递
 * - 最大 III 级
 */
class SweepingEnchantment : public Enchantment {
public:
    SweepingEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:sweeping"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.sweeping";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Weapon; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 5 + (level - 1) * 9; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 15; }

    /**
     * @brief 获取横扫伤害比例
     * @param level 附魔等级
     * @return 伤害传递比例 (0.0-1.0)
     */
    [[nodiscard]] static f32 getSweepingDamageRatio(i32 level) noexcept
    {
        // I: 50%, II: 67%, III: 75%
        // 公式: 1 / (level + 1) 的补数
        return 1.0f - (1.0f / static_cast<f32>(level + 1));
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
