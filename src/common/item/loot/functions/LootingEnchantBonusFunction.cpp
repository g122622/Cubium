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

#include "LootingEnchantBonusFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>

namespace mc {
namespace loot {

LootingEnchantBonusFunction::LootingEnchantBonusFunction(const RandomValueRange& count, i32 limit)
    : m_count(count)
    , m_limit(limit)
{}

ItemStack LootingEnchantBonusFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 获取掠夺附魔等级
    i32 lootingLevel = context.getLootingModifier();
    if (lootingLevel <= 0) {
        return stack;
    }

    // 计算掠夺附魔加成
    f32 bonus = static_cast<f32>(lootingLevel) * m_count.generateFloat(context.getRandom());
    stack.grow(math::roundTo<i32>(bonus));

    // 检查限制
    if (m_limit > 0 && stack.getCount() > m_limit) {
        stack.setCount(m_limit);
    }

    return stack;
}

std::unique_ptr<LootFunction> LootingEnchantBonusFunction::clone() const noexcept
{
    auto func = std::make_unique<LootingEnchantBonusFunction>(m_count, m_limit);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
