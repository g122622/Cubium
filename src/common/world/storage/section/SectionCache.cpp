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

#include "SectionCache.hpp"
#include "common/core/Types.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/world/storage/db/SectionCodec.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

SectionCache::SectionCache(size_t capacity)
    : m_capacity(capacity)
{
    m_stats.capacity = capacity;
}

// ============================================================================
// 缓存操作
// ============================================================================

std::shared_ptr<SectionData> SectionCache::get(const SectionKey& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cacheMap.find(key);
    if (it == m_cacheMap.end()) {
        ++m_stats.misses;
        return nullptr;
    }

    // 更新访问顺序
    _updateAccessOrder(it->second);

    // 更新访问时间
    it->second->entry.lastAccessTime = _getCurrentTimeMs();

    ++m_stats.hits;
    return it->second->entry.data;
}

std::shared_ptr<SectionData> SectionCache::put(const SectionKey& key, std::shared_ptr<SectionData> data, bool dirty)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否已存在
    auto it = m_cacheMap.find(key);
    if (it != m_cacheMap.end()) {
        // 更新现有条目
        auto oldValue = it->second->entry.data;
        it->second->entry.data = std::move(data);
        it->second->entry.dirty = dirty;
        it->second->entry.lastAccessTime = _getCurrentTimeMs();
        _updateAccessOrder(it->second);
        return oldValue;
    }

    // 驱逐直到有空间
    std::shared_ptr<SectionData> evicted;
    while (m_lruList.size() >= m_capacity) {
        evicted = _evictLRU();
    }

    // 添加新条目
    m_lruList.push_front({key, CacheEntry{std::move(data), dirty, _getCurrentTimeMs()}});
    m_cacheMap[key] = m_lruList.begin();

    m_stats.currentSize = m_lruList.size();

    return evicted;
}

bool SectionCache::contains(const SectionKey& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cacheMap.find(key) != m_cacheMap.end();
}

std::shared_ptr<SectionData> SectionCache::evict(const SectionKey& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cacheMap.find(key);
    if (it == m_cacheMap.end()) {
        return nullptr;
    }

    auto data = it->second->entry.data;
    m_lruList.erase(it->second);
    m_cacheMap.erase(it);

    m_stats.currentSize = m_lruList.size();
    return data;
}

std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> SectionCache::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> dirtySections;

    // 收集脏Section
    for (const auto& node : m_lruList) {
        if (node.entry.dirty && node.entry.data) {
            dirtySections.emplace_back(node.key, node.entry.data);
        }
    }

    m_lruList.clear();
    m_cacheMap.clear();
    m_stats.currentSize = 0;

    return dirtySections;
}

// ============================================================================
// 脏标记管理
// ============================================================================

bool SectionCache::markDirty(const SectionKey& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cacheMap.find(key);
    if (it == m_cacheMap.end()) {
        return false;
    }

    it->second->entry.dirty = true;
    return true;
}

bool SectionCache::markClean(const SectionKey& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cacheMap.find(key);
    if (it == m_cacheMap.end()) {
        return false;
    }

    it->second->entry.dirty = false;
    return true;
}

bool SectionCache::isDirty(const SectionKey& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cacheMap.find(key);
    if (it == m_cacheMap.end()) {
        return false;
    }

    return it->second->entry.dirty;
}

std::vector<SectionKey> SectionCache::getDirtyKeys() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<SectionKey> keys;
    keys.reserve(m_lruList.size() / 4); // 预估25%为脏

    for (const auto& node : m_lruList) {
        if (node.entry.dirty) {
            keys.push_back(node.key);
        }
    }

    return keys;
}

std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> SectionCache::getDirtySections() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> sections;
    sections.reserve(m_lruList.size() / 4); // 预估25%为脏

    for (const auto& node : m_lruList) {
        if (node.entry.dirty && node.entry.data) {
            sections.emplace_back(node.key, node.entry.data);
        }
    }

    return sections;
}

std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> SectionCache::getAllSections() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> sections;
    sections.reserve(m_lruList.size());

    for (const auto& node : m_lruList) {
        if (node.entry.data) {
            sections.emplace_back(node.key, node.entry.data);
        }
    }

    return sections;
}

// ============================================================================
// 缓存管理
// ============================================================================

void SectionCache::setCapacity(size_t capacity)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_capacity = capacity;
    m_stats.capacity = capacity;

    // 驱逐多余的Section
    while (m_lruList.size() > m_capacity) {
        _evictLRU();
    }

    m_stats.currentSize = m_lruList.size();
}

size_t SectionCache::size() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lruList.size();
}

SectionCache::CacheStats SectionCache::getStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.currentSize = m_lruList.size();
    return m_stats;
}

void SectionCache::resetStats()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.hits = 0;
    m_stats.misses = 0;
    m_stats.evictions = 0;
}

// ============================================================================
// 内部方法
// ============================================================================

void SectionCache::_updateAccessOrder(LRUIterator it)
{
    // 移动到列表最前
    m_lruList.splice(m_lruList.begin(), m_lruList, it);
}

std::shared_ptr<SectionData> SectionCache::_evictLRU()
{
    if (m_lruList.empty()) {
        return nullptr;
    }

    // 获取最久未使用的条目
    auto& last = m_lruList.back();
    auto data = last.entry.data;

    // 从映射和列表中移除
    m_cacheMap.erase(last.key);
    m_lruList.pop_back();

    ++m_stats.evictions;
    m_stats.currentSize = m_lruList.size();

    return data;
}

u64 SectionCache::_getCurrentTimeMs()
{
    return util::TimeUtils::getCurrentTimeMs();
}

} // namespace mc::world::storage
