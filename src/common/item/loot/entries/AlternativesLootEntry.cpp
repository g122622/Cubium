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

#include "AlternativesLootEntry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

// ============================================================================
// AlternativesLootEntry
// ============================================================================

AlternativesLootEntry::AlternativesLootEntry(std::vector<std::unique_ptr<LootEntry>> children)
    : m_children(std::move(children))
{}

std::unique_ptr<LootEntry> AlternativesLootEntry::clone() const
{
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<AlternativesLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void AlternativesLootEntry::addChild(std::unique_ptr<LootEntry> child)
{
    m_children.push_back(std::move(child));
}

void AlternativesLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<AlternativesLootEntry*>(this));
}

bool AlternativesLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 尝试每个子条目，直到一个成功
    for (const auto& child : m_children) {
        if (child->generate(consumer, context)) {
            return true;
        }
    }
    return false;
}

} // namespace loot
} // namespace mc
