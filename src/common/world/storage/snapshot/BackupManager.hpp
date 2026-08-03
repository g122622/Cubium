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
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace mc::world::storage {

// 前向声明
class RocksDBDatabase;

/**
 * @brief 备份ID类型
 */
using BackupID = u64;

/**
 * @brief 快照元数据
 */
struct SnapshotMetadata {
    BackupID id;             ///< 备份ID
    i64 timestamp;           ///< 创建时间戳（毫秒）
    size_t size;             ///< 总大小（字节）
    u32 fileCount;           ///< 文件数量
    std::string name;        ///< 显示名称
    std::string description; ///< 描述
};

/**
 * @brief 备份管理器
 *
 * 封装 RocksDB BackupEngine，提供世界快照的创建、恢复和管理功能。
 * 支持增量备份，只复制变化的数据文件。
 *
 * 使用示例：
 * @code
 * auto backupMgr = BackupManager::open("/path/to/backups");
 * if (backupMgr.success()) {
 *     auto id = backupMgr.value()->createBackup(db, "Before Update");
 *     // ...
 *     backupMgr.value()->restoreBackup(id, "/path/to/restore");
 * }
 * @endcode
 */
class BackupManager {
public:
    /**
     * @brief 打开备份引擎
     *
     * @param backupDir 备份目录路径
     * @return 成功返回 BackupManager 实例，失败返回错误
     */
    static Result<std::unique_ptr<BackupManager>> open(const std::filesystem::path& backupDir);

    /**
     * @brief 析构函数
     */
    ~BackupManager();

    // 禁止拷贝
    BackupManager(const BackupManager&) = delete;
    BackupManager& operator=(const BackupManager&) = delete;

    // 允许移动
    BackupManager(BackupManager&&) noexcept;
    BackupManager& operator=(BackupManager&&) noexcept;

    // ========== 备份操作 ==========

    /**
     * @brief 创建备份
     *
     * @param db 数据库实例
     * @param name 备份名称
     * @param description 可选描述
     * @return 成功返回备份ID，失败返回错误
     */
    Result<BackupID> createBackup(RocksDBDatabase& db, const std::string& name, const std::string& description = "");

    /**
     * @brief 列出所有备份
     *
     * @return 备份元数据列表，按时间戳降序排列
     */
    Result<std::vector<SnapshotMetadata>> listBackups();

    /**
     * @brief 恢复备份
     *
     * 将数据库恢复到指定备份状态。目标目录必须为空或不存在。
     *
     * @param id 备份ID
     * @param targetDir 恢复目标目录
     * @return 成功或错误
     */
    Result<void> restoreBackup(BackupID id, const std::filesystem::path& targetDir);

    /**
     * @brief 删除备份
     *
     * @param id 备份ID
     * @return 成功或错误
     */
    Result<void> deleteBackup(BackupID id);

    /**
     * @brief 清理旧备份
     *
     * 保留最近 N 个备份，删除其余备份。
     *
     * @param keepCount 保留数量
     * @return 成功删除的备份数量，或错误
     */
    Result<size_t> pruneOldBackups(size_t keepCount);

    /**
     * @brief 验证备份完整性
     *
     * @param id 备份ID
     * @return 成功返回是否有效，失败返回错误
     */
    Result<bool> verifyBackup(BackupID id);

    // ========== 信息查询 ==========

    /**
     * @brief 获取备份目录
     */
    [[nodiscard]] const std::filesystem::path& backupDir() const { return m_backupDir; }

    /**
     * @brief 获取备份数量
     */
    [[nodiscard]] size_t backupCount() const;

private:
    /**
     * @brief 私有构造函数
     */
    explicit BackupManager(const std::filesystem::path& backupDir);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::filesystem::path m_backupDir;
};

} // namespace mc::world::storage
