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

#include "LootEntry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <algorithm>
#include <memory>
#include <utility>

namespace mc {
namespace loot {

// ============================================================================
// LootEntry
// ============================================================================

LootEntry::~LootEntry() = default;

void LootEntry::addCondition(std::unique_ptr<LootCondition> condition)
{
    m_conditions.push_back(std::move(condition));
}

bool LootEntry::testConditions(LootContext& context) const
{
    return std::all_of(m_conditions.begin(),
        m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) { return cond && cond->test(context); });
}

void LootEntry::addFunction(std::unique_ptr<LootFunction> function)
{
    m_functions.push_back(std::move(function));
}

ItemStack LootEntry::applyFunctions(ItemStack stack, LootContext& context) const
{
    for (const auto& func : m_functions) {
        if (func && func->testConditions(context)) {
            stack = func->apply(std::move(stack), context);
            if (stack.isEmpty()) {
                break; // 函数可以返回空堆来取消掉落
            }
        }
    }
    return stack;
}

} // namespace loot
} // namespace mc
