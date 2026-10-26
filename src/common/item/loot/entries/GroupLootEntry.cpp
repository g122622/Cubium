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

#include "GroupLootEntry.hpp"
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
// GroupLootEntry
// ============================================================================

GroupLootEntry::GroupLootEntry(std::vector<std::unique_ptr<LootEntry>> children) noexcept
    : m_children(std::move(children))
{}

std::unique_ptr<LootEntry> GroupLootEntry::clone() const
{
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<GroupLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void GroupLootEntry::addChild(std::unique_ptr<LootEntry> child)
{
    m_children.push_back(std::move(child));
}

bool GroupLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 对齐原版 `EntryGroup#compose`：所有子条目都展开，且**无论子条目成败都返回 true**
    // （空组恒真）。组的条件不满足时整体不参与（原版 `CompositeEntryBase#expand` 先 canRun）。
    if (!testConditions(context)) {
        return false;
    }

    for (const auto& child : m_children) {
        child->expand(context, consumer);
    }
    return true;
}

void GroupLootEntry::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 每个子条目都按自己的规则重新展开并生成（条件不在此重复判定）。
    for (const auto& child : m_children) {
        child->expand(context, [&context, next = consumer](const LootEntry& selected) { selected.generate(next, context); });
    }
}

} // namespace loot
} // namespace mc
