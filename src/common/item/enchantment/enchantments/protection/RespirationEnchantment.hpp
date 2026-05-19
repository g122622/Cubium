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
 * @brief 水下呼吸附魔
 *
 * 延长水下呼吸时间。
 * 参考 MC 1.16.5 RespirationEnchantment
 *
 * 效果:
 * - 每级延长 15 秒呼吸时间
 * - 增加溺水伤害间隔
 * - 最大 III 级
 */
class RespirationEnchantment : public Enchantment {
public:
    RespirationEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:respiration"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.respiration";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::ArmorHead; }

    [[nodiscard]] i32 minLevel() const override { return 1; }

    [[nodiscard]] i32 maxLevel() const override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        return 10 * level; // MC 1.16.5: 10 * level
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        return getMinCost(level) + 30; // MC 1.16.5: getMinEnchantability + 30
    }

    /**
     * @brief 获取额外呼吸时间（tick）
     * @param level 附魔等级
     * @return 额外呼吸时间（tick）
     */
    [[nodiscard]] static i32 getExtraAirTicks(i32 level)
    {
        // 每级延长 15 秒 = 300 tick
        return level * 300;
    }

    /**
     * @brief 获取总呼吸时间（tick）
     * @param level 附魔等级
     * @return 总呼吸时间（tick）
     */
    [[nodiscard]] static i32 getTotalAirTicks(i32 level)
    {
        // 基础 300 tick + 每级 300 tick
        return 300 + getExtraAirTicks(level);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
