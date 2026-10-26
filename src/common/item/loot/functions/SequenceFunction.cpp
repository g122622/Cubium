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

#include "SequenceFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace loot {

SequenceFunction::SequenceFunction(std::vector<std::unique_ptr<LootFunction>> functions)
    : m_functions(std::move(functions))
{}

ItemStack SequenceFunction::run(ItemStack stack, LootContext& context) const
{
    for (const auto& function : m_functions) {
        if (function == nullptr || !function->testConditions(context)) {
            continue;
        }
        stack = function->apply(std::move(stack), context);
        if (stack.isEmpty()) {
            return stack;
        }
    }
    return stack;
}

std::unique_ptr<LootFunction> SequenceFunction::clone() const noexcept
{
    std::vector<std::unique_ptr<LootFunction>> cloned;
    cloned.reserve(m_functions.size());
    for (const auto& function : m_functions) {
        cloned.push_back(function != nullptr ? function->clone() : nullptr);
    }
    auto func = std::make_unique<SequenceFunction>(std::move(cloned));
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
