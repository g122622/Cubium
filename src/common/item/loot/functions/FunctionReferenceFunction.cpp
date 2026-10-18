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

#include "FunctionReferenceFunction.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <memory>
#include <utility>

#include "common/item/core/ItemStack.hpp"

namespace mc {
namespace loot {

FunctionReferenceFunction::FunctionReferenceFunction(std::string name, const LootFunction* target)
    : m_name(std::move(name))
    , m_target(target)
{}

ItemStack FunctionReferenceFunction::apply(ItemStack stack, LootContext& context) const
{
    // 目标函数自身的条件由被引用函数在注册时保留，这里直接应用（与 vanilla 一致）
    return m_target->apply(std::move(stack), context);
}

std::unique_ptr<LootFunction> FunctionReferenceFunction::clone() const noexcept
{
    auto func = std::make_unique<FunctionReferenceFunction>(m_name, m_target);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
