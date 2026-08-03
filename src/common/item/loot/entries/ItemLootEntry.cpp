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

void ItemLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<ItemLootEntry*>(this));
}

bool ItemLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    const bool conditionsPassed = testConditions(context);
    // spdlog::info("ItemLootEntry::generate itemId='{}' conditionsPassed={}", m_itemId, conditionsPassed);
    if (!conditionsPassed) {
        return false;
    }

    // 获取物品
    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(m_itemId));
    if (!item) {
        spdlog::warn("ItemLootEntry::generate could not resolve item '{}'", m_itemId);
        return false;
    }

    // 计算数量
    i32 count = m_count.generateInt(context.getRandom());
    // spdlog::info(
    // "ItemLootEntry::generate resolved item='{}' generatedCount={}", item->itemLocation().toString(), count);
    if (count <= 0) {
        return true; // 数量为0不算失败
    }

    // 创建物品堆
    ItemStack stack(*item, count);

    // 应用条目级函数
    stack = applyFunctions(std::move(stack), context);
    // spdlog::info("ItemLootEntry::generate after functions item='{}' count={} empty={}",
    //     stack.getItem() ? stack.getItem()->itemLocation().toString() : "null",
    //     stack.getCount(),
    //     stack.isEmpty());

    // 如果函数返回空堆，则不生成物品
    if (!stack.isEmpty()) {
        consumer(stack);
    } else {
        spdlog::warn("ItemLootEntry::generate stack became empty for itemId='{}' after functions", m_itemId);
    }

    return true;
}

} // namespace loot
} // namespace mc
