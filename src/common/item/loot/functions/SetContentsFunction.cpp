/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "SetContentsFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

SetContentsFunction::~SetContentsFunction() noexcept = default;

void SetContentsFunction::addEntry(std::unique_ptr<LootEntry> entry)
{
    if (entry) {
        m_entries.push_back(std::move(entry));
    }
}

ItemStack SetContentsFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty() || m_entries.empty()) {
        return stack;
    }

    // 收集生成的物品
    std::vector<ItemStack> generatedItems;

    // 遍历所有条目，展开并生成物品
    for (const auto& entry : m_entries) {
        if (!entry) {
            continue;
        }

        // 使用 generate() 方法生成物品
        entry->generate(
            [&generatedItems](const ItemStack& item) {
                if (!item.isEmpty()) {
                    // 如果物品数量超过最大堆叠数，需要拆分成多个堆
                    if (item.getCount() <= item.getMaxStackSize()) {
                        generatedItems.push_back(item);
                    } else {
                        // 拆分超限堆叠
                        i32 remaining = item.getCount();
                        while (remaining > 0) {
                            ItemStack splitItem = item.copy();
                            i32 splitCount = std::min(remaining, splitItem.getMaxStackSize());
                            splitItem.setCount(splitCount);
                            remaining -= splitCount;
                            generatedItems.push_back(std::move(splitItem));
                        }
                    }
                }
            },
            context);
    }

    // 如果没有生成任何物品，直接返回原堆
    if (generatedItems.empty()) {
        return stack;
    }

    // 将物品列表序列化到 BlockEntityTag
    nlohmann::json itemsArray = nlohmann::json::array();
    for (size_t i = 0; i < generatedItems.size(); ++i) {
        const ItemStack& item = generatedItems[i];
        if (item.isEmpty()) {
            continue;
        }

        // 序列化物品数据
        nlohmann::json itemJson = item.toJson();
        itemJson["Slot"] = static_cast<i32>(i);
        itemsArray.push_back(std::move(itemJson));
    }

    // 获取或创建 BlockEntityTag
    nlohmann::json& blockEntityTag = stack.getOrCreateChildTag("BlockEntityTag");

    // 创建新的物品数据 NBT
    nlohmann::json newItemData = nlohmann::json::object();
    newItemData["Items"] = std::move(itemsArray);

    // 合并到已有的 BlockEntityTag
    // 注意：新数据与旧数据合并时，旧数据的同名键会覆盖新数据
    // 所以我们先创建新数据，然后用旧数据覆盖
    ItemStack::mergeJsonObjects(newItemData, blockEntityTag);

    // 将合并后的数据写入 BlockEntityTag
    blockEntityTag = std::move(newItemData);

    return stack;
}

std::unique_ptr<LootFunction> SetContentsFunction::clone() const noexcept
{
    auto func = std::make_unique<SetContentsFunction>();
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    for (const auto& entry : m_entries) {
        func->addEntry(entry->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
