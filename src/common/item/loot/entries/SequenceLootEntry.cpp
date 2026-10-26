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

#include "SequenceLootEntry.hpp"
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
// SequenceLootEntry
// ============================================================================

SequenceLootEntry::SequenceLootEntry(std::vector<std::unique_ptr<LootEntry>> children)
    : m_children(std::move(children))
{}

// ============================================================================
// 公共方法
// ============================================================================

std::unique_ptr<LootEntry> SequenceLootEntry::clone() const
{
    // 深拷贝所有子条目
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    clonedChildren.reserve(m_children.size());
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }

    auto entry = std::make_unique<SequenceLootEntry>(std::move(clonedChildren));

    // 拷贝条件和函数
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void SequenceLootEntry::addChild(std::unique_ptr<LootEntry> child)
{
    m_children.push_back(std::move(child));
}

// ============================================================================
// 重写方法
// ============================================================================

bool SequenceLootEntry::expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const
{
    // 对齐原版 `SequentialEntry#compose`：依次展开，**任一子条目展开失败则整条序列
    // 不成立**。注意原版在这种情况下不会回滚已经交给消费者的候选（`and` 的语义是
    // 「前者先展开、后者返回假」，前者产生的候选仍会进入池），这里保持一致。
    if (!testConditions(context)) {
        return false;
    }

    for (const auto& child : m_children) {
        if (!child->expand(context, consumer)) {
            return false;
        }
    }
    return true;
}

void SequenceLootEntry::generate(ConsumerPtr consumer, LootContext& context) const
{
    // 逐个子条目重新展开并生成；条件不在此重复判定（理由同 alternatives）。
    for (const auto& child : m_children) {
        if (!child->expand(context, [&context, next = consumer](const LootEntry& selected) { selected.generate(next, context); })) {
            return;
        }
    }
}

} // namespace loot
} // namespace mc
