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

#include "common/item/loot/conditions/OrCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

OrCondition::OrCondition(std::vector<std::unique_ptr<LootCondition>> conditions)
    : m_conditions(std::move(conditions))
{}

bool OrCondition::test(LootContext& context) const
{
    // 任一子条件满足即返回 true
    return std::any_of(m_conditions.begin(),
        m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) { return cond && cond->test(context); });
}

std::unique_ptr<LootCondition> OrCondition::clone() const noexcept
{
    // 深拷贝所有子条件
    std::vector<std::unique_ptr<LootCondition>> cloned;
    cloned.reserve(m_conditions.size());
    for (const auto& cond : m_conditions) {
        if (cond) {
            cloned.push_back(cond->clone());
        }
    }
    return std::make_unique<OrCondition>(std::move(cloned));
}

void OrCondition::addCondition(std::unique_ptr<LootCondition> condition)
{
    m_conditions.push_back(std::move(condition));
}

} // namespace loot
} // namespace mc
