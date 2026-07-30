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
#include "common/item/core/ItemRegistry.hpp"
#include "conditions/LootConditions.hpp"
#include "functions/LootFunctions.hpp"
#include <algorithm>
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

ItemStack LootTable::applyFunctions(ItemStack stack, LootContext& context) const
{
    for (const auto& func : m_functions) {
        if (func->testConditions(context)) {
            stack = func->apply(stack, context);
            if (stack.isEmpty()) {
                return stack;
            }
        }
    }
    return stack;
}

std::vector<ItemStack> LootTable::generate(LootContext& context) const
{
    std::vector<ItemStack> items;

    // 处理物品堆叠
    auto consumer = [&items](const ItemStack& stack) {
        spdlog::info("LootTable::generate received item stack: {} x{}",
            stack.getItem() ? stack.getItem()->itemLocation().toString() : "null",
            stack.getCount());
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

    recursiveGenerate(consumer, context);
    // spdlog::info("Generated {} item stacks from loot table '{}'", items.size(), m_id);
    return items;
}

void LootTable::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    recursiveGenerate(consumer, context);
}

void LootTable::recursiveGenerate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
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

    // 如果有表级函数，包装 consumer 以应用函数
    std::function<void(const ItemStack&)> tableConsumer;
    if (m_functions.empty()) {
        tableConsumer = consumer;
    } else {
        tableConsumer = [this, &consumer, &context](const ItemStack& stack) {
            if (!stack.isEmpty()) {
                ItemStack result = applyFunctions(stack, context);
                if (!result.isEmpty()) {
                    consumer(result);
                }
            }
        };
    }

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
