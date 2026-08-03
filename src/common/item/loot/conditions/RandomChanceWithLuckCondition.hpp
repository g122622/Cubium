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
 * @brief 随机概率条件（受幸运值影响）
 *
 * 概率受幸运值影响的随机条件。
 * 参考: net.minecraft.loot.conditions.RandomChanceWithLooting
 *
 * 基础概率 + (幸运值 * 幸运系数)
 */
class RandomChanceWithLuckCondition : public LootCondition {
public:
    /**
     * @brief 构造条件
     * @param baseChance 基础概率
     * @param luckCoefficient 幸运系数（每点幸运增加的概率）
     */
    RandomChanceWithLuckCondition(f32 baseChance, f32 luckCoefficient);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "random_chance_with_luck"; }

    [[nodiscard]] f32 getBaseChance() const { return m_baseChance; }
    [[nodiscard]] f32 getLuckCoefficient() const { return m_luckCoefficient; }

private:
    f32 m_baseChance;
    f32 m_luckCoefficient;
};

} // namespace loot
} // namespace mc
