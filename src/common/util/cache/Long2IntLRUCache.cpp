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

#include "Long2IntLRUCache.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <mutex>

namespace mc {

Long2IntLRUCache::Long2IntLRUCache(i32 maxSize)
    : m_maxSize(maxSize)
{
    m_cache.reserve(static_cast<size_t>(maxSize * 2)); // 预分配空间
}

bool Long2IntLRUCache::get(i64 key, i32& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return getLocked(key, value);
}

void Long2IntLRUCache::put(i64 key, i32 value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    putLocked(key, value);
}

i32 Long2IntLRUCache::size() const
{
    return static_cast<i32>(m_cache.size());
}

void Long2IntLRUCache::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_list.clear();
}

bool Long2IntLRUCache::getLocked(i64 key, i32& value)
{
    // 注意：此方法不加锁，调用方必须已持有锁
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // 移动到链表前端（最近访问）
        m_list.splice(m_list.begin(), m_list, it->second);
        value = it->second->second;
        return true;
    }
    return false;
}

void Long2IntLRUCache::putLocked(i64 key, i32 value)
{
    // 注意：此方法不加锁，调用方必须已持有锁

    // 检查是否已存在
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // 更新值并移动到前端
        it->second->second = value;
        m_list.splice(m_list.begin(), m_list, it->second);
        return;
    }

    // 检查是否需要淘汰
    if (static_cast<i32>(m_cache.size()) >= m_maxSize) {
        // 移除链表尾部（最旧）
        i64 oldestKey = m_list.back().first;
        m_cache.erase(oldestKey);
        m_list.pop_back();
    }

    // 插入新条目到前端
    m_list.emplace_front(key, value);
    m_cache[key] = m_list.begin();
}

} // namespace mc
