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

#include "../../../core/Result.hpp"
#include "ColumnFamilies.hpp"
#include "RocksDBConfig.hpp"
#include "SectionKey.hpp"
#include "common/core/Types.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/backup_engine.h>
#include <rocksdb/write_batch.h>

namespace mc::world::storage {

/**
 * @brief RocksDB数据库封装
 *
 * 提供对RocksDB数据库的高级封装，支持：
 * - 多列族操作
 * - Section数据的读写
 * - 批量写入
 * - 快照和备份
 * - 一致性模式配置
 *
 * 线程安全：RocksDB实例可被多线程共享访问。
 */
class RocksDBDatabase {
public:
    // ========================================================================
    // 工厂方法
    // ========================================================================

    /**
     * @brief 打开或创建数据库
     *
     * @param path 数据库路径
     * @param config 配置选项
     * @return 成功返回数据库实例，失败返回错误
     */
    static Result<std::unique_ptr<RocksDBDatabase>> open(
        const std::filesystem::path& path, const RocksDBConfig& config = RocksDBConfig{});

    /**
     * @brief 以只读模式打开数据库
     *
     * @param path 数据库路径
     * @return 成功返回数据库实例，失败返回错误
     */
    static Result<std::unique_ptr<RocksDBDatabase>> openReadOnly(const std::filesystem::path& path);

    // ========================================================================
    // 构造与析构
    // ========================================================================

    ~RocksDBDatabase();

    // 禁止拷贝
    RocksDBDatabase(const RocksDBDatabase&) = delete;
    RocksDBDatabase& operator=(const RocksDBDatabase&) = delete;

    // 允许移动
    RocksDBDatabase(RocksDBDatabase&&) noexcept;
    RocksDBDatabase& operator=(RocksDBDatabase&&) noexcept;

    // ========================================================================
    // 基础操作
    // ========================================================================

    /**
     * @brief 检查数据库是否已打开
     */
    [[nodiscard]] bool isOpen() const noexcept { return m_db != nullptr; }

    /**
     * @brief 获取数据库路径
     */
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const RocksDBConfig& config() const noexcept { return m_config; }

    /**
     * @brief 单点读取
     *
     * @param cfName 列族名
     * @param key 键
     * @return 成功返回值，失败返回错误
     */
    Result<std::vector<u8>> get(const std::string& cfName, const std::vector<u8>& key);

    /**
     * @brief 单点读取（Slice版本）
     */
    Result<std::vector<u8>> get(const std::string& cfName, const rocksdb::Slice& key);

    /**
     * @brief 单点写入
     *
     * @param cfName 列族名
     * @param key 键
     * @param value 值
     * @param sync 是否同步刷盘
     * @return 成功返回空，失败返回错误
     */
    Result<void> put(
        const std::string& cfName, const std::vector<u8>& key, const std::vector<u8>& value, bool sync = false);

    /**
     * @brief 单点写入（Slice版本）
     */
    Result<void> put(
        const std::string& cfName, const rocksdb::Slice& key, const rocksdb::Slice& value, bool sync = false);

    /**
     * @brief 删除键
     *
     * @param cfName 列族名
     * @param key 键
     * @return 成功返回空，失败返回错误
     */
    Result<void> del(const std::string& cfName, const std::vector<u8>& key);

    /**
     * @brief 删除键（Slice版本）
     */
    Result<void> del(const std::string& cfName, const rocksdb::Slice& key);

    /**
     * @brief 检查键是否存在
     *
     * @param cfName 列族名
     * @param key 键
     * @return 存在返回true，不存在返回false
     */
    [[nodiscard]] bool exists(const std::string& cfName, const std::vector<u8>& key);

    // ========================================================================
    // 批量操作
    // ========================================================================

    /**
     * @brief 批量写入
     *
     * @param batch 写入批次
     * @param sync 是否同步刷盘
     * @return 成功返回空，失败返回错误
     */
    Result<void> writeBatch(rocksdb::WriteBatch& batch, bool sync = false);

    /**
     * @brief 同列族批量读取
     *
     * @param cfName 列族名
     * @param keys 键列表
     * @return 与输入顺序一致的读取结果列表
     */
    Result<std::vector<Result<std::vector<u8>>>> multiGet(
        const std::string& cfName, const std::vector<std::vector<u8>>& keys);

    // ========================================================================
    // 范围操作
    // ========================================================================

    /**
     * @brief 创建迭代器
     *
     * @param cfName 列族名
     * @return 迭代器
     */
    [[nodiscard]] std::unique_ptr<rocksdb::Iterator> newIterator(const std::string& cfName);

    /**
     * @brief 范围删除
     *
     * @param cfName 列族名
     * @param startKey 起始键（包含）
     * @param endKey 结束键（不包含）
     * @return 成功返回空，失败返回错误
     */
    Result<void> deleteRange(const std::string& cfName, const std::vector<u8>& startKey, const std::vector<u8>& endKey);

    // ========================================================================
    // 快照
    // ========================================================================

    /**
     * @brief 创建快照
     *
     * 注意：RocksDB快照是内存中的sequence number，
     * 不持久化，重启后丢失。
     * 长期持有会阻止压缩清理旧数据。
     *
     * @return 快照指针
     */
    [[nodiscard]] const rocksdb::Snapshot* createSnapshot();

