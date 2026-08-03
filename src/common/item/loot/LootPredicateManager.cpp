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
 * The above copyright notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "LootPredicateManager.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

// ============================================================================
// LootPredicateManager
// ============================================================================

void LootPredicateManager::registerPredicate(const std::string& id, std::unique_ptr<LootCondition> condition)
{
    if (condition) {
        m_predicates[id] = std::move(condition);
    }
}

const LootCondition* LootPredicateManager::getPredicate(const std::string& id) const
{
    auto it = m_predicates.find(id);
    if (it != m_predicates.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool LootPredicateManager::hasPredicate(const std::string& id) const
{
    return m_predicates.find(id) != m_predicates.end();
}

void LootPredicateManager::clear()
{
    m_predicates.clear();
}

std::vector<std::string> LootPredicateManager::getAllPredicateIds() const
{
    std::vector<std::string> ids;
    ids.reserve(m_predicates.size());
    for (const auto& [id, _] : m_predicates) {
        ids.push_back(id);
    }
    return ids;
}

} // namespace loot
} // namespace mc
