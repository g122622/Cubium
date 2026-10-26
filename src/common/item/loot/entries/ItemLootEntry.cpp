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

#include "ItemLootEntry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc {
namespace loot {

// ============================================================================
// ItemLootEntry
// ============================================================================

ItemLootEntry::ItemLootEntry(const std::string& itemId, const RandomValueRange& count, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_itemId(itemId)
    , m_count(count)
{}

std::unique_ptr<LootEntry> ItemLootEntry::clone() const
{
    auto entry = std::make_unique<ItemLootEntry>(m_itemId, m_count, m_weight, m_quality);
    // 复制条件
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    // 复制函数
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

bool ItemLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 对齐原版 `LootPoolSingletonContainer#expand`：条件通过才成为候选，
    // 并且条件只在这里判定一次（`generate` 不再重复判定）。
    if (!testConditions(context)) {
        return false;
    }
    consumer(*this);
    return true;
}

void ItemLootEntry::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 条件已在 `expand` 阶段判定过，这里不再重复判定。
    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(m_itemId));
    if (!item) {
        spdlog::warn("ItemLootEntry::generate could not resolve item '{}'", m_itemId);
        return;
    }

    // 对齐原版 `LootItem#createItemStack`：先造 **数量为 1** 的堆，**不消耗随机数**。
    // 数据包里若有 `count` 字段（原版没有这个字段，本项目的扩展），按需取数。
    const bool hasEntryCount = !(m_count.getMin() == 1.0f && m_count.getMax() == 1.0f);
    ItemStack stack(*item, hasEntryCount ? m_count.generateInt(context.getRandom()) : 1);

    // 交给被条目函数装饰过的下游回调（原版 `decoration` 同样无条件执行，空堆照交）。
    const auto decorated = decorate(std::move(consumer), context);
    decorated->accept(stack);
}

} // namespace loot
} // namespace mc
