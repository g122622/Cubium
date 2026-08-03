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

#include "DirtyTracker.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include <cstddef>
#include <mutex>
#include <vector>

namespace mc::world::storage {

// ============================================================================
// 脏标记操作
// ============================================================================

bool DirtyTracker::markDirty(const SectionKey& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto [it, inserted] = m_dirtySet.insert(key);
    return inserted;
}

bool DirtyTracker::clearDirty(const SectionKey& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dirtySet.erase(key) > 0;
}

bool DirtyTracker::isDirty(const SectionKey& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dirtySet.count(key) > 0;
}

void DirtyTracker::clearAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dirtySet.clear();
}

// ============================================================================
// 查询操作
// ============================================================================

size_t DirtyTracker::dirtyCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dirtySet.size();
}

std::vector<SectionKey> DirtyTracker::getDirtyKeys() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<SectionKey>(m_dirtySet.begin(), m_dirtySet.end());
}

// ============================================================================
// 批量操作
// ============================================================================

size_t DirtyTracker::markDirtyBatch(const std::vector<SectionKey>& keys)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& key : keys) {
        auto [it, inserted] = m_dirtySet.insert(key);
        if (inserted) {
            ++count;
        }
    }
    return count;
}

size_t DirtyTracker::clearDirtyBatch(const std::vector<SectionKey>& keys)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& key : keys) {
        if (m_dirtySet.erase(key) > 0) {
            ++count;
        }
    }
    return count;
}

} // namespace mc::world::storage
