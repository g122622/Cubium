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

#pragma once

#include "../../core/Types.hpp"
#include <list>
#include <mutex>
#include <unordered_map>

namespace mc {

/**
 * @brief LRU 缓存实现（链表+哈希表）
 *
 * 使用 std::unordered_map + std::list 实现 O(1) LRU 缓存。
 * 保留用于性能对比测试。
 *
 * 参考 MC 1.16.5 Long2IntLinkedOpenHashMap 的设计思想。
 *
 * 用法示例：
 * @code
 * Long2IntLRUCache cache(1024);
 *
 * // 存储值
 * cache.put(Long2IntLRUCache::packCoords(10, 20), 42);
 *
 * // 获取值
 * i32 value;
 * if (cache.get(Long2IntLRUCache::packCoords(10, 20), value)) {
 *     // 命中
 * }
 * @endcode
 */
class Long2IntLRUCache {
public:
    /**
     * @brief 构造 LRU 缓存
     * @param maxSize 最大缓存条目数
     */
    explicit Long2IntLRUCache(i32 maxSize = 1024);

    /**
     * @brief 获取缓存值（会更新访问顺序）
     * @param key 坐标键（打包的 x, z）
     * @param value 输出值
     * @return 是否找到
     */
    [[nodiscard]] bool get(i64 key, i32& value);

    /**
     * @brief 设置缓存值
     * @param key 坐标键
     * @param value 值
     */
    void put(i64 key, i32 value);

    /**
     * @brief 打包坐标为键
     * @param x X 坐标
     * @param z Z 坐标
     * @return 打包的键
     *
     * @note 使用位运算打包，高 32 位为 x，低 32 位为 z
     */
    [[nodiscard]] static i64 packCoords(i32 x, i32 z)
    {
        return (static_cast<i64>(x) << 32) | (static_cast<i64>(z) & 0xFFFFFFFFLL);
    }

    /**
     * @brief 获取缓存大小
     */
    [[nodiscard]] i32 size() const;

    /**
     * @brief 清除缓存
     */
    void clear();

    /**
     * @brief 获取互斥锁（用于批量操作）
     */
    std::mutex& getMutex() const { return m_mutex; }

    /**
     * @brief 获取缓存值（调用方已持有锁）
     * @param key 坐标键
     * @param value 输出值
     * @return 是否找到
     *
     * @note 用于批量操作时避免重复加锁
     */
    [[nodiscard]] bool getLocked(i64 key, i32& value);

    /**
     * @brief 设置缓存值（调用方已持有锁）
     * @param key 坐标键
     * @param value 值
     *
     * @note 用于批量操作时避免重复加锁
     */
    void putLocked(i64 key, i32 value);

private:
    i32 m_maxSize;

    // 双向链表节点：存储 (key, value)
    using ListNode = std::pair<i64, i32>;
    mutable std::list<ListNode> m_list; // 前面是最新，后面是最旧

    // 哈希表：key -> 链表迭代器
    mutable std::unordered_map<i64, std::list<ListNode>::iterator> m_cache;

    mutable std::mutex m_mutex;
};

} // namespace mc
