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
#include "common/util/math/random/Random.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 耐久附魔
 *
 * 减少物品耐久度消耗的概率。
 *
 * 本类只承载附魔元数据（id/等级/稀有度/花费）。耐久损耗的随机判定由
 * ItemStack::attemptDamageItem → EnchantmentHelper::shouldIgnoreDurabilityLoss 唯一承载：
 * 每次损耗点对 level + 1 取随机整数，非零即忽略该点（概率 level / (level + 1)）；
 * 盔甲额外先以 60% 概率跳过保护。
 */
class UnbreakingEnchantment : public Enchantment {
public:
    UnbreakingEnchantment() noexcept = default;

    [[nodiscard]] std::string id() const override { return "minecraft:unbreaking"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.unbreaking";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Breakable; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Uncommon; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 5 + (level - 1) * 8; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 获取耐久保护概率（非盔甲）
     * @param level 附魔等级
     * @return 不消耗耐久的概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getDurabilityProtectionChance(i32 level)
    {
        // I: 1/2 = 50%, II: 2/3 = 67%, III: 3/4 = 75%
        return static_cast<f32>(level) / static_cast<f32>(level + 1);
    }

    /**
     * @brief 获取盔甲耐久保护概率
     * @param level 附魔等级
     * @return 不消耗耐久的概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getArmorDurabilityProtectionChance(i32 level)
    {
        // 盔甲有 60% 概率忽略耐久保护
        // I: 0.4 * 50% = 20%, II: 0.4 * 67% = 27%, III: 0.4 * 75% = 30%
        return 0.4f * static_cast<f32>(level) / static_cast<f32>(level + 1);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
