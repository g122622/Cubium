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

#include "TagLootEntry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace loot {

// ============================================================================
// TagLootEntry
// ============================================================================

TagLootEntry::TagLootEntry(const std::string& tagId, bool expand, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_tagId(tagId)
    , m_expand(expand)
{}

std::unique_ptr<LootEntry> TagLootEntry::clone() const
{
    auto entry = std::make_unique<TagLootEntry>(m_tagId, m_expand, m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void TagLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<TagLootEntry*>(this));
}

bool TagLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 查找标签
    auto* tag = item::tag::ItemTags::getTag(m_tagId);
    if (!tag || tag->getItems().empty()) {
        // 标签不存在或为空，不生成物品
        return false;
    }

    const auto& items = tag->getItems();
    if (m_expand) {
        bool anyGenerated = false;
        for (const Item* item : items) {
            ItemStack stack(*item, 1);
            stack = applyFunctions(std::move(stack), context);
            if (!stack.isEmpty()) {
                consumer(stack);
                anyGenerated = true;
            }
        }
        return anyGenerated;
    }

    auto it = items.begin();
    std::advance(it, context.getRandom().nextInt(static_cast<i32>(items.size())));
    ItemStack stack(**it, 1);
    stack = applyFunctions(std::move(stack), context);
    if (!stack.isEmpty()) {
        consumer(stack);
    }
    return true;
}

} // namespace loot
} // namespace mc
