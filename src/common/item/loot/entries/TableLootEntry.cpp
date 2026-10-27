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

#include "TableLootEntry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

// ============================================================================
// TableLootEntry
// ============================================================================

TableLootEntry::TableLootEntry(const std::string& tableId, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_tableId(tableId)
{}

TableLootEntry::TableLootEntry(std::unique_ptr<LootTable> inlineTable, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_inlineTable(std::move(inlineTable))
{}

TableLootEntry::~TableLootEntry() = default;

std::unique_ptr<LootEntry> TableLootEntry::clone() const
{
    std::unique_ptr<TableLootEntry> entry;
    if (m_inlineTable) {
        entry = std::make_unique<TableLootEntry>(m_inlineTable->clone(), m_weight, m_quality);
    } else {
        entry = std::make_unique<TableLootEntry>(m_tableId, m_weight, m_quality);
    }
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

bool TableLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 对齐原版 `LootPoolSingletonContainer#expand`：条件通过才成为候选，
    // 并且条件只在这里判定一次（`generate` 不再重复判定）。
    if (!testConditions(context)) {
        return false;
    }
    consumer(*this);
    return true;
}

void TableLootEntry::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 条件已在 `expand` 阶段判定过，这里不再重复判定。
    const LootTable* table = m_inlineTable ? m_inlineTable.get() : context.getLootTable(m_tableId);
    if (table == nullptr) {
        return;
    }

    // 对齐原版 `NestedLootTable#createItemStack`：用 `getRandomItemsRaw` 直接投喂，
    // 不做同物品合并、也不做堆拆分（那些是「放进容器」时才有的行为）。
    table->generate(consumer, context);
}

} // namespace loot
} // namespace mc
