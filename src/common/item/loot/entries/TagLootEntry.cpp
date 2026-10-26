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
#include <vector>
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

bool TagLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 对齐原版 `LootPoolSingletonContainer#expand`：条件通过才成为候选，
    // 并且条件只在这里判定一次（`generate` 不再重复判定）。
    if (!testConditions(context)) {
        return false;
    }
    consumer(*this);
    return true;
}

void TagLootEntry::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 条件已在 `expand` 阶段判定过，这里不再重复判定。
    auto* tag = item::tag::ItemTags::getTag(m_tagId);
    if (!tag || tag->getItems().empty()) {
        return;
    }

    // 对齐原版 `TagEntry#createItemStack`：标签里的**全部**物品逐个产出
    // （不是随机挑一个），每个都交给被条目函数装饰过的下游回调。
    const auto decorated = decorate(std::move(consumer), context);
    for (const Item* item : tag->getItems()) {
        decorated->accept(ItemStack(*item, 1));
    }
}

} // namespace loot
} // namespace mc
