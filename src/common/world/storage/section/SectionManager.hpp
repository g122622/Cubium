#pragma once

#include "../db/RocksDBDatabase.hpp"
#include "../db/SectionKey.hpp"
#include "../db/SectionCodec.hpp"
#include "../db/ConsistencyMode.hpp"
#include "SectionCache.hpp"
#include "../../../core/Result.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/thread/ServerWorkerPool.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <future>
#include <functional>

namespace mc::world::storage {

/**
 * @brief Section管理器
 *
 * 负责Section数据的加载、保存、缓存管理。
 * 与RocksDB数据库和区块系统交互。
 *
 * 线程安全：所有公共方法都是线程安全的。
 *
 * 特性：
 * - LRU缓存，自动淘汰未使用的Section
 * - 脏标记追踪，批量保存
 * - 异步加载/保存，优先级调度
 * - Perfetto追踪集成
 */
class SectionManager {
public:
    // ========================================================================
    // 配置
    // ========================================================================

    /// 管理器配置
    struct Config {
        /// 缓存容量（Section数量）
        size_t cacheCapacity = 1024;

        /// 是否预计算哈希（用于快照去重）
        bool computeHash = true;

        /// 自动保存脏Section阈值
        size_t autoSaveThreshold = 100;

        /// 批量保存大小
        size_t batchSize = 50;

        /// 一致性模式
        ConsistencyMode consistencyMode = ConsistencyMode::Eventual;

        /// 默认配置
        static Config Default() { return Config{}; }
    };

    // ========================================================================
    // 回调类型
    // ========================================================================

    /// 加载完成回调 - 使用指针避免 Result 不可拷贝问题
    using LoadCallback = std::function<void(const SectionKey&, SectionData*)>;

    /// 保存完成回调 - 使用 bool 表示成功/失败
    using SaveCallback = std::function<void(const SectionKey&, bool success)>;

    // ========================================================================
    // 构造与析构
    // ========================================================================

    /**
     * @brief 构造Section管理器
     *
     * @param db RocksDB数据库实例
     * @param dimension 维度ID
     * @param config 配置
     */
    SectionManager(
        RocksDBDatabase& db,
        DimensionId dimension,
        const Config& config
    );

    ~SectionManager() = default;

    // 禁止拷贝
    SectionManager(const SectionManager&) = delete;
    SectionManager& operator=(const SectionManager&) = delete;

    // 允许移动
    SectionManager(SectionManager&&) noexcept = default;
    SectionManager& operator=(SectionManager&&) noexcept = default;

    // ========================================================================
    // Section加载
    // ========================================================================

    /**
     * @brief 同步加载Section
     *
     * 优先从缓存加载，缓存未命中则从数据库加载。
     *
     * @param key Section标识
     * @return Section数据指针，失败返回错误
     */
    Result<SectionData*> loadSection(const SectionKey& key);

    /**
     * @brief 异步加载Section
     *
     * 提交加载任务到工作线程池。
     *
     * @param key Section标识
     * @param priority 任务优先级
     * @return 未来的Section数据
     */
    std::future<Result<SectionData*>> loadSectionAsync(
        const SectionKey& key,
        util::TaskPriority priority = util::TaskPriority::Normal
    );

    /**
     * @brief 批量加载Section
     *
     * @param keys Section标识列表
     * @param callback 加载完成回调
     */
    void loadSections(
        const std::vector<SectionKey>& keys,
        LoadCallback callback
    );

    // ========================================================================
    // Section保存
    // ========================================================================

    /**
     * @brief 保存Section
     *
     * 将Section数据写入数据库并标记为干净。
     *
     * @param key Section标识
     * @param data Section数据
     * @param immediate 是否立即同步写入
     * @return 成功或错误
     */
    Result<void> saveSection(
        const SectionKey& key,
        const SectionData& data,
        bool immediate = false
    );

    /**
     * @brief 异步保存Section
     *
     * @param key Section标识
     * @param data Section数据
     * @param priority 任务优先级
     * @return 未来的保存结果
     */
    std::future<Result<void>> saveSectionAsync(
        const SectionKey& key,
        const SectionData& data,
        util::TaskPriority priority = util::TaskPriority::Normal
    );

