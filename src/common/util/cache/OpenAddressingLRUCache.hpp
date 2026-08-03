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
#include <atomic>
#include <cstddef>
#include <list>
#include <mutex>
#include <vector>

namespace mc {

/**
 * @brief 开放寻址 LRU 缓存
 *
 * 使用开放寻址哈希表实现，比 std::unordered_map + std::list 更高效。
 * 参考 MC Java 的 Long2IntLinkedOpenHashMap 设计思想。
 *
 * 性能优势：
 * - 无指针追踪开销（连续内存布局）
 * - 更好的缓存局部性
 * - FIB 哈希分布更均匀
 * - 批量淘汰减少频繁操作
 *
 * 用法示例：
 * @code
 * OpenAddressingLRUCache cache(1024);
 *
 * // 存储值
 * cache.put(OpenAddressingLRUCache::packCoords(10, 20), 42);
 *
 * // 获取值
 * i32 value;
 * if (cache.get(OpenAddressingLRUCache::packCoords(10, 20), value)) {
 *     // 命中
 * }
 * @endcode
 */
class OpenAddressingLRUCache {
public:
    /**
     * @brief 构造开放寻址 LRU 缓存
     * @param maxSize 最大缓存条目数（实际容量为不小于 maxSize 的 2 的幂次）
     */
    explicit OpenAddressingLRUCache(i32 maxSize = 1024);

    /**
     * @brief 获取缓存值
     * @param key 坐标键
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
     * @note 使用 XOR 打包，比 OR 更分散
     */
    [[nodiscard]] static i64 packCoords(i32 x, i32 z) { return (static_cast<i64>(x) << 32) ^ static_cast<i64>(z); }

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

    /**
     * @brief 获取命中次数（用于性能分析）
     */
    [[nodiscard]] u64 hitCount() const { return m_hitCount.load(); }

    /**
     * @brief 获取未命中次数（用于性能分析）
     */
    [[nodiscard]] u64 missCount() const { return m_missCount.load(); }

    /**
     * @brief 重置统计计数器
     */
    void resetStats()
    {
        m_hitCount.store(0);
        m_missCount.store(0);
    }

private:
    /**
     * @brief 缓存条目
     */
    struct Entry {
        i64 key = 0;           ///< 坐标键
        i32 value = 0;         ///< 缓存值
        u32 timestamp = 0;     ///< 访问时间戳（用于 LRU）
        bool occupied = false; ///< 是否被占用
    };

    /**
     * @brief 计算哈希索引
     * @param key 键
     * @return 哈希表索引
     *
     * @note 使用 FIB 哈希，比取模更均匀
     */
    [[nodiscard]] size_t hashIndex(i64 key) const;

    /**
     * @brief 批量淘汰旧条目
     *
     * 淘汰 capacity / EVICT_DIVISOR 个最旧条目
     */
    void evictBatch();

    /**
     * @brief 查找条目位置
     * @param key 键
     * @return 条目指针，如果不存在返回 nullptr
     */
    [[nodiscard]] Entry* findEntry(i64 key);

    /**
     * @brief 查找空槽位
     * @param key 键（用于确定起始位置）
     * @return 空槽位指针
     */
    [[nodiscard]] Entry* findEmptySlot(i64 key);

    std::vector<Entry> m_table; ///< 哈希表
    u32 m_capacity;             ///< 表容量（2 的幂次）
    u32 m_maxSize;              ///< 最大条目数
    u32 m_shift;                ///< 用于哈希的位移量（64 - log2(capacity)）
    std::atomic<u32> m_size{0}; ///< 当前条目数
    u32 m_timestamp = 0;        ///< 全局时间戳计数器

    mutable std::mutex m_mutex;

    // 性能统计
    std::atomic<u64> m_hitCount{0};
    std::atomic<u64> m_missCount{0};

    // FIB 哈希乘数（黄金比例的 64 位近似）
    static constexpr u64 FIB_MULTIPLIER = 11400714819323198485ULL;

    // 批量淘汰除数（每次淘汰 capacity / EVICT_DIVISOR 个条目）
    static constexpr u32 EVICT_DIVISOR = 16;

    // 最大负载因子（超过则淘汰）
    static constexpr f32 MAX_LOAD_FACTOR = 0.75f;
};

} // namespace mc
