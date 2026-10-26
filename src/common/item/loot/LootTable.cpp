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

#include "LootTable.hpp"
#include "LootSerializers.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParameterSet.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace loot {

// ============================================================================
// LootTable
// ============================================================================

const LootTable LootTable::EMPTY;

void LootTable::addPool(std::unique_ptr<LootPool> pool)
{
    if (pool) {
        m_pools.push_back(std::move(pool));
    }
}

LootPool* LootTable::getPool(const std::string& name)
{
    for (auto& pool : m_pools) {
        if (pool->getName() == name) {
            return pool.get();
        }
    }
    return nullptr;
}

std::unique_ptr<LootPool> LootTable::removePool(const std::string& name)
{
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        if ((*it)->getName() == name) {
            auto pool = std::move(*it);
            m_pools.erase(it);
            return pool;
        }
    }
    return nullptr;
}

void LootTable::addFunction(std::unique_ptr<LootFunction> function)
{
    if (function) {
        m_functions.push_back(std::move(function));
    }
}

ConsumerPtr LootTable::decorate(ConsumerPtr consumer, LootContext& context) const
{
    if (m_functions.empty()) {
        return consumer;
    }
    // 对齐原版 `LootItemFunction#decorate`：本层函数对每个经过的堆执行一次，
    // 变换后的空堆照样交付，不在这一层过滤。
    const auto& functions = m_functions;
    return makeConsumer([&functions, consumer = std::move(consumer), &context](const ItemStack& received) {
        ItemStack stack = received;
        for (const auto& func : functions) {
            stack = func->apply(std::move(stack), context);
        }
        consumer->accept(stack);
    });
}

std::vector<ItemStack> LootTable::generate(LootContext& context) const
{
    std::vector<ItemStack> items;

    // 处理物品堆叠
    auto collected = [&items](const ItemStack& stack) {
        if (!stack.isEmpty()) {
            // 尝试合并到现有堆
            for (auto& existing : items) {
                if (existing.canMergeWith(stack)) {
                    i32 space = existing.getMaxStackSize() - existing.getCount();
                    if (space > 0) {
                        i32 toAdd = std::min(space, stack.getCount());
                        existing.grow(toAdd);
                        if (toAdd >= stack.getCount()) {
                            return; // 完全合并
                        }
                        // 部分合并，创建新堆
                        ItemStack remaining(stack.copy());
                        remaining.shrink(toAdd);
                        items.push_back(remaining);
                        return;
                    }
                }
            }
            // 无法合并，添加新堆
            items.push_back(stack);
        }
    };

    generate(makeConsumer(collected), context);
    return items;
}

void LootTable::generate(ConsumerPtr consumer, LootContext& context) const
{
    recursiveGenerate(std::move(consumer), context);
}

void LootTable::recursiveGenerate(ConsumerPtr consumer, LootContext& context) const
{
    // 参数集校验：检查必需参数是否已提供
    if (m_paramSet.getType() != LootParameterSet::Type::Empty &&
        m_paramSet.getType() != LootParameterSet::Type::Generic) {
        auto providedIds = context.getParamIds();
        if (!m_paramSet.validate(providedIds)) {
            spdlog::warn("Loot table '{}' parameter set validation failed: missing required parameters", m_id);
        }
    }

    // 循环检测
    if (!context.pushLootTable(this)) {
        // 检测到循环引用，跳过
        spdlog::error(
            "Detected loot table recursion loop at '{}', skipping loot generation to prevent infinite loop", m_id);
        return;
    }

    // 对齐原版 `LootTable#getRandomItemsRaw`：表级函数通过 `decorate` 挂到 consumer 上，
    // 对每个产出的堆都执行一次。
    const auto tableConsumer = decorate(consumer, context);

    // 执行所有池
    for (auto& pool : m_pools) {
        pool->generate(tableConsumer, context);
    }

    context.popLootTable(this);
}

Result<std::unique_ptr<LootTable>> LootTable::fromJson(const std::string& json)
{
    return LootSerializers::parseLootTable(json);
}

std::unique_ptr<LootTable> LootTable::clone() const
{
    auto table = std::make_unique<LootTable>();
    table->setId(m_id);
    table->setParameterSet(m_paramSet);

    for (const auto& pool : m_pools) {
        table->addPool(pool->clone());
    }
    for (const auto& func : m_functions) {
        table->addFunction(func->clone());
    }

    return table;
}

std::string LootTable::toJson() const
{
    return LootSerializers::toJsonString(*this, 2);
}

// ============================================================================
// LootTableBuilder
// ============================================================================

std::unique_ptr<LootTable> LootTableBuilder::build() const
{
    auto table = std::make_unique<LootTable>();
    table->setId(m_id);
    table->setParameterSet(m_paramSet);

    for (const auto& pool : m_pools) {
        table->addPool(pool->clone());
    }
    for (const auto& func : m_functions) {
        table->addFunction(func->clone());
    }

    return table;
}

} // namespace loot
} // namespace mc
