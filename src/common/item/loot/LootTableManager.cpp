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

#include "LootTableManager.hpp"
#include "LootPredicateManager.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

// ============================================================================
// LootTableManager
// ============================================================================

void LootTableManager::registerTable(const std::string& id, std::unique_ptr<LootTable> table)
{
    if (table) {
        table->setId(id);
        m_tables[id] = std::move(table);
    }
}

const LootTable* LootTableManager::getTable(const std::string& id) const
{
    auto it = m_tables.find(id);
    if (it != m_tables.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool LootTableManager::hasTable(const std::string& id) const
{
    return m_tables.find(id) != m_tables.end();
}

void LootTableManager::clear()
{
    m_tables.clear();
}

std::vector<std::string> LootTableManager::getAllTableIds() const
{
    std::vector<std::string> ids;
    ids.reserve(m_tables.size());
    for (const auto& [id, table] : m_tables) {
        ids.push_back(id);
    }
    return ids;
}

const LootTable& LootTableManager::getEmptyTable()
{
    return LootTable::EMPTY;
}

const LootCondition* LootTableManager::getPredicate(const std::string& id) const noexcept
{
    if (m_predicateManager != nullptr) {
        return m_predicateManager->getPredicate(id);
    }
    return nullptr;
}

} // namespace loot
} // namespace mc
