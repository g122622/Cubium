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
 * @brief 冲击附魔
 *
 * 增加箭矢的击退距离。
 *
 * 效果:
 * - 每级增加 3 格击退距离
 * - 最大 II 级
 */
class PunchEnchantment : public Enchantment {
public:
    PunchEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:punch"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.punch";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Bow; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 2; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 12 + (level - 1) * 20; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 25; }

    /**
     * @brief 获取击退距离加成
     * @param level 附魔等级
     * @return 额外击退距离（格）
     */
    [[nodiscard]] static f32 getKnockbackBonus(i32 level)
    {
        // 每级增加 3 格击退
        return static_cast<f32>(level) * 3.0f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
