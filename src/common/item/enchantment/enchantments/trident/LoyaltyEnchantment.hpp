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
 * @brief 忠诚附魔
 *
 * 三叉戟投掷后自动返回。
 *
 * 效果:
 * - 每级减少返回时间
 * - 最大 III 级
 * - 在末地无效
 */
class LoyaltyEnchantment : public Enchantment {
public:
    LoyaltyEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:loyalty"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.loyalty";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Trident; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Uncommon; }

    // 等级1: 12, 等级2: 19, 等级3: 26
    [[nodiscard]] i32 getMinCost(i32 level) const override { return 5 + level * 7; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }

    /**
     * @brief 获取返回速度
     * @param level 附魔等级
     * @return 返回速度乘数
     */
    [[nodiscard]] static f32 getReturnSpeed(i32 level)
    {
        // 每级增加返回速度
        return 0.5f + static_cast<f32>(level) * 0.5f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
