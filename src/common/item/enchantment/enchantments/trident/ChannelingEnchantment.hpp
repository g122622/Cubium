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
 * @brief 引雷附魔
 *
 * 在雷暴天气中召唤闪电。
 *
 * 效果:
 * - 在雷暴天气中击中目标时召唤闪电
 * - 只有 I 级
 * - 与激流互斥
 */
class ChannelingEnchantment : public Enchantment {
public:
    ChannelingEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:channeling"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.channeling";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Trident; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 1; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        (void)level;
        return 25;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        (void)level;
        return 50;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与激流互斥
        if (other.id() == "minecraft:riptide") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
