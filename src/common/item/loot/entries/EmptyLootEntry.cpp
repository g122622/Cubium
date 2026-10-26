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
#include <vector>

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

bool EmptyLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 空条目仍然可以被抽中（占权重），只是不产出物品。
    if (!testConditions(context)) {
        return false;
    }
    consumer(*this);
    return true;
}

void EmptyLootEntry::generate(ConsumerPtr /*consumer*/, LootContext& /*context*/) const
{
    // 空条目不生成任何物品（原版 `EmptyLootItem#createItemStack` 是空实现）。
}

} // namespace loot
} // namespace mc
