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

#include "LootContext.hpp"
#include "common/util/math/random/Random.hpp"
#include <string>

namespace mc {
namespace loot {

// ============================================================================
// LootContext
// ============================================================================

LootContext::LootContext(IWorld& world, math::Random& random)
    : m_world(world)
    , m_random(random)
{}

const LootTable* LootContext::getLootTable(const std::string& id) const noexcept
{
    if (m_lootTableResolver) {
        return m_lootTableResolver(id);
    }
    return nullptr;
}

bool LootContext::pushLootTable(const LootTable* table) noexcept
{
    // 检查是否已经在访问栈中（循环引用）
    for (const auto* visited : m_visitedTables) {
        if (visited == table) {
            return false; // 循环引用
        }
    }
    m_visitedTables.push_back(table);
    return true;
}

void LootContext::popLootTable(const LootTable* table) noexcept
{
    if (!m_visitedTables.empty() && m_visitedTables.back() == table) {
        m_visitedTables.pop_back();
    }
}

const LootCondition* LootContext::getPredicate(const std::string& id) const noexcept
{
    if (m_predicateResolver) {
        return m_predicateResolver(id);
    }
    return nullptr;
}

bool LootContext::pushPredicate(const LootCondition* predicate) noexcept
{
    // 检查是否已经在访问栈中（循环引用）
    for (const auto* visited : m_visitedPredicates) {
        if (visited == predicate) {
            return false; // 循环引用
        }
    }
    m_visitedPredicates.push_back(predicate);
    return true;
}

void LootContext::popPredicate(const LootCondition* predicate) noexcept
{
    if (!m_visitedPredicates.empty() && m_visitedPredicates.back() == predicate) {
        m_visitedPredicates.pop_back();
    }
}

} // namespace loot
} // namespace mc
