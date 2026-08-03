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

#include "EmptyLootEntry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include <functional>
#include <memory>

namespace mc {
namespace loot {

// ============================================================================
// EmptyLootEntry
// ============================================================================

std::unique_ptr<LootEntry> EmptyLootEntry::clone() const
{
    auto entry = std::make_unique<EmptyLootEntry>(m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void EmptyLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    // 空条目仍然可以被选择，但不生成任何物品
    consumer(*const_cast<EmptyLootEntry*>(this));
}

bool EmptyLootEntry::generate(std::function<void(const ItemStack&)> /*consumer*/, LootContext& /*context*/) const
{
    // 空条目不生成物品，但返回true表示"成功"（可用于条件判断）
    return true;
}

} // namespace loot
} // namespace mc
