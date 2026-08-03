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

#include "LootEntryBuilder.hpp"
#include "DynamicLootEntry.hpp"
#include "EmptyLootEntry.hpp"
#include "ItemLootEntry.hpp"
#include "TableLootEntry.hpp"
#include "TagLootEntry.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

// ============================================================================
// LootEntryBuilder
// ============================================================================

LootEntryBuilder LootEntryBuilder::item(const std::string& itemId)
{
    LootEntryBuilder builder;
    builder.m_itemId = itemId;
    builder.m_type = LootEntryType::Item;
    return builder;
}

LootEntryBuilder LootEntryBuilder::empty()
{
    LootEntryBuilder builder;
    builder.m_type = LootEntryType::Empty;
    return builder;
}

LootEntryBuilder LootEntryBuilder::table(const std::string& tableId)
{
    LootEntryBuilder builder;
    builder.m_tableId = tableId;
    builder.m_type = LootEntryType::Table;
    return builder;
}

LootEntryBuilder LootEntryBuilder::tag(const std::string& tagId, bool expand)
{
    LootEntryBuilder builder;
    builder.m_tagId = tagId;
    builder.m_expand = expand;
    builder.m_type = LootEntryType::Tag;
    return builder;
}

LootEntryBuilder LootEntryBuilder::dynamic_(const std::string& name)
{
    LootEntryBuilder builder;
    builder.m_dynamicName = name;
    builder.m_type = LootEntryType::Dynamic;
    return builder;
}

LootEntryBuilder& LootEntryBuilder::count(f32 min, f32 max)
{
    m_count = RandomValueRange(min, max);
    return *this;
}

LootEntryBuilder& LootEntryBuilder::count(i32 value)
{
    m_count = RandomValueRange(static_cast<f32>(value), static_cast<f32>(value));
    return *this;
}

std::unique_ptr<LootEntry> LootEntryBuilder::build() const
{
    std::unique_ptr<LootEntry> entry;

    switch (m_type) {
        case LootEntryType::Item:
            entry = std::make_unique<ItemLootEntry>(m_itemId, m_count, m_weight, m_quality);
            break;
        case LootEntryType::Table:
            entry = std::make_unique<TableLootEntry>(m_tableId, m_weight, m_quality);
            break;
        case LootEntryType::Tag:
            entry = std::make_unique<TagLootEntry>(m_tagId, m_expand, m_weight, m_quality);
            break;
        case LootEntryType::Dynamic:
            entry = std::make_unique<DynamicLootEntry>(m_dynamicName, m_weight, m_quality);
            break;
        case LootEntryType::Empty:
        default:
            entry = std::make_unique<EmptyLootEntry>(m_weight, m_quality);
            break;
    }

    // 添加条件
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }

    // 添加函数
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }

    return entry;
}

} // namespace loot
} // namespace mc
