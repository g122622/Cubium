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
 * @brief 深海探索者附魔
 *
 * 增加水下移动速度。
 *
 * 效果:
 * - 每级减少水下移动惩罚
 * - I: 1/3 减免, II: 2/3 减免, III: 完全减免
 * - 最大 III 级
 */
class DepthStriderEnchantment : public Enchantment {
public:
    DepthStriderEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:depth_strider"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.depth_strider";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::ArmorFeet; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const noexcept override { return 10 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const noexcept override { return getMinCost(level) + 15; }

    /**
     * @brief 获取水下移动速度乘数
     * @param level 附魔等级
     * @return 速度乘数 (0.0-1.0)
     */
    [[nodiscard]] static f32 getWaterSpeedMultiplier(i32 level) noexcept
    {
        // 每级减少 1/3 的水下移动惩罚
        return static_cast<f32>(level) / 3.0f;
    }

    /**
     * @brief 检查是否与冰霜行者互斥
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
