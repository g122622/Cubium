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

void SequenceLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    // 序列条目直接将自身添加到候选列表
    consumer(*const_cast<SequenceLootEntry*>(this));
}

bool SequenceLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 按顺序执行所有子条目，直到一个失败
    for (const auto& child : m_children) {
        if (!child->generate(consumer, context)) {
            return false;
        }
    }
    return true;
}

} // namespace loot
} // namespace mc
