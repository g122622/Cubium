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
#include <vector>
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

bool DynamicLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 动态条目本身就是一个候选，条件只在此判定一次（`generate` 不再重复判定）。
    if (!testConditions(context)) {
        return false;
    }
    consumer(*this);
    return true;
}

void DynamicLootEntry::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 条件已在 `expand` 阶段判定过，这里不再重复判定。
    // 目前仅支持 minecraft:contents（从容器方块实体中读取物品）
    if (m_name == "minecraft:contents" || m_name == "contents") {
        // 从上下文获取方块实体
        auto* blockEntity = context.get<BlockEntity>(LootParams::BLOCK_ENTITY);
        if (blockEntity) {
            auto* containerEntity = dynamic_cast<ContainerBlockEntity*>(blockEntity);
            if (containerEntity) {
                IInventory* inventory = containerEntity->getInventory();
                if (inventory) {
                    // 原版 `DynamicLoot` 把被条目函数装饰过的回调交给 `addDynamicDrops`，
                    // 由上下文逐个投喂物品堆；本项目把这段遍历放在这里，语义相同。
                    const auto decorated = decorate(std::move(consumer), context);
                    for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
                        ItemStack stack = inventory->getItem(i);
                        if (!stack.isEmpty()) {
                            decorated->accept(stack);
                        }
                    }
                    return;
                }
            }
        }
        // 无方块实体或非容器：不生成任何物品
        return;
    }

    // 未知动态名称：不生成物品
}

} // namespace loot
} // namespace mc
