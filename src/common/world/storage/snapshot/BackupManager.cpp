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

#include "BackupManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <rocksdb/env.h>
#include <rocksdb/status.h>
#include <rocksdb/utilities/backup_engine.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::world::storage {

// ============================================================================
// BackupManager::Impl
// ============================================================================

struct BackupManager::Impl {
    std::unique_ptr<rocksdb::BackupEngine> engine;
};

// ============================================================================
// 构造与析构
// ============================================================================

BackupManager::BackupManager(const std::filesystem::path& backupDir)
    : m_impl(std::make_unique<Impl>())
    , m_backupDir(backupDir)
{}

BackupManager::~BackupManager() = default;

BackupManager::BackupManager(BackupManager&&) noexcept = default;
BackupManager& BackupManager::operator=(BackupManager&&) noexcept = default;

// ============================================================================
// 静态工厂方法
// ============================================================================

Result<std::unique_ptr<BackupManager>> BackupManager::open(const std::filesystem::path& backupDir)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Task, "BackupManager::open", "path", backupDir.string());

    // 创建备份目录
    try {
        std::filesystem::create_directories(backupDir);
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to create backup directory: {}", e.what()));
    }

    auto manager = std::unique_ptr<BackupManager>(new BackupManager(backupDir));

    // 打开备份引擎
    rocksdb::BackupEngineOptions options(backupDir.string());
    rocksdb::BackupEngine* engine = nullptr;
    rocksdb::Status status = rocksdb::BackupEngine::Open(rocksdb::Env::Default(), options, &engine);

    if (!status.ok()) {
        return Error(ErrorCode::RocksDBError, fmt::format("Failed to open backup engine: {}", status.ToString()));
    }

    manager->m_impl->engine.reset(engine);

    spdlog::info("BackupManager opened at {}", backupDir.string());
    return manager;
}

// ============================================================================
// 备份操作
// ============================================================================

Result<BackupID> BackupManager::createBackup(
    RocksDBDatabase& db, const std::string& name, const std::string& description)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Task, "BackupManager::createBackup", "name", name);

    if (!m_impl->engine) {
        return Error(ErrorCode::InvalidState, "Backup engine not initialized");
    }

    // 构建元数据 JSON
    std::string metadata;
    if (!name.empty() || !description.empty()) {
        metadata = fmt::format(R"({{"name":"{}","description":"{}","timestamp":{}}})",
            name,
            description,
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

    // 创建备份
    rocksdb::Status status;
    if (metadata.empty()) {
        status = m_impl->engine->CreateNewBackup(db.rawDB());
    } else {
        status = m_impl->engine->CreateNewBackupWithMetadata(db.rawDB(), metadata);
    }

    if (!status.ok()) {
        return Error(ErrorCode::RocksDBError, fmt::format("Failed to create backup: {}", status.ToString()));
    }

    // 获取最新的备份ID
    std::vector<rocksdb::BackupInfo> infos;
    m_impl->engine->GetBackupInfo(&infos);

    if (infos.empty()) {
        return Error(ErrorCode::OperationFailed, "No backup created");
    }

    BackupID id = static_cast<BackupID>(infos.back().backup_id);

    spdlog::info("Created backup {} with name '{}'", id, name);
    return id;
}

Result<std::vector<SnapshotMetadata>> BackupManager::listBackups()
{
    if (!m_impl->engine) {
        return Error(ErrorCode::InvalidState, "Backup engine not initialized");
    }

    std::vector<rocksdb::BackupInfo> infos;
    m_impl->engine->GetBackupInfo(&infos);

    std::vector<SnapshotMetadata> result;
    result.reserve(infos.size());

    for (const auto& info : infos) {
        SnapshotMetadata meta;
        meta.id = static_cast<BackupID>(info.backup_id);
        meta.timestamp = info.timestamp;
        meta.size = info.size;
        meta.fileCount = static_cast<u32>(info.number_files);
        meta.name = ""; // 从 metadata 解析
        meta.description = "";

        // 解析元数据
        if (!info.app_metadata.empty()) {
            // 简单解析 JSON 元数据
            // 格式: {"name":"xxx","description":"yyy","timestamp":123}
            auto parseField = [&info](const std::string& field) -> std::string {
                std::string key = "\"" + field + "\":\"";
                size_t start = info.app_metadata.find(key);
                if (start != std::string::npos) {
                    start += key.length();
                    size_t end = info.app_metadata.find("\"", start);
                    if (end != std::string::npos) {
                        return info.app_metadata.substr(start, end - start);
                    }
                }
                return "";
            };

            meta.name = parseField("name");
            meta.description = parseField("description");
        }

        result.push_back(meta);
    }

    // 按时间戳降序排序
    std::sort(result.begin(), result.end(), [](const SnapshotMetadata& a, const SnapshotMetadata& b) {
        return a.timestamp > b.timestamp;
    });

    return result;
}

Result<void> BackupManager::restoreBackup(BackupID id, const std::filesystem::path& targetDir)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Storage.Task, "BackupManager::restoreBackup", "id", id, "target", targetDir.string());

    if (!m_impl->engine) {
        return Error(ErrorCode::InvalidState, "Backup engine not initialized");
    }

    // 验证备份存在
    std::vector<rocksdb::BackupInfo> infos;
    m_impl->engine->GetBackupInfo(&infos);

    bool found = false;
    for (const auto& info : infos) {
        if (static_cast<BackupID>(info.backup_id) == id) {
            found = true;
            break;
        }
    }

    if (!found) {
        return Error(ErrorCode::SnapshotNotFound, fmt::format("Backup {} not found", id));
    }

    // 恢复备份
    rocksdb::Status status = m_impl->engine->RestoreDBFromBackup(static_cast<rocksdb::BackupID>(id),
        targetDir.string(), // db_dir
        targetDir.string()  // wal_dir
    );

    if (!status.ok()) {
        return Error(ErrorCode::SnapshotRestoreFailed, fmt::format("Failed to restore backup: {}", status.ToString()));
    }

    spdlog::info("Restored backup {} to {}", id, targetDir.string());
    return {};
}

Result<void> BackupManager::deleteBackup(BackupID id)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Task, "BackupManager::deleteBackup", "id", id);

    if (!m_impl->engine) {
        return Error(ErrorCode::InvalidState, "Backup engine not initialized");
    }

    rocksdb::Status status = m_impl->engine->DeleteBackup(static_cast<rocksdb::BackupID>(id));

    if (!status.ok()) {
        return Error(ErrorCode::RocksDBError, fmt::format("Failed to delete backup: {}", status.ToString()));
    }

    spdlog::info("Deleted backup {}", id);
    return {};
}

Result<size_t> BackupManager::pruneOldBackups(size_t keepCount)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Task, "BackupManager::pruneOldBackups", "keepCount", keepCount);

    if (!m_impl->engine) {
        return Error(ErrorCode::InvalidState, "Backup engine not initialized");
    }

    // 获取当前备份数量
    std::vector<rocksdb::BackupInfo> infos;
    m_impl->engine->GetBackupInfo(&infos);

    if (infos.size() <= keepCount) {
        return 0; // 无需删除
    }

    size_t toDelete = infos.size() - keepCount;

    // 删除最旧的备份
    // 注意：RocksDB 的 PurgeOldBackups 可以直接实现此功能
    rocksdb::Status status = m_impl->engine->PurgeOldBackups(static_cast<u32>(keepCount));

    if (!status.ok()) {
        return Error(ErrorCode::RocksDBError, fmt::format("Failed to purge old backups: {}", status.ToString()));
    }

    spdlog::info("Purged {} old backups, keeping {}", toDelete, keepCount);
    return toDelete;
}

Result<bool> BackupManager::verifyBackup(BackupID id)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Task, "BackupManager::verifyBackup", "id", id);

    if (!m_impl->engine) {
        return Error(ErrorCode::InvalidState, "Backup engine not initialized");
    }

    rocksdb::Status status = m_impl->engine->VerifyBackup(static_cast<rocksdb::BackupID>(id));

    if (!status.ok()) {
        if (status.IsNotFound()) {
            return false; // 备份不存在
        }
        return Error(ErrorCode::SnapshotCorrupted, fmt::format("Backup verification failed: {}", status.ToString()));
    }

    return true;
}

// ============================================================================
// 信息查询
// ============================================================================

size_t BackupManager::backupCount() const
{
    if (!m_impl->engine) {
        return 0;
    }

    std::vector<rocksdb::BackupInfo> infos;
    m_impl->engine->GetBackupInfo(&infos);
    return infos.size();
}

} // namespace mc::world::storage
