#include "DirtyTracker.hpp"
#include <mutex>

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
