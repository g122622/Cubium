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

#include "AlternativesLootEntry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

// ============================================================================
// AlternativesLootEntry
// ============================================================================

AlternativesLootEntry::AlternativesLootEntry(std::vector<std::unique_ptr<LootEntry>> children)
    : m_children(std::move(children))
{}

std::unique_ptr<LootEntry> AlternativesLootEntry::clone() const
{
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<AlternativesLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void AlternativesLootEntry::addChild(std::unique_ptr<LootEntry> child)
{
    m_children.push_back(std::move(child));
}

bool AlternativesLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 对齐原版 `AlternativesEntry#compose`：
    //   逐个尝试子条目，**第一个展开成功的**把候选交给消费者，后面的不再尝试。
    //   「展开成功」= 子条目自身的条件通过（由子条目的 expand 判定）。
    // 若全部失败，本条目不产生候选。
    if (!testConditions(context)) {
        return false;
    }

    for (const auto& child : m_children) {
        if (child->expand(context, consumer)) {
            return true;
        }
    }
    return false;
}

void AlternativesLootEntry::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 本条目作为候选被抽中时，`expand` 已经决定由哪个子条目出货；这里按同一规则
    // 重新展开一次并让第一个成功者生成。条件不在这里重复判定——重复判定会让带随机
    // 数的条件被评估两次，随机数序列与原版错位。
    for (const auto& child : m_children) {
        if (child->expand(context, [&context, next = consumer](const LootEntry& selected) { selected.generate(next, context); })) {
            return;
        }
    }
}

} // namespace loot
} // namespace mc