    /**
     * @brief 批量保存脏Section
     *
     * 将所有标记为脏的Section保存到数据库。
     *
     * @return 保存的Section数量
     */
    Result<size_t> flushDirtySections();

    /**
     * @brief 保存所有缓存Section
     *
     * 无论是否脏都保存。
     *
     * @return 保存的Section数量
     */
    Result<size_t> saveAll();

    // ========================================================================
    // Section卸载
    // ========================================================================

    /**
     * @brief 卸载Section
     *
     * 从缓存中移除Section。如果Section为脏，先保存到数据库。
     *
     * @param key Section标识
     * @return 成功或错误
     */
    Result<void> unloadSection(const SectionKey& key);

    /**
     * @brief 卸载所有Section
     *
     * 保存所有脏Section并清空缓存。
     *
     * @return 成功或错误
     */
    Result<void> unloadAll();

    // ========================================================================
    // Section删除
    // ========================================================================

    /**
     * @brief 删除Section
     *
     * 从缓存和数据库中删除Section。
     *
     * @param key Section标识
     * @return 成功或错误
     */
    Result<void> deleteSection(const SectionKey& key);

    /**
     * @brief 删除区块的所有Section
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 删除的Section数量
     */
    Result<size_t> deleteChunkSections(i32 chunkX, i32 chunkZ);

    // ========================================================================
    // 缓存管理
    // ========================================================================

    /**
     * @brief 设置缓存容量
     */
    void setCacheCapacity(size_t capacity);

    /**
     * @brief 获取缓存统计
     */
    [[nodiscard]] SectionCache::CacheStats getCacheStats() const;

    /**
     * @brief 清空缓存
     *
     * 注意：不保存脏Section，直接丢弃。
     */
    void clearCache();

    /**
     * @brief 检查Section是否在缓存中
     */
    [[nodiscard]] bool isCached(const SectionKey& key) const;

    // ========================================================================
    // 脏标记追踪
    // ========================================================================

    /**
     * @brief 标记Section为脏
     *
     * @param key Section标识
     * @return 是否成功（Section必须在缓存中）
     */
    bool markDirty(const SectionKey& key);

    /**
     * @brief 获取脏Section数量
     */
    [[nodiscard]] size_t getDirtyCount() const;

    /**
     * @brief 获取所有脏Section键
     */
    [[nodiscard]] std::vector<SectionKey> getDirtyKeys() const;

    // ========================================================================
    // 访问器
    // ========================================================================

    /**
     * @brief 获取维度ID
     */
    [[nodiscard]] DimensionId dimension() const noexcept { return m_dimension; }

    /**
     * @brief 获取列族名
     */
    [[nodiscard]] const std::string& columnFamily() const noexcept { return m_cfName; }

    /**
     * @brief 获取数据库引用
     */
    [[nodiscard]] RocksDBDatabase& database() noexcept { return m_db; }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const Config& config() const noexcept { return m_config; }

private:
    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 从数据库加载Section
     */
    Result<SectionData*> loadFromDatabase(const SectionKey& key);

    /**
     * @brief 保存Section到数据库
     */
    Result<void> saveToDatabase(
        const SectionKey& key,
        const SectionData& data,
        bool sync = false
    );

    /**
     * @brief 删除Section范围
     */
    Result<void> deleteSectionRange(
        const std::vector<u8>& startKey,
        const std::vector<u8>& endKey
    );

    // ========================================================================
    // 成员变量
    // ========================================================================

    /// 数据库引用
    RocksDBDatabase& m_db;

    /// 维度ID
    DimensionId m_dimension;

    /// 列族名
    std::string m_cfName;

    /// 配置
    Config m_config;

    /// LRU缓存
    mutable SectionCache m_cache;

    /// 脏Section集合（用于快速查询）
    mutable std::unordered_set<SectionKey, SectionKey::Hash> m_dirtySet;

    /// 脏集合互斥锁
    mutable std::mutex m_dirtyMutex;
};

} // namespace mc::world::storage