    /**
     * @brief 释放快照
     *
     * @param snapshot 快照指针
     */
    void releaseSnapshot(const rocksdb::Snapshot* snapshot);

    // ========================================================================
    // 备份
    // ========================================================================

    /**
     * @brief 创建备份
     *
     * 使用RocksDB BackupEngine创建增量备份。
     *
     * @param backupDir 备份目录
     * @param metadata 元数据（JSON格式）
     * @return 成功返回备份ID，失败返回错误
     */
    Result<u64> createBackup(const std::filesystem::path& backupDir, const std::string& metadata = "");

    /**
     * @brief 从备份恢复
     *
     * @param backupDir 备份目录
     * @param backupId 备份ID（0表示最新）
     * @param targetDir 目标目录
     * @return 成功返回空，失败返回错误
     */
    Result<void> restoreFromBackup(
        const std::filesystem::path& backupDir, u64 backupId, const std::filesystem::path& targetDir);

    // ========================================================================
    // 管理操作
    // ========================================================================

    /**
     * @brief 手动压缩指定列族
     *
     * @param cfName 列族名
     * @return 成功返回空，失败返回错误
     */
    Result<void> compact(const std::string& cfName);

    /**
     * @brief 刷新MemTable到磁盘
     *
     * @param cfName 列族名（空表示所有列族）
     * @param sync 是否同步等待
     * @return 成功返回空，失败返回错误
     */
    Result<void> flush(const std::string& cfName = "", bool sync = true);

    /**
     * @brief 关闭数据库
     */
    void close();

    /**
     * @brief 获取统计信息字符串
     */
    [[nodiscard]] std::string getStatistics() const;

    /**
     * @brief 获取属性值
     *
     * @param cfName 列族名
     * @param property 属性名
     * @return 属性值，不存在返回空
     */
    [[nodiscard]] std::string getProperty(const std::string& cfName, const std::string& property);

    // ========================================================================
    // 列族管理
    // ========================================================================

    /**
     * @brief 获取列族句柄
     *
     * @param cfName 列族名
     * @return 列族句柄，不存在返回nullptr
     */
    [[nodiscard]] rocksdb::ColumnFamilyHandle* getCF(const std::string& cfName);

    /**
     * @brief 检查列族是否存在
     */
    [[nodiscard]] bool hasColumnFamily(const std::string& cfName) const;

    /**
     * @brief 获取所有列族名
     */
    [[nodiscard]] std::vector<std::string> listColumnFamilies() const;

    /**
     * @brief 获取原始 RocksDB 实例
     *
     * 注意：此方法仅供内部模块使用（如 BackupManager）。
     * 外部代码应通过 RocksDBDatabase 的方法操作数据库。
     *
     * @return RocksDB 实例指针
     */
    [[nodiscard]] rocksdb::DB* rawDB() { return m_db; }
    [[nodiscard]] const rocksdb::DB* rawDB() const { return m_db; }

private:
    /**
     * @brief 私有构造函数
     */
    RocksDBDatabase(const std::filesystem::path& path, const RocksDBConfig& config);

    /**
     * @brief 初始化列族
     *
     * 在打开数据库后，将 RocksDB 返回的列族句柄与期望的列族名称对应存储。
     * 对于新建数据库，所有定义在 cf::ALL_COLUMN_FAMILIES 中的列族都会被创建。
     * 对于已有数据库，缺失的列族会被自动补充。
     *
     * @param cfDescriptors 传给 DB::Open 的列族描述符列表
     * @param cfHandles DB::Open 返回的列族句柄列表（与 cfDescriptors 一一对应）
     * @return 成功返回空，失败返回错误
     */
    Result<void> _initializeColumnFamilies(const std::vector<rocksdb::ColumnFamilyDescriptor>& cfDescriptors,
        const std::vector<rocksdb::ColumnFamilyHandle*>& cfHandles);

    /**
     * @brief 构建列族描述符列表
     *
     * 根据数据库是否已存在（新数据库 vs 已有数据库），构建合适的列族描述符。
     * 新数据库创建所有定义的列族；已有数据库打开已有列族并补充缺失的列族。
     *
     * @param dbExists 数据库是否已存在
     * @return 列族描述符列表
     */
    [[nodiscard]] std::vector<rocksdb::ColumnFamilyDescriptor> _buildColumnFamilyDescriptors(bool dbExists);

    /**
     * @brief 销毁所有列族句柄
     *
     * 在关闭数据库前调用，逐个销毁列族句柄以释放 RocksDB 资源。
     * 必须在 delete m_db 之前调用。
     */
    void _destroyColumnFamilyHandles();

    /**
     * @brief 创建默认列族选项
     */
    [[nodiscard]] rocksdb::ColumnFamilyOptions _createCFOptions() const;

    // 成员变量
    rocksdb::DB* m_db = nullptr;
    std::filesystem::path m_path;
    RocksDBConfig m_config;
    std::unordered_map<std::string, rocksdb::ColumnFamilyHandle*> m_cfHandles;
};

} // namespace mc::world::storage
