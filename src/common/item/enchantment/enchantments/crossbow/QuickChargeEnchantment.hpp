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
 * @brief 快速装填附魔
 *
 * 减少弩的装填时间。
 *
 * 效果:
 * - 每级减少 0.25 秒装填时间
 * - 基础装填时间 1.25 秒
 * - I: 1.0秒, II: 0.75秒, III: 0.5秒
 * - 最大 III 级
 */
class QuickChargeEnchantment : public Enchantment {
public:
    QuickChargeEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:quick_charge"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.quick_charge";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Crossbow; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Uncommon; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 12 + (level - 1) * 20; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }

    /**
     * @brief 获取装填时间（tick）
     * @param level 附魔等级
     * @return 装填时间（tick）
     */
    [[nodiscard]] static i32 getChargeTime(i32 level)
    {
        // 基础 1.25 秒 = 25 tick, 每级减少 0.25 秒 = 5 tick
        return 25 - level * 5;
    }

    /**
     * @brief 获取装填时间（秒）
     * @param level 附魔等级
     * @return 装填时间（秒）
     */
    [[nodiscard]] static f32 getChargeTimeSeconds(i32 level) { return static_cast<f32>(getChargeTime(level)) / 20.0f; }
};

} // namespace enchant
} // namespace item
} // namespace mc
