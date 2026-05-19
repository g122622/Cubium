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
#include "LootConditions.hpp"
#include <algorithm>

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

std::unique_ptr<LootPool> LootPool::clone() const
{
    auto pool = std::make_unique<LootPool>(m_rolls, m_bonusRolls);
    pool->setName(m_name);
    for (const auto& entry : m_entries) {
        pool->addEntry(entry->clone());
    }
    return pool;
}

void LootPool::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 计算掷骰次数 = 基础次数 + 幸运值加成
    math::Random& random = context.getRandom();
    i32 rollCount =
        m_rolls.generateInt(random) + static_cast<i32>(m_bonusRolls.generateFloat(random) * context.getLuck());

    // 执行每次掷骰
    for (i32 i = 0; i < rollCount; ++i) {
        generateRoll(consumer, context);
    }
}

void LootPool::generateRoll(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    if (m_entries.empty()) {
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
        if (effectiveWeight > 0) {
            weightedEntries.push_back({entry.get(), effectiveWeight});
            totalWeight += effectiveWeight;
        }
    }

    if (weightedEntries.empty() || totalWeight <= 0) {
        return;
    }

    // 如果只有一个条目，直接生成
    if (weightedEntries.size() == 1) {
        weightedEntries[0].entry->generate(consumer, context);
        return;
    }

    // 按权重随机选择
    math::Random& random = context.getRandom();
    i32 selected = random.nextInt(totalWeight);

    for (const auto& we : weightedEntries) {
        selected -= we.weight;
        if (selected < 0) {
            we.entry->generate(consumer, context);
            return;
        }
    }

    // 如果选中了最后一个
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

    return pool;
}

} // namespace loot
} // namespace mc
