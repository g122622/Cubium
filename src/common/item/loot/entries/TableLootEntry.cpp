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

void TableLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<TableLootEntry*>(this));
}

bool TableLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 内联表：直接生成，复用 LootTable::generate 内部的循环检测
    if (m_inlineTable) {
        auto items = m_inlineTable->generate(context);
        for (const auto& item : items) {
            consumer(item);
        }
        return !items.empty();
    }

    // 外部引用：通过解析器查找掉落表
    const LootTable* table = context.getLootTable(m_tableId);
    if (!table) {
        return false;
    }

    auto items = table->generate(context);
    for (const auto& item : items) {
        consumer(item);
    }

    return !items.empty();
}

} // namespace loot
} // namespace mc
