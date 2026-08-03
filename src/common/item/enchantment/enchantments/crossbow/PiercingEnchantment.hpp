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
 * @brief 穿透附魔
 *
 * 箭矢可以穿透实体。
 *
 * 效果:
 * - 每级增加一个穿透目标数
 * - I: 穿透1个, II: 穿透2个, III: 穿透3个, IV: 穿透4个
 * - 最大 IV 级
 * - 与多重射击互斥
 */
class PiercingEnchantment : public Enchantment {
public:
    PiercingEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:piercing"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.piercing";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Crossbow; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 4; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Common; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 1 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return 50; }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与多重射击互斥
        if (other.id() == "minecraft:multishot") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 获取穿透目标数
     * @param level 附魔等级
     * @return 可穿透的目标数
     */
    [[nodiscard]] static i32 getPiercingCount(i32 level) { return level; }
};

} // namespace enchant
} // namespace item
} // namespace mc
