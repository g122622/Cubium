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

#include "common/item/loot/conditions/FortuneCondition.hpp"

namespace mc {
namespace loot {

FortuneCondition::FortuneCondition(i32 minLevel)
    : m_minLevel(minLevel)
{}

bool FortuneCondition::test(LootContext& context) const
{
    i32 fortuneLevel = getFortuneLevel(context);
    return fortuneLevel >= m_minLevel;
}

std::unique_ptr<LootCondition> FortuneCondition::clone() const
{
    return std::make_unique<FortuneCondition>(m_minLevel);
}

i32 FortuneCondition::getFortuneLevel(LootContext& context) noexcept
{
    // 从上下文获取时运附魔等级
    auto* fortuneLevel = context.get<i32>(LootParams::FORTUNE_LEVEL);
    if (fortuneLevel && *fortuneLevel > 0) {
        return *fortuneLevel;
    }
    return 0;
}

i32 FortuneCondition::applyFortuneBonus(i32 baseCount, i32 fortuneLevel, math::Random& random) noexcept
{
    if (fortuneLevel <= 0) {
        return baseCount;
    }

    // OreDropsFormula (乘法式):
    // int i = random.nextInt(fortune + 2) - 1;
    // if (i < 0) i = 0;
    // return baseCount * (i + 1);
    //
    // 注意：此方法已弃用，请使用 ApplyBonusFunction::calculateOreDrops()
    i32 i = random.nextInt(fortuneLevel + 2) - 1;
    if (i < 0) {
        i = 0;
    }

    return baseCount * (i + 1);
}

} // namespace loot
} // namespace mc
