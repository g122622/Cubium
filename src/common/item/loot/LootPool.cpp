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
#include "common/util/math/MathUtils.hpp"
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

ConsumerPtr LootPool::decorate(ConsumerPtr consumer, LootContext& context) const
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

void LootPool::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 检查池级条件
    const bool conditionsPassed = testConditions(context);
    // spdlog::info(
    // "LootPool::generate pool='{}' conditionsPassed={} entryCount={}", m_name, conditionsPassed, m_entries.size());
    if (!conditionsPassed) {
        return;
    }

    // 计算掷骰次数 = 基础次数 + floor(加成次数 × 幸运值)
    // 对齐原版 `LootPool#addRandomItems`：加成部分先乘幸运值再 `Mth.floor`，不是先转整数再乘。
    math::IRandom& random = context.getRandom();
    const i32 rollCount =
        m_rolls.generateInt(random) + math::floorTo<i32>(m_bonusRolls.generateFloat(random) * context.getLuck());

    // 对齐原版：池级函数通过 `decorate` 挂到 consumer 上，**对每个产出的堆都执行一次**
    // （包括变换后成为空堆的那些），因此这里不预先过滤空堆。
    const auto poolConsumer = decorate(std::move(consumer), context);
    // 执行每次掷骰
    for (i32 i = 0; i < rollCount; ++i) {
        _generateRoll(poolConsumer, context);
    }
}

void LootPool::_generateRoll(const ConsumerPtr& consumer, LootContext& context) const
{
    if (m_entries.empty()) {
        // spdlog::info("LootPool::generateRoll pool='{}' skipped because there are no entries", m_name);
        return;
    }

    // 收集所有有效条目及其权重。
    //
    // 对齐原版 LootPool#addRandomItem：先用 `expand` 把组合条目展开成候选，
    // **再**按权重抽签。组合条目（alternatives / group / sequence）内部的条件
    // 在这一步就决定它是否成为候选——条件不满足的分支既不占权重也不消耗随机数。
    // 若改成「先按权重选中、再判定条件」，alternatives 这类表会给出完全不同的结果。
    struct WeightedEntry {
        const LootEntry* entry;
        i32 weight;
    };

    std::vector<WeightedEntry> weightedEntries;
    i32 totalWeight = 0;

    for (const auto& entry : m_entries) {
        entry->expand(context, [&context, &weightedEntries, &totalWeight](const LootEntry& candidate) {
            i32 effectiveWeight = candidate.getEffectiveWeight(context.getLuck());
            if (effectiveWeight > 0) {
                weightedEntries.push_back({&candidate, effectiveWeight});
                totalWeight += effectiveWeight;
            }
        });
    }

    if (weightedEntries.empty() || totalWeight <= 0) {
        // 原版在这种情况下静默跳过（没有可抽的条目不是错误：条件被上下文筛空很常见）。
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
    math::IRandom& random = context.getRandom();
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
