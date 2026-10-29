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
    PowerEnchantment()
    {
        setAnvilCost(1);
    }

    [[nodiscard]] std::string id() const noexcept override
    {
        return "minecraft:power";
    }

    /**
     * @brief 每级弓箭伤害加成（原版 1.21.11 的 DAMAGE 效果组件）
     *
     * 公式 0.5 × level + 0.5 此前直接写死在箭矢的伤害计算里；这里把它放回附魔本身上，
     * 箭矢侧改为调用本函数，避免同一条公式在两处各写一遍。
     */
    [[nodiscard]] f32 getDamageBonusPerLevel(i32 level) const noexcept override
    {
        return static_cast<f32>(level) * 0.5f + 0.5f;
    }

    [[nodiscard]] std::string getNameKey(i32 level) const noexcept override
    {
        (void)level;
        return "enchantment.minecraft.power";
    }

    [[nodiscard]] EnchantmentType type() const noexcept override
    {
        return EnchantmentType::Bow;
    }

    [[nodiscard]] i32 minLevel() const noexcept override
    {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const noexcept override
    {
        return 5;
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override
    {
        return EnchantmentRarity::Common;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const noexcept override
    {
        return 1 + (level - 1) * 10;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const noexcept override
    {
        return getMinCost(level) + 15;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
