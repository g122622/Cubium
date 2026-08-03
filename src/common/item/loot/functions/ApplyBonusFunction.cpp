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

#include "ApplyBonusFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>

namespace mc {
namespace loot {

ApplyBonusFunction::ApplyBonusFunction(BonusType bonusType, i32 bonusMultiplier, i32 extra, f32 probability)
    : m_bonusType(bonusType)
    , m_bonusMultiplier(bonusMultiplier)
    , m_extra(extra)
    , m_probability(probability)
{}

ItemStack ApplyBonusFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 从上下文获取工具
    auto* tool = context.get<ItemStack>(LootParams::TOOL);
    if (!tool || tool->isEmpty()) {
        return stack;
    }

    // 获取时运等级
    i32 fortuneLevel = 0;
    auto* fortuneParam = context.get<i32>(LootParams::FORTUNE_LEVEL);
    if (fortuneParam) {
        fortuneLevel = *fortuneParam;
    } else {
        // 尝试从工具获取时运等级
        fortuneLevel = item::enchant::EnchantmentHelper::getFortuneLevel(*tool);
    }

    // 根据加成类型计算新数量
    i32 baseCount = stack.getCount();
    i32 newCount = baseCount;
    switch (m_bonusType) {
        case BonusType::OreDrops:
            newCount = calculateOreDrops(baseCount, fortuneLevel, context.getRandom());
            break;
        case BonusType::Uniform:
            newCount = calculateUniformBonus(baseCount, fortuneLevel, m_bonusMultiplier, context.getRandom());
            break;
        case BonusType::Binomial:
            newCount = calculateBinomialBonus(baseCount, fortuneLevel, m_extra, m_probability, context.getRandom());
            break;
    }

    stack.setCount(newCount);
    return stack;
}

std::unique_ptr<LootFunction> ApplyBonusFunction::clone() const noexcept
{
    auto func = std::make_unique<ApplyBonusFunction>(m_bonusType, m_bonusMultiplier, m_extra, m_probability);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

i32 ApplyBonusFunction::calculateOreDrops(i32 baseCount, i32 fortuneLevel, math::Random& random)
{
    // MC 1.16.5 OreDropsFormula:
    // if (fortune > 0) {
    //     int i = random.nextInt(fortune + 2) - 1;
    //     if (i < 0) i = 0;
    //     return baseCount * (i + 1);
    // } else {
    //     return baseCount;
    // }
    if (fortuneLevel <= 0) {
        return baseCount;
    }

    // random.nextInt(fortune + 2) - 1
    // fortune=1: random.nextInt(3) - 1 -> -1, 0, 1 (修正后 0, 0, 1) -> multiplier: 1, 1, 2
    // fortune=2: random.nextInt(4) - 1 -> -1, 0, 1, 2 (修正后 0, 0, 1, 2) -> multiplier: 1, 1, 2, 3
    // fortune=3: random.nextInt(5) - 1 -> -1, 0, 1, 2, 3 (修正后 0, 0, 1, 2, 3) -> multiplier: 1, 1, 2, 3, 4
    i32 i = random.nextInt(fortuneLevel + 2) - 1;
    if (i < 0) {
        i = 0;
    }

    return baseCount * (i + 1);
}

i32 ApplyBonusFunction::calculateUniformBonus(
    i32 baseCount, i32 fortuneLevel, i32 bonusMultiplier, math::Random& random)
{
    // MC 1.16.5 UniformBonusCountFormula:
    // count + random.nextInt(bonusMultiplier * fortune + 1)
    if (fortuneLevel <= 0) {
        return baseCount;
    }

    i32 bonus = random.nextInt(bonusMultiplier * fortuneLevel + 1);
    return baseCount + bonus;
}

i32 ApplyBonusFunction::calculateBinomialBonus(
    i32 baseCount, i32 fortuneLevel, i32 extra, f32 probability, math::Random& random)
{
    // MC 1.16.5 BinomialWithBonusCountFormula:
    // for (int i = 0; i < fortune + extra; ++i) {
    //     if (random.nextFloat() < probability) {
    //         ++count;
    //     }
    // }
    i32 trials = fortuneLevel + extra;
    i32 result = baseCount;

    for (i32 i = 0; i < trials; ++i) {
        if (random.nextFloat() < probability) {
            ++result;
        }
    }

    return result;
}

} // namespace loot
} // namespace mc
