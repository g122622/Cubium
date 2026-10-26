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
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 随机概率条件（按攻击者附魔等级提升概率）
 *
 * 对齐原版 `LootItemRandomChanceWithEnchantedBonusCondition`：
 * `minecraft:random_chance_with_enchanted_bonus`。
 *
 * 读数路径与原版一致：从上下文的 `ATTACKING_ENTITY` 读取实体的附魔等级。
 * 等级为 0 时用 `unenchanted_chance`，否则用 `enchanted_chance` 的
 * `LevelBasedValue.Linear` 求值（`base + per_level_above_first * (level - 1)`）。
 */
class RandomChanceWithEnchantedBonusCondition : public LootCondition {
public:
    /**
     * @brief 构造条件
     * @param unenchantedChance 无附魔时的概率
     * @param enchantedBase 有附魔时的基础概率（等级 1）
     * @param perLevelAboveFirst 每高一级增加的概率
     * @param enchantmentId 参与计算的附魔 ID
     */
    RandomChanceWithEnchantedBonusCondition(
        f32 unenchantedChance, f32 enchantedBase, f32 perLevelAboveFirst, std::string enchantmentId);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;

    [[nodiscard]] std::string getType() const override
    {
        return "random_chance_with_enchanted_bonus";
    }

    [[nodiscard]] f32 getUnenchantedChance() const
    {
        return m_unenchantedChance;
    }

    [[nodiscard]] f32 getEnchantedBase() const
    {
        return m_enchantedBase;
    }

    [[nodiscard]] f32 getPerLevelAboveFirst() const
    {
        return m_perLevelAboveFirst;
    }

    [[nodiscard]] const std::string& getEnchantmentId() const
    {
        return m_enchantmentId;
    }

private:
    f32 m_unenchantedChance;
    f32 m_enchantedBase;
    f32 m_perLevelAboveFirst;
    std::string m_enchantmentId;
};

} // namespace loot
} // namespace mc