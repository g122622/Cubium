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

#include "common/core/Types.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::storage {

// 前向声明
struct SectionData;

/**
 * @brief Section LRU缓存
 *
 * 线程安全的LRU缓存，用于缓存已加载的Section数据。
 * 缓存容量按Section数量限制，当超过容量时自动淘汰最久未使用的Section。
 *
 * 特性：
 * - 线程安全（使用互斥锁保护）
 * - O(1) 时间复杂度的访问和更新
 * - 支持手动驱逐和清空
 * - 提供缓存统计信息
 */
class SectionCache {
public:
    // ========================================================================
    // 类型定义
    // ========================================================================

    /// 缓存条目
    struct CacheEntry {
        /// Section数据
        std::shared_ptr<SectionData> data;

        /// 是否已修改（脏标记）
        bool dirty = false;

        /// 最后访问时间戳（毫秒）
        u64 lastAccessTime = 0;
    };

    /// 缓存统计信息
    struct CacheStats {
        /// 缓存命中次数
        size_t hits = 0;

        /// 缓存未命中次数
        size_t misses = 0;

        /// 驱逐次数
        size_t evictions = 0;

        /// 当前缓存大小
        size_t currentSize = 0;

        /// 缓存容量
        size_t capacity = 0;

        /// 命中率
        [[nodiscard]] double hitRate() const
        {
            size_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;
        }
    };

    // ========================================================================
    // 构造与析构
    // ========================================================================

    /**
     * @brief 构造Section缓存
     *
     * @param capacity 最大缓存的Section数量
     */
    explicit SectionCache(size_t capacity = 1024);

    ~SectionCache() = default;

    // 禁止拷贝
    SectionCache(const SectionCache&) = delete;
    SectionCache& operator=(const SectionCache&) = delete;

    // 禁止移动（包含 std::mutex）
    SectionCache(SectionCache&&) noexcept = delete;
    SectionCache& operator=(SectionCache&&) noexcept = delete;

    // ========================================================================
    // 缓存操作
    // ========================================================================

    /**
     * @brief 获取Section
     *
     * 如果Section在缓存中，返回数据并更新LRU顺序。
     * 否则返回nullptr。
     *
     * @param key Section标识
     * @return Section数据，不存在返回nullptr
     */
    [[nodiscard]] std::shared_ptr<SectionData> get(const SectionKey& key);

    /**
     * @brief 放入Section
     *
     * 将Section放入缓存。如果缓存已满，淘汰最久未使用的Section。
     * 如果Section已存在，更新数据并移动到最前。
     *
     * @param key Section标识
     * @param data Section数据
     * @param dirty 是否标记为脏
     * @return 被驱逐的Section（如果有）
     */
    std::shared_ptr<SectionData> put(const SectionKey& key, std::shared_ptr<SectionData> data, bool dirty = false);

    /**
     * @brief 检查Section是否存在
     *
     * @param key Section标识
     * @return 是否存在
     */
    [[nodiscard]] bool contains(const SectionKey& key) const;

    /**
     * @brief 驱逐Section
     *
     * 从缓存中移除Section，但不删除数据。
     *
     * @param key Section标识
     * @return 被驱逐的Section数据，不存在返回nullptr
     */
    std::shared_ptr<SectionData> evict(const SectionKey& key);

    /**
     * @brief 清空缓存
     *
     * 移除所有缓存的Section。
     *
     * @return 被清空的脏Section列表
     */
    std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> clear();

    // ========================================================================
    // 脏标记管理
    // ========================================================================

    /**
     * @brief 标记Section为脏
     *
     * @param key Section标识
     * @return 是否成功（Section必须在缓存中）
     */
    bool markDirty(const SectionKey& key);

    /**
     * @brief 标记Section为干净
     *
     * @param key Section标识
     * @return 是否成功
     */
    bool markClean(const SectionKey& key);

    /**
     * @brief 检查Section是否为脏
     *
     * @param key Section标识
     * @return 是否为脏（不存在返回false）
     */
    [[nodiscard]] bool isDirty(const SectionKey& key) const;

    /**
     * @brief 获取所有脏Section
     *
     * @return 脏Section键列表
     */
    [[nodiscard]] std::vector<SectionKey> getDirtyKeys() const;

    /**
     * @brief 获取所有脏Section数据
     *
     * @return 脏Section数据列表
     */
    [[nodiscard]] std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> getDirtySections() const;

    /**
     * @brief 获取所有缓存Section数据
     *
     * @return 所有缓存Section数据列表
     */
    [[nodiscard]] std::vector<std::pair<SectionKey, std::shared_ptr<SectionData>>> getAllSections() const;

    // ========================================================================
    // 缓存管理
    // ========================================================================

    /**
     * @brief 设置缓存容量
     *
     * 如果新容量小于当前缓存大小，会驱逐多余的Section。
     *
     * @param capacity 新容量
     */
    void setCapacity(size_t capacity);

    /**
     * @brief 获取缓存容量
     */
    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }

    /**
     * @brief 获取当前缓存大小
     */
    [[nodiscard]] size_t size() const noexcept;

    /**
     * @brief 检查缓存是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    /**
     * @brief 获取缓存统计信息
     */
    [[nodiscard]] CacheStats getStats() const;

    /**
     * @brief 重置缓存统计
     */
    void resetStats();

private:
    // ========================================================================
    // 内部类型
    // ========================================================================

    /// LRU列表中的条目
    struct ListNode {
        SectionKey key;
        CacheEntry entry;
    };

    /// LRU列表类型
    using LRUList = std::list<ListNode>;

    /// LRU列表迭代器类型
    using LRUIterator = LRUList::iterator;

    /// 缓存映射类型
    using CacheMap = std::unordered_map<SectionKey, LRUIterator, SectionKey::Hash>;

    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 更新访问顺序
     *
     * 将Section移动到LRU列表最前。
     *
     * @param it 列表迭代器
     */
    void _updateAccessOrder(LRUIterator it);

    /**
     * @brief 驱逐最久未使用的Section
     *
     * @return 被驱逐的Section
     */
    std::shared_ptr<SectionData> _evictLRU();

    /**
     * @brief 获取当前时间戳（毫秒）
     */
    [[nodiscard]] static u64 _getCurrentTimeMs();

    // ========================================================================
    // 成员变量
    // ========================================================================

    /// 缓存容量
    size_t m_capacity;

    /// LRU列表（最前为最近使用，最后为最久未使用）
    mutable LRUList m_lruList;

    /// 缓存映射
    mutable CacheMap m_cacheMap;

    /// 互斥锁
    mutable std::mutex m_mutex;

    /// 统计信息
    mutable CacheStats m_stats;
};

} // namespace mc::world::storage
