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

#include "DynamicLootEntry.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace loot {

// ============================================================================
// DynamicLootEntry
// ============================================================================

DynamicLootEntry::DynamicLootEntry(const std::string& name, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_name(name)
{}

std::unique_ptr<LootEntry> DynamicLootEntry::clone() const
{
    auto entry = std::make_unique<DynamicLootEntry>(m_name, m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void DynamicLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    // 动态条目不展开，自身作为候选条目
    consumer(*const_cast<DynamicLootEntry*>(this));
}

bool DynamicLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 目前仅支持 minecraft:contents（从容器方块实体中读取物品）
    if (m_name == "minecraft:contents" || m_name == "contents") {
        // 从上下文获取方块实体
        auto* blockEntity = context.get<BlockEntity>(LootParams::BLOCK_ENTITY);
        if (blockEntity) {
            auto* containerEntity = dynamic_cast<ContainerBlockEntity*>(blockEntity);
            if (containerEntity) {
                IInventory* inventory = containerEntity->getInventory();
                if (inventory) {
                    bool anyItems = false;
                    for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
                        ItemStack stack = inventory->getItem(i);
                        if (!stack.isEmpty()) {
                            stack = applyFunctions(std::move(stack), context);
                            if (!stack.isEmpty()) {
                                consumer(stack);
                                anyItems = true;
                            }
                        }
                    }
                    return anyItems;
                }
            }
        }
        // 无方块实体或非容器，返回 false
        return false;
    }

    // 未知动态名称，不生成物品
    return false;
}

} // namespace loot
} // namespace mc
