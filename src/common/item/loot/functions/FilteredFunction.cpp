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

#include "FilteredFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace loot {

FilteredFunction::FilteredFunction(advancement::ItemPredicate filter,
    std::optional<std::unique_ptr<LootFunction>> onPass,
    std::optional<std::unique_ptr<LootFunction>> onFail)
    : m_filter(std::move(filter))
    , m_onPass(std::move(onPass))
    , m_onFail(std::move(onFail))
{}

ItemStack FilteredFunction::run(ItemStack stack, LootContext& context) const
{
    // vanilla：谓词命中走 on_pass，否则走 on_fail；子函数同样先检查自身条件
    const auto& selected = m_filter.test(stack) ? m_onPass : m_onFail;
    if (!selected.has_value() || *selected == nullptr) {
        return stack;
    }
    if (!(*selected)->testConditions(context)) {
        return stack;
    }
    return (*selected)->apply(stack, context);
}

std::unique_ptr<LootFunction> FilteredFunction::clone() const noexcept
{
    auto func = std::make_unique<FilteredFunction>(m_filter,
        m_onPass.has_value() ? std::optional<std::unique_ptr<LootFunction>>((*m_onPass)->clone()) : std::nullopt,
        m_onFail.has_value() ? std::optional<std::unique_ptr<LootFunction>>((*m_onFail)->clone()) : std::nullopt);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
