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
 * @brief 海之眷顾附魔
 *
 * 增加钓鱼获得宝藏的概率。
 *
 * 效果:
 * - 每级增加宝藏概率，减少垃圾概率
 * - 最大 III 级
 */
class LuckOfTheSeaEnchantment : public Enchantment {
public:
    LuckOfTheSeaEnchantment() noexcept = default;

    [[nodiscard]] std::string id() const override { return "minecraft:luck_of_the_sea"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.luck_of_the_sea";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::FishingRod; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 15 + (level - 1) * 9; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 获取宝藏概率加成
     * @param level 附魔等级
     * @return 概率加成 (0.0-1.0)
     */
    [[nodiscard]] static f32 getTreasureBonus(i32 level)
    {
        // 基础概率 0.05, 每级 +0.02
        return static_cast<f32>(level) * 0.02f;
    }

    /**
     * @brief 获取垃圾概率减少
     * @param level 附魔等级
     * @return 概率减少 (0.0-1.0)
     */
    [[nodiscard]] static f32 getJunkReduction(i32 level)
    {
        // 基础概率 0.10, 每级 -0.02
        return static_cast<f32>(level) * 0.02f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
