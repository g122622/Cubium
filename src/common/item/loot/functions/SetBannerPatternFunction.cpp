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

#include "SetBannerPatternFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>

namespace mc {
namespace loot {

SetBannerPatternFunction::SetBannerPatternFunction(nlohmann::json patterns, bool append)
    : m_patterns(std::move(patterns))
    , m_append(append)
{}

ItemStack SetBannerPatternFunction::apply(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);
    if (stack.isEmpty() || !m_patterns.is_array()) {
        return stack;
    }
    auto& patterns = stack.getOrCreateTag()["BlockEntityTag"]["Patterns"];
    if (!patterns.is_array() || !m_append) {
        patterns = nlohmann::json::array();
    }
    for (const auto& pattern : m_patterns) {
        patterns.push_back(pattern);
    }
    return stack;
}

std::unique_ptr<LootFunction> SetBannerPatternFunction::clone() const noexcept
{
    auto func = std::make_unique<SetBannerPatternFunction>(m_patterns, m_append);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
