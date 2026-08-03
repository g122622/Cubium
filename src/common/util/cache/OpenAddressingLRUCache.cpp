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

#include "OpenAddressingLRUCache.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace mc {

OpenAddressingLRUCache::OpenAddressingLRUCache(i32 maxSize)
    : m_maxSize(static_cast<u32>(maxSize))
    , m_shift(0)
{
    // 计算容量：不小于 maxSize * 1.5 的最小 2 的幂次
    // 这样可以保证负载因子不超过 0.67
    u32 minCapacity = static_cast<u32>(maxSize * 1.5);
    m_capacity = 1;
    while (m_capacity < minCapacity) {
        m_capacity *= 2;
        ++m_shift;
    }
    // m_shift = 64 - log2(capacity)，用于 FIB 哈希
    m_shift = 64 - m_shift;

    // 初始化哈希表
    m_table.resize(m_capacity);
}

bool OpenAddressingLRUCache::get(i64 key, i32& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return getLocked(key, value);
}

void OpenAddressingLRUCache::put(i64 key, i32 value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    putLocked(key, value);
}

i32 OpenAddressingLRUCache::size() const
{
    return static_cast<i32>(m_size.load());
}

void OpenAddressingLRUCache::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_table.clear();
    m_table.resize(m_capacity);
    m_size.store(0);
    m_timestamp = 0;
    m_hitCount.store(0);
    m_missCount.store(0);
}

bool OpenAddressingLRUCache::getLocked(i64 key, i32& value)
{
    // 使用开放寻址线性探测查找
    Entry* entry = findEntry(key);
    if (entry != nullptr && entry->occupied) {
        // 命中：更新时间戳
        entry->timestamp = ++m_timestamp;
        value = entry->value;
        m_hitCount.fetch_add(1);
        return true;
    }

    // 未命中
    m_missCount.fetch_add(1);
    return false;
}

void OpenAddressingLRUCache::putLocked(i64 key, i32 value)
{
    // 先查找是否已存在
    Entry* existing = findEntry(key);
    if (existing != nullptr && existing->occupied) {
        // 更新已存在的条目
        existing->value = value;
        existing->timestamp = ++m_timestamp;
        return;
    }

    // 检查是否需要淘汰
    if (m_size.load() >= m_maxSize) {
        evictBatch();
    }

    // 找到空槽位插入
    Entry* slot = findEmptySlot(key);
    slot->key = key;
    slot->value = value;
    slot->timestamp = ++m_timestamp;
    slot->occupied = true;
    m_size.fetch_add(1);
}

size_t OpenAddressingLRUCache::hashIndex(i64 key) const
{
    // FIB 哈希：比取模分布更均匀
    // 对于 2 的幂次容量，使用位运算代替取模
    // 结果需要与 (m_capacity - 1) 进行与运算以保持在范围内
    return static_cast<size_t>((static_cast<u64>(key) * FIB_MULTIPLIER) >> m_shift) & (m_capacity - 1);
}

void OpenAddressingLRUCache::evictBatch()
{
    // 计算需要淘汰的条目数
    u32 evictCount = m_capacity / EVICT_DIVISOR;
    if (evictCount == 0) {
        evictCount = 1;
    }

    // 找到最旧的 evictCount 个条目
    // 使用选择算法：找到第 k 小的时间戳
    std::vector<std::pair<u32, size_t>> timestamps;
    timestamps.reserve(m_size.load());

    for (size_t i = 0; i < m_capacity; ++i) {
        if (m_table[i].occupied) {
            timestamps.emplace_back(m_table[i].timestamp, i);
        }
    }

    if (timestamps.size() <= evictCount) {
        // 淘汰所有条目
        for (auto& entry : m_table) {
            entry.occupied = false;
        }
        m_size.store(0);
        return;
    }

    // 使用 nth_element 找到第 evictCount 小的时间戳
    std::nth_element(
        timestamps.begin(), timestamps.begin() + evictCount, timestamps.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

    // 淘汰时间戳最小的 evictCount 个条目
    u32 threshold = timestamps[evictCount].first;
    for (size_t i = 0; i < m_capacity; ++i) {
        if (m_table[i].occupied && m_table[i].timestamp < threshold) {
            m_table[i].occupied = false;
            m_size.fetch_sub(1);
        }
    }
}

OpenAddressingLRUCache::Entry* OpenAddressingLRUCache::findEntry(i64 key)
{
    size_t index = hashIndex(key);
    size_t startIndex = index;

    // 线性探测
    do {
        Entry& entry = m_table[index];
        if (!entry.occupied) {
            // 空槽位，说明不存在
            return nullptr;
        }
        if (entry.key == key) {
            // 找到
            return &entry;
        }
        // 继续探测
        index = (index + 1) & (m_capacity - 1);
    } while (index != startIndex);

    // 表已满且未找到
    return nullptr;
}

OpenAddressingLRUCache::Entry* OpenAddressingLRUCache::findEmptySlot(i64 key)
{
    size_t index = hashIndex(key);

    // 线性探测找到空槽位或可覆盖的槽位
    while (m_table[index].occupied) {
        index = (index + 1) & (m_capacity - 1);
    }

    return &m_table[index];
}

} // namespace mc
