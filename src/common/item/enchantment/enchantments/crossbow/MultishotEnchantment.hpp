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
 * @brief 多重射击附魔
 *
 * 一次射出三支箭。
 *
 * 效果:
 * - 一次射出三支箭（中间一支正常，左右各一支偏移）
 * - 只消耗一支箭
 * - 只有 I 级
 * - 与穿透互斥
 */
class MultishotEnchantment : public Enchantment {
public:
    MultishotEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:multishot"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.multishot";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Crossbow; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 20;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与穿透互斥
        if (other.id() == "minecraft:piercing") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 获取箭矢数量
     * @return 箭矢数量（固定为3）
     */
    [[nodiscard]] static i32 getArrowCount() { return 3; }
};

} // namespace enchant
} // namespace item
} // namespace mc
