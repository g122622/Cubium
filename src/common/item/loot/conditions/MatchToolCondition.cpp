/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "common/item/loot/conditions/MatchToolCondition.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include <memory>
#include <optional>
#include <utility>

namespace mc {
namespace loot {

MatchToolCondition::MatchToolCondition()
    : m_predicate(std::nullopt)
{}

MatchToolCondition::MatchToolCondition(advancement::ItemPredicate predicate)
    : m_predicate(std::move(predicate))
{}

bool MatchToolCondition::test(LootContext& context) const
{
    // 从上下文获取工具参数
    auto* tool = context.get<ItemStack>(LootParams::TOOL);
    if (!tool || tool->isEmpty()) {
        // 无工具时不满足条件
        return false;
    }

    // 无谓词时，只要存在非空工具即满足条件
    // 对应 MC Java: MatchTool.test() 中 predicate.isEmpty() 时返回 true
    if (!m_predicate.has_value()) {
        return true;
    }

    // 使用 ItemPredicate 进行完整匹配
    return m_predicate->test(*tool);
}

std::unique_ptr<LootCondition> MatchToolCondition::clone() const noexcept
{
    if (m_predicate.has_value()) {
        return std::make_unique<MatchToolCondition>(m_predicate.value());
    }
    return std::make_unique<MatchToolCondition>();
}

} // namespace loot
} // namespace mc
