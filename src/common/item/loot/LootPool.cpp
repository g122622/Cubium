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

#include "LootPool.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include "entries/ItemLootEntry.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace loot {

// ============================================================================
// LootPool
// ============================================================================

LootPool::LootPool(const RandomValueRange& rolls, const RandomValueRange& bonusRolls)
    : m_rolls(rolls)
    , m_bonusRolls(bonusRolls)
{}

void LootPool::addEntry(std::unique_ptr<LootEntry> entry)
{
    m_entries.push_back(std::move(entry));
}

void LootPool::addCondition(std::unique_ptr<LootCondition> condition)
{
    m_conditions.push_back(std::move(condition));
}

bool LootPool::testConditions(LootContext& context) const
{
    for (const auto& cond : m_conditions) {
        if (!cond->test(context)) {
            return false;
        }
    }
    return true;
}

void LootPool::addFunction(std::unique_ptr<LootFunction> function)
{
    m_functions.push_back(std::move(function));
}

ItemStack LootPool::applyFunctions(ItemStack stack, LootContext& context) const
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

std::unique_ptr<LootPool> LootPool::clone() const
{
    auto pool = std::make_unique<LootPool>(m_rolls, m_bonusRolls);
    pool->setName(m_name);
    for (const auto& entry : m_entries) {
        pool->addEntry(entry->clone());
    }
    for (const auto& cond : m_conditions) {
        pool->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        pool->addFunction(func->clone());
    }
    return pool;
}

void LootPool::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查池级条件
    const bool conditionsPassed = testConditions(context);
    // spdlog::info(
    // "LootPool::generate pool='{}' conditionsPassed={} entryCount={}", m_name, conditionsPassed, m_entries.size());
    if (!conditionsPassed) {
        return;
    }

    // 计算掷骰次数 = 基础次数 + 幸运值加成
    math::Random& random = context.getRandom();
    i32 rollCount =
        m_rolls.generateInt(random) + static_cast<i32>(m_bonusRolls.generateFloat(random) * context.getLuck());
    // spdlog::info("LootPool::generate pool='{}' rollCount={} luck={}", m_name, rollCount, context.getLuck());

    // 如果有池级函数，包装 consumer 以应用函数
    std::function<void(const ItemStack&)> poolConsumer;
    if (m_functions.empty()) {
        poolConsumer = consumer;
    } else {
        poolConsumer = [this, &consumer, &context](const ItemStack& stack) {
            if (!stack.isEmpty()) {
                ItemStack result = applyFunctions(stack, context);
                if (!result.isEmpty()) {
                    consumer(result);
                }
            }
        };
    }

    // 执行每次掷骰
    for (i32 i = 0; i < rollCount; ++i) {
        _generateRoll(poolConsumer, context);
    }
}

void LootPool::_generateRoll(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    if (m_entries.empty()) {
        // spdlog::info("LootPool::generateRoll pool='{}' skipped because there are no entries", m_name);
        return;
    }

    // 收集所有有效条目及其权重
    struct WeightedEntry {
        LootEntry* entry;
        i32 weight;
    };

    std::vector<WeightedEntry> weightedEntries;
    i32 totalWeight = 0;

    for (auto& entry : m_entries) {
        i32 effectiveWeight = entry->getEffectiveWeight(context.getLuck());
        // spdlog::info("LootPool::generateRoll pool='{}' entryType='{}' effectiveWeight={}",
        //     m_name,
        //     static_cast<i32>(entry->getType()),
        //     effectiveWeight);
        if (effectiveWeight > 0) {
            weightedEntries.push_back({entry.get(), effectiveWeight});
            totalWeight += effectiveWeight;
        }
    }

    if (weightedEntries.empty() || totalWeight <= 0) {
        spdlog::warn("LootPool::generateRoll pool='{}' found no weighted entries: weightedEntryCount={} totalWeight={}",
            m_name,
            weightedEntries.size(),
            totalWeight);
        return;
    }

    // 如果只有一个条目，直接生成
    if (weightedEntries.size() == 1) {
        // spdlog::info("LootPool::generateRoll pool='{}' selected sole entryType='{}'",
        //     m_name,
        //     static_cast<i32>(weightedEntries[0].entry->getType()));
        weightedEntries[0].entry->generate(consumer, context);
        return;
    }

    // 按权重随机选择
    math::Random& random = context.getRandom();
    i32 selected = random.nextInt(totalWeight);

    for (const auto& we : weightedEntries) {
        selected -= we.weight;
        if (selected < 0) {
            // spdlog::info("LootPool::generateRoll pool='{}' selected entryType='{}' weight={}",
            //     m_name,
            //     static_cast<i32>(we.entry->getType()),
            //     we.weight);
            we.entry->generate(consumer, context);
            return;
        }
    }

    // 如果选中了最后一个
    // spdlog::info("LootPool::generateRoll pool='{}' fell back to last entryType='{}'",
    //     m_name,
    //     static_cast<i32>(weightedEntries.back().entry->getType()));
    weightedEntries.back().entry->generate(consumer, context);
}

// ============================================================================
// LootPoolBuilder
// ============================================================================

LootPoolBuilder& LootPoolBuilder::item(const std::string& itemId, i32 count, i32 weight)
{
    auto entry = std::make_unique<ItemLootEntry>(
        itemId, RandomValueRange(static_cast<f32>(count), static_cast<f32>(count)), weight, 0);
    m_entries.push_back(std::move(entry));
    return *this;
}

std::unique_ptr<LootPool> LootPoolBuilder::build() const
{
    auto pool = std::make_unique<LootPool>(m_rolls, m_bonusRolls);
    pool->setName(m_name);

    for (const auto& entry : m_entries) {
        pool->addEntry(entry->clone());
    }
    for (const auto& cond : m_conditions) {
        pool->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        pool->addFunction(func->clone());
    }

    return pool;
}

} // namespace loot
} // namespace mc
