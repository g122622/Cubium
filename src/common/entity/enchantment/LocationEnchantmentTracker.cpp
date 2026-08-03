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
 * LIABILITY, AN ARISING FROM, ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "LocationEnchantmentTracker.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>

namespace mc {
namespace entity {

bool LocationEnchantmentTracker::isActive(i32 slot, const std::string& enchantmentId) const
{
    auto it = m_activeEnchantments.find(slot);
    if (it == m_activeEnchantments.end()) {
        return false;
    }
    return it->second.count(enchantmentId) > 0;
}

void LocationEnchantmentTracker::setActive(i32 slot, const std::string& enchantmentId)
{
    m_activeEnchantments[slot].insert(enchantmentId);
}

bool LocationEnchantmentTracker::setInactive(i32 slot, const std::string& enchantmentId)
{
    auto it = m_activeEnchantments.find(slot);
    if (it == m_activeEnchantments.end()) {
        return false;
    }
    auto& enchantments = it->second;
    size_t erased = enchantments.erase(enchantmentId);
    if (enchantments.empty()) {
        m_activeEnchantments.erase(it);
    }
    return erased > 0;
}

std::unordered_set<std::string> LocationEnchantmentTracker::clearSlot(i32 slot)
{
    auto it = m_activeEnchantments.find(slot);
    if (it == m_activeEnchantments.end()) {
        return {};
    }
    auto result = std::move(it->second);
    m_activeEnchantments.erase(it);
    return result;
}

void LocationEnchantmentTracker::clearAll()
{
    m_activeEnchantments.clear();
}

bool LocationEnchantmentTracker::hasActiveEnchantments() const
{
    return !m_activeEnchantments.empty();
}

const std::unordered_set<std::string>& LocationEnchantmentTracker::getActiveEnchantments(i32 slot) const
{
    static const std::unordered_set<std::string> empty;
    auto it = m_activeEnchantments.find(slot);
    if (it == m_activeEnchantments.end()) {
        return empty;
    }
    return it->second;
}

} // namespace entity
} // namespace mc
