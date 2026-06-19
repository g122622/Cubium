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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"
#include <cmath>
#include <list>
#include <unordered_map>
#include <utility>

namespace mc {

/**
 * @brief Long → Float LRU 缓存
 *
 * 参考 MC 1.21.11 的 Long2FloatLinkedOpenHashMap，用于 Biome 温度缓存。
 * 特性：
 * - 容量固定，不进行 rehash
 * - 默认返回 NaN 表示缓存未命中
 * - FIFO 淘汰（与 MC 一致，不是 LRU）
 *
 * 用法示例：
 * @code
 * Long2FloatLRUCache cache(1024);
 * float value = cache.get(key);
 * if (std::isnan(value)) {
 *     value = computeTemperature(x, y, z);
 *     cache.put(key, value);
 * }
 * @endcode
 */
class Long2FloatLRUCache {
public:
    /**
     * @brief 构造 LRU 缓存
     * @param maxSize 最大缓存条目数
     *
     * MC 1.21.11 Biome.TEMPERATURE_CACHE_SIZE = 1024
     */
    explicit Long2FloatLRUCache(i32 maxSize = 1024)
        : m_maxSize(maxSize)
    {}

    /**
     * @brief 获取缓存值
     * @param key 键
     * @return 缓存的值，如果未命中返回 NaN
     *
     * MC: Long2FloatLinkedOpenHashMap.defaultReturnValue(Float.NaN)
     * MC: getAndMoveToFirst() — 但本实现使用 FIFO（不移动到首部）
     * 因为 MC 的 getTemperature 也只是 get 而非 getAndMoveToFirst。
     */
    [[nodiscard]] f32 get(i64 key) const
    {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second->second;
        }
        return NAN;
    }

    /**
     * @brief 设置缓存值
     * @param key 键
     * @param value 值
     *
     * MC: 如果 size == 1024 则 removeFirstFloat()，然后 putAndMoveToFirst()
     */
    void put(i64 key, f32 value)
    {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // 已存在：更新值并移到前面
            it->second->second = value;
            m_list.splice(m_list.begin(), m_list, it->second);
            return;
        }

        // 淘汰最旧条目
        if (static_cast<i32>(m_list.size()) >= m_maxSize) {
            auto last = m_list.back();
            m_cache.erase(last.first);
            m_list.pop_back();
        }

        // 插入新条目到前面
        m_list.emplace_front(key, value);
        m_cache[key] = m_list.begin();
    }

    /**
     * @brief 打包 BlockPos 为键
     *
     * MC: BlockPos.asLong() — (x & 0x3FFFFFF) << 38 | (z & 0x3FFFFFF) << 12 | (y & 0xFFF)
     * 我们使用相同的打包方式
     */
    [[nodiscard]] static i64 packBlockPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFFLL) << 38 | (static_cast<i64>(z) & 0x3FFFFFFLL) << 12 |
            (static_cast<i64>(y) & 0xFFFL);
    }

    /**
     * @brief 获取缓存大小
     */
    [[nodiscard]] i32 size() const { return static_cast<i32>(m_list.size()); }

    /**
     * @brief 清除缓存
     */
    void clear()
    {
        m_list.clear();
        m_cache.clear();
    }

private:
    i32 m_maxSize;

    // 双向链表：前面是最新，后面是最旧
    using ListNode = std::pair<i64, f32>;
    mutable std::list<ListNode> m_list;

    // 哈希表：key → 链表迭代器
    mutable std::unordered_map<i64, std::list<ListNode>::iterator> m_cache;
};

} // namespace mc
