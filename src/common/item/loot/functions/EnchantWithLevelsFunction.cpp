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

#include "EnchantWithLevelsFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"

namespace mc {
namespace loot {

EnchantWithLevelsFunction::EnchantWithLevelsFunction(const RandomValueRange& levels, bool treasure)
    : m_levels(levels)
    , m_treasure(treasure)
{}

ItemStack EnchantWithLevelsFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // MC 1.16.5: EnchantWithLevels.doApply
    // 生成随机等级并添加随机附魔
    i32 level = m_levels.generateInt(context.getRandom());
    return item::enchant::EnchantmentHelper::addRandomEnchantment(
        context.getRandom(), std::move(stack), level, m_treasure);
}

std::unique_ptr<LootFunction> EnchantWithLevelsFunction::clone() const
{
    auto func = std::make_unique<EnchantWithLevelsFunction>(m_levels, m_treasure);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
