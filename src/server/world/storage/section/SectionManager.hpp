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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "server/world/storage/db/ConsistencyMode.hpp"
#include "server/world/storage/db/RocksDBDatabase.hpp"
#include "server/world/storage/db/SectionCodec.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mc::world::storage {

/**
 * @brief 待落盘的单个区块段
 *
 * 保存路径不经过 `SectionData`：`ChunkSection` 序列化后的字节直接放进本结构，
 * 由 `SectionManager::saveSectionsBatch` 原样写入 RocksDB。`SectionData` 只服务
 * 读取路径（反序列化中间表示），这样每段的常驻开销只有压缩后的字节数（约 1~5 KB），
 * 而非 `SectionData` 展开后的约 18.7 KB。
 */
struct SectionWrite {
    /// Section 标识
    SectionKey key;

    /// 已序列化（含 ZSTD 压缩）的落盘字节
    std::vector<u8> bytes;
};

/**
 * @brief Section管理器
 *
 * 负责单维度 Section 数据的批量读取与批量写回，是 `SingleLevelStorageManager`
 * 与 RocksDB 之间的直接桥梁，自身不持有任何常驻缓存。
 *
 * 线程安全：所有公共方法都是线程安全的。
 */
class SectionManager {
public:
    // ========================================================================
    // 配置
    // ========================================================================

    /// 管理器配置
    struct Config {
        /// 一致性模式
        ConsistencyMode consistencyMode = ConsistencyMode::Eventual;
    };

    /**
     * @brief 构造Section管理器
     *
     * @param db RocksDB数据库实例
     * @param dimension 维度ID
     * @param config 配置
     */
    SectionManager(RocksDBDatabase& db, DimensionId dimension, const Config& config);

    ~SectionManager() = default;

    // 禁止拷贝
    SectionManager(const SectionManager&) = delete;
    SectionManager& operator=(const SectionManager&) = delete;

    // 禁止移动（有引用成员）
    SectionManager(SectionManager&&) noexcept = delete;
    SectionManager& operator=(SectionManager&&) noexcept = delete;

    // ========================================================================
    // Section加载
    // ========================================================================

    /**
     * @brief 批量加载Section
     *
     * 对全部 key 执行一次 RocksDB 批量读取，不再做任何缓存命中判断。
     * 返回结果顺序与输入 keys 完全一致；某个位置返回空 shared_ptr 表示 Section 不存在。
     *
     * @param keys Section标识列表
     * @return 与输入顺序一致的加载结果列表
     */
    Result<std::vector<std::shared_ptr<const SectionData>>> loadSectionsSync(const std::vector<SectionKey>& keys);

    // ========================================================================
    // Section保存
    // ========================================================================

    /**
     * @brief 批量写回Section
     *
     * 把全部待写段聚合成**一个** `WriteBatch` 提交，一次调用只付一次 WAL fsync。
     * 逐段调用 `RocksDBDatabase::put` 会让每段各付一次 fsync——视野距离 16 下
     * 2048 个段实测约 6.6 秒，聚合后同一份数据只需几十毫秒。
     *
     * @param writes 待写段列表（键 + 已序列化字节）
     * @param sync 是否要求本次提交等待 fsync 落盘；为 false 时仍受一致性模式约束
     * @return 成功写回的段数
     */
    Result<size_t> saveSectionsBatch(const std::vector<SectionWrite>& writes, bool sync = false);

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
     * @brief 从数据库批量加载多个 Section
     *
     * @param keys Section 标识列表
     * @return 与输入顺序一致的加载结果列表
     */
    Result<std::vector<std::shared_ptr<const SectionData>>> _loadFromDatabaseBatch(const std::vector<SectionKey>& keys);

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
};

} // namespace mc::world::storage
