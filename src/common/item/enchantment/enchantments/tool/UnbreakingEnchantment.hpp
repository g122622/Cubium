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
 * 效果:
 * - 每级有 level / (level + 1) 的概率不消耗耐久
 *   I: 50%, II: 67%, III: 75%
 * - 对于盔甲，有60%概率忽略耐久保护，实际保护概率为 0.4 * level / (level + 1)
 *   I: 20%, II: 27%, III: 30%
 * - 最大 III 级
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
     * @brief 计算耐久保护的物品是否应该消耗耐久
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return true 如果应该消耗耐久，false 如果不消耗
     */
    [[nodiscard]] static bool shouldConsumeDurability(i32 level, math::Random& random);

    /**
     * @brief 计算盔甲的耐久保护
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return true 如果应该消耗耐久，false 如果不消耗
     */
    [[nodiscard]] static bool shouldArmorConsumeDurability(i32 level, math::Random& random);

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
