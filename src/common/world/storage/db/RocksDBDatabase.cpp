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

#include "RocksDBDatabase.hpp"
#include "../../../perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <rocksdb/utilities/backup_engine.h>
#include <rocksdb/write_batch.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

RocksDBDatabase::RocksDBDatabase(const std::filesystem::path& path, const RocksDBConfig& config)
    : m_path(path)
    , m_config(config)
{}

RocksDBDatabase::~RocksDBDatabase()
{
    close();
}

RocksDBDatabase::RocksDBDatabase(RocksDBDatabase&& other) noexcept
    : m_db(other.m_db)
    , m_path(std::move(other.m_path))
    , m_config(std::move(other.m_config))
    , m_cfHandles(std::move(other.m_cfHandles))
{
    other.m_db = nullptr;
}

RocksDBDatabase& RocksDBDatabase::operator=(RocksDBDatabase&& other) noexcept
{
    if (this != &other) {
        close();
        m_db = other.m_db;
        m_path = std::move(other.m_path);
        m_config = std::move(other.m_config);
        m_cfHandles = std::move(other.m_cfHandles);
        other.m_db = nullptr;
    }
    return *this;
}

// ============================================================================
// 工厂方法
// ============================================================================

Result<std::unique_ptr<RocksDBDatabase>> RocksDBDatabase::open(
    const std::filesystem::path& path, const RocksDBConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::open", "path", path.string());

    auto db = std::unique_ptr<RocksDBDatabase>(new RocksDBDatabase(path, config));

    // 创建数据库选项
    rocksdb::DBOptions dbOptions = config.createDBOptions();

    // 检查数据库是否已存在（通过检查 CURRENT 文件）
    std::filesystem::path currentFile = path / "CURRENT";
    bool dbExists = std::filesystem::exists(currentFile);

    if (!dbExists) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::open::new", "event", "Creating new database");
        spdlog::info("Creating new database at {}", path.string());

        // 确保目录存在
        std::error_code ec;
        if (!std::filesystem::exists(path)) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db,
                "RocksDBDatabase::open::create_directories",
                "event",
                "Creating database directory");

            if (!std::filesystem::create_directories(path, ec)) {
                return Error(
                    ErrorCode::FileOpenFailed, fmt::format("Failed to create database directory: {}", ec.message()));
            }
        }
    } else {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Storage.Db, "RocksDBDatabase::open::existing", "event", "Opening existing database");
        spdlog::info("Opening existing database at {}", path.string());
    }

    // 构建列族描述符
    std::vector<rocksdb::ColumnFamilyDescriptor> cfDescriptors = db->_buildColumnFamilyDescriptors(dbExists);

    // 打开数据库
    std::vector<rocksdb::ColumnFamilyHandle*> cfHandles;
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "rocksdb::DB::Open", "path", path.string());

        rocksdb::Status status = rocksdb::DB::Open(dbOptions, path.string(), cfDescriptors, &cfHandles, &db->m_db);
        if (!status.ok()) {
            return Error(ErrorCode::FileOpenFailed, fmt::format("Failed to open database: {}", status.ToString()));
        }
    }

    // 初始化列族句柄映射
    auto initResult = db->_initializeColumnFamilies(cfDescriptors, cfHandles);
    if (!initResult.success()) {
        // 初始化失败时销毁列族句柄并关闭数据库
        for (auto* handle : cfHandles) {
            if (handle != nullptr && db->m_db != nullptr) {
                db->m_db->DestroyColumnFamilyHandle(handle);
            }
        }
        delete db->m_db;
        db->m_db = nullptr;
        return initResult.error();
    }

    spdlog::info("Database opened successfully with {} column families", db->m_cfHandles.size());

    return db;
}

Result<std::unique_ptr<RocksDBDatabase>> RocksDBDatabase::openReadOnly(const std::filesystem::path& path)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::openReadOnly", "path", path.string());

    RocksDBConfig config;
    auto db = std::unique_ptr<RocksDBDatabase>(new RocksDBDatabase(path, config));

    rocksdb::DBOptions dbOptions = config.createDBOptions();

    // 只读模式只能打开已有列族，不能创建新列族
    std::vector<std::string> existingCFNames;
    rocksdb::Status status = rocksdb::DB::ListColumnFamilies(dbOptions, path.string(), &existingCFNames);

    if (!status.ok()) {
        return Error(ErrorCode::FileNotFound, fmt::format("Database not found: {}", status.ToString()));
    }

    spdlog::info("Opening existing database read-only with {} column families", existingCFNames.size());

    rocksdb::ColumnFamilyOptions cfOptions = db->_createCFOptions();
    std::vector<rocksdb::ColumnFamilyDescriptor> cfDescriptors;

    for (const auto& cfName : existingCFNames) {
        cfDescriptors.emplace_back(cfName, cfOptions);
    }

    // 以只读模式打开
    std::vector<rocksdb::ColumnFamilyHandle*> cfHandles;
    status = rocksdb::DB::OpenForReadOnly(dbOptions, path.string(), cfDescriptors, &cfHandles, &db->m_db);

    if (!status.ok()) {
        return Error(
            ErrorCode::FileOpenFailed, fmt::format("Failed to open database read-only: {}", status.ToString()));
    }

    // 初始化列族句柄映射
    auto initResult = db->_initializeColumnFamilies(cfDescriptors, cfHandles);
    if (!initResult.success()) {
        for (auto* handle : cfHandles) {
            if (handle != nullptr && db->m_db != nullptr) {
                db->m_db->DestroyColumnFamilyHandle(handle);
            }
        }
        delete db->m_db;
        db->m_db = nullptr;
        return initResult.error();
    }

    spdlog::info("Database opened read-only with {} column families", db->m_cfHandles.size());

    return db;
}

// ============================================================================
// 基础操作
// ============================================================================

Result<std::vector<u8>> RocksDBDatabase::get(const std::string& cfName, const std::vector<u8>& key)
{
    rocksdb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    return get(cfName, keySlice);
}

Result<std::vector<u8>> RocksDBDatabase::get(const std::string& cfName, const rocksdb::Slice& key)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::get", "cf", cfName, "keySize", key.size());

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return Error(ErrorCode::NotFound, fmt::format("Column family not found: {}", cfName));
    }

    rocksdb::ReadOptions options = m_config.createReadOptions();
    std::string value;

    rocksdb::Status status = m_db->Get(options, cf, key, &value);

    if (status.IsNotFound()) {
        return Error(ErrorCode::NotFound, "Key not found");
    }

    if (!status.ok()) {
        return Error(ErrorCode::FileReadFailed, fmt::format("Failed to read key: {}", status.ToString()));
    }

    return std::vector<u8>(value.begin(), value.end());
}

Result<void> RocksDBDatabase::put(
    const std::string& cfName, const std::vector<u8>& key, const std::vector<u8>& value, bool sync)
{
    rocksdb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    rocksdb::Slice valueSlice(reinterpret_cast<const char*>(value.data()), value.size());
    return put(cfName, keySlice, valueSlice, sync);
}

Result<void> RocksDBDatabase::put(
    const std::string& cfName, const rocksdb::Slice& key, const rocksdb::Slice& value, bool sync)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db,
        "RocksDBDatabase::put",
        "cf",
        cfName,
        "keySize",
        key.size(),
        "valueSize",
        value.size(),
        "sync",
        sync);

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return Error(ErrorCode::NotFound, fmt::format("Column family not found: {}", cfName));
    }

    rocksdb::WriteOptions options = m_config.createWriteOptions();
    if (sync) {
        options.sync = true;
    }

    rocksdb::Status status = m_db->Put(options, cf, key, value);

    if (!status.ok()) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to write key: {}", status.ToString()));
    }

    return {};
}

Result<void> RocksDBDatabase::del(const std::string& cfName, const std::vector<u8>& key)
{
    rocksdb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    return del(cfName, keySlice);
}

Result<void> RocksDBDatabase::del(const std::string& cfName, const rocksdb::Slice& key)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::del", "cf", cfName, "keySize", key.size());

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return Error(ErrorCode::NotFound, fmt::format("Column family not found: {}", cfName));
    }

    rocksdb::WriteOptions options = m_config.createWriteOptions();
    rocksdb::Status status = m_db->Delete(options, cf, key);

    if (!status.ok()) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to delete key: {}", status.ToString()));
    }

    return {};
}

bool RocksDBDatabase::exists(const std::string& cfName, const std::vector<u8>& key)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::exists", "cf", cfName, "keySize", key.size());

    if (!isOpen()) {
        return false;
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return false;
    }

    rocksdb::ReadOptions options = m_config.createReadOptions();
    std::string value;

    rocksdb::Status status =
        m_db->Get(options, cf, rocksdb::Slice(reinterpret_cast<const char*>(key.data()), key.size()), &value);

    return status.ok();
}

// ============================================================================
// 批量操作
// ============================================================================

Result<void> RocksDBDatabase::writeBatch(rocksdb::WriteBatch& batch, bool sync)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::writeBatch", "count", batch.Count(), "sync", sync);

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    rocksdb::WriteOptions options = m_config.createWriteOptions();
    if (sync) {
        options.sync = true;
    }

    rocksdb::Status status = m_db->Write(options, &batch);

    if (!status.ok()) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to write batch: {}", status.ToString()));
    }

    return {};
}

Result<std::vector<Result<std::vector<u8>>>> RocksDBDatabase::multiGet(
    const std::string& cfName, const std::vector<std::vector<u8>>& keys)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::multiGet", "cf", cfName, "count", keys.size());

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return Error(ErrorCode::NotFound, fmt::format("Column family not found: {}", cfName));
    }

    rocksdb::ReadOptions options = m_config.createReadOptions();
    std::vector<rocksdb::Slice> keySlices;
    keySlices.reserve(keys.size());

    for (const auto& key : keys) {
        keySlices.emplace_back(reinterpret_cast<const char*>(key.data()), key.size());
    }

    std::vector<rocksdb::ColumnFamilyHandle*> handles(keys.size(), cf);
    std::vector<std::string> values(keys.size());
    std::vector<rocksdb::Status> statuses = m_db->MultiGet(options, handles, keySlices, &values);
    std::vector<Result<std::vector<u8>>> results;
    results.reserve(keys.size());

    for (size_t i = 0; i < statuses.size(); ++i) {
        const auto& status = statuses[i];
        if (status.ok()) {
            results.emplace_back(std::vector<u8>(values[i].begin(), values[i].end()));
            continue;
        }

        if (status.IsNotFound()) {
            results.emplace_back(Error(ErrorCode::NotFound, "Key not found"));
            continue;
        }

        results.emplace_back(
            Error(ErrorCode::FileReadFailed, fmt::format("Failed to read key: {}", status.ToString())));
    }

    return results;
}

// ============================================================================
// 范围操作
// ============================================================================

std::unique_ptr<rocksdb::Iterator> RocksDBDatabase::newIterator(const std::string& cfName)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::newIterator", "cf", cfName);

    if (!isOpen()) {
        return nullptr;
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return nullptr;
    }

    rocksdb::ReadOptions options = m_config.createReadOptions();
    return std::unique_ptr<rocksdb::Iterator>(m_db->NewIterator(options, cf));
}

Result<void> RocksDBDatabase::deleteRange(
    const std::string& cfName, const std::vector<u8>& startKey, const std::vector<u8>& endKey)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db,
        "RocksDBDatabase::deleteRange",
        "cf",
        cfName,
        "startKeySize",
        startKey.size(),
        "endKeySize",
        endKey.size());

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return Error(ErrorCode::NotFound, fmt::format("Column family not found: {}", cfName));
    }

    rocksdb::WriteOptions options = m_config.createWriteOptions();

    rocksdb::Slice start(reinterpret_cast<const char*>(startKey.data()), startKey.size());
    rocksdb::Slice end(reinterpret_cast<const char*>(endKey.data()), endKey.size());

    rocksdb::Status status = m_db->DeleteRange(options, cf, start, end);

    if (!status.ok()) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to delete range: {}", status.ToString()));
    }

    return {};
}

// ============================================================================
// 快照
// ============================================================================

const rocksdb::Snapshot* RocksDBDatabase::createSnapshot()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::createSnapshot");

    if (!isOpen()) {
        return nullptr;
    }

    return m_db->GetSnapshot();
}

void RocksDBDatabase::releaseSnapshot(const rocksdb::Snapshot* snapshot)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::releaseSnapshot");

    if (isOpen() && snapshot) {
        m_db->ReleaseSnapshot(snapshot);
    }
}

// ============================================================================
// 备份
// ============================================================================

Result<u64> RocksDBDatabase::createBackup(const std::filesystem::path& backupDir, const std::string& metadata)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Storage.Db, "RocksDBDatabase::createBackup", "backupDir", backupDir.string(), "metadata", metadata);

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    rocksdb::BackupEngine* backupEngine = nullptr;
    rocksdb::BackupEngineOptions backupOptions(backupDir.string());

    rocksdb::Status status = rocksdb::BackupEngine::Open(m_db->GetEnv(), backupOptions, &backupEngine);

    if (!status.ok()) {
        return Error(ErrorCode::FileOpenFailed, fmt::format("Failed to open backup engine: {}", status.ToString()));
    }

    // 创建备份
    status = backupEngine->CreateNewBackupWithMetadata(m_db, metadata);

    if (!status.ok()) {
        delete backupEngine;
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to create backup: {}", status.ToString()));
    }

    // 获取备份ID
    std::vector<rocksdb::BackupInfo> backupInfos;
    backupEngine->GetBackupInfo(&backupInfos);

    u64 backupId = backupInfos.empty() ? 0 : backupInfos.back().backup_id;

    delete backupEngine;

    spdlog::info("Created backup {} at {}", backupId, backupDir.string());

    return backupId;
}

Result<void> RocksDBDatabase::restoreFromBackup(
    const std::filesystem::path& backupDir, u64 backupId, const std::filesystem::path& targetDir)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db,
        "RocksDBDatabase::restoreFromBackup",
        "backupDir",
        backupDir.string(),
        "backupId",
        backupId,
        "targetDir",
        targetDir.string());

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    rocksdb::BackupEngine* backupEngine = nullptr;
    rocksdb::BackupEngineOptions backupOptions(backupDir.string());

    rocksdb::Status status = rocksdb::BackupEngine::Open(m_db->GetEnv(), backupOptions, &backupEngine);

    if (!status.ok()) {
        return Error(ErrorCode::FileOpenFailed, fmt::format("Failed to open backup engine: {}", status.ToString()));
    }

    // 恢复数据库
    status = backupEngine->RestoreDBFromBackup(static_cast<rocksdb::BackupID>(backupId),
        targetDir.string(), // db_dir
        targetDir.string()  // wal_dir
    );

    delete backupEngine;

    if (!status.ok()) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to restore backup: {}", status.ToString()));
    }

    spdlog::info("Restored backup {} to {}", backupId, targetDir.string());

    return {};
}

// ============================================================================
// 管理操作
// ============================================================================

Result<void> RocksDBDatabase::compact(const std::string& cfName)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::compact", "cf", cfName);

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return Error(ErrorCode::NotFound, fmt::format("Column family not found: {}", cfName));
    }

    rocksdb::CompactRangeOptions options;
    rocksdb::Status status = m_db->CompactRange(options, cf, nullptr, nullptr);

    if (!status.ok()) {
        return Error(ErrorCode::OperationFailed, fmt::format("Failed to compact: {}", status.ToString()));
    }

    return {};
}

Result<void> RocksDBDatabase::flush(const std::string& cfName, bool sync)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Storage.Db, "RocksDBDatabase::flush", "cf", cfName.empty() ? "(all)" : cfName, "sync", sync);

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Database is not open");
    }

    rocksdb::FlushOptions options;
    options.wait = sync;

    rocksdb::Status status;

    if (cfName.empty()) {
        // 刷新所有列族
        std::vector<rocksdb::ColumnFamilyHandle*> handles;
        for (auto& [name, handle] : m_cfHandles) {
            handles.push_back(handle);
        }
        status = m_db->Flush(options, handles);
    } else {
        auto* cf = getCF(cfName);
        if (!cf) {
            return Error(ErrorCode::NotFound, fmt::format("Column family not found: {}", cfName));
        }
        status = m_db->Flush(options, cf);
    }

    if (!status.ok()) {
        return Error(ErrorCode::OperationFailed, fmt::format("Failed to flush: {}", status.ToString()));
    }

    return {};
}

void RocksDBDatabase::close()
{
    if (!isOpen()) {
        return;
    }

    spdlog::info("Closing database at {}", m_path.string());

    // 刷新所有数据
    flush("", false);

    // 销毁所有列族句柄（必须在 delete m_db 之前调用）
    _destroyColumnFamilyHandles();

    // 关闭数据库
    delete m_db;
    m_db = nullptr;
}

std::string RocksDBDatabase::getStatistics() const
{
    if (!isOpen()) {
        return "";
    }

    auto stats = m_db->GetDBOptions().statistics;
    if (stats) {
        return stats->ToString();
    }

    return "";
}

std::string RocksDBDatabase::getProperty(const std::string& cfName, const std::string& property)
{
    if (!isOpen()) {
        return "";
    }

    auto* cf = getCF(cfName);
    if (!cf) {
        return "";
    }

    std::string value;
    m_db->GetProperty(cf, property, &value);
    return value;
}

// ============================================================================
// 列族管理
// ============================================================================

rocksdb::ColumnFamilyHandle* RocksDBDatabase::getCF(const std::string& cfName)
{
    auto it = m_cfHandles.find(cfName);
    if (it != m_cfHandles.end()) {
        return it->second;
    }
    return nullptr;
}

bool RocksDBDatabase::hasColumnFamily(const std::string& cfName) const
{
    return m_cfHandles.find(cfName) != m_cfHandles.end();
}

std::vector<std::string> RocksDBDatabase::listColumnFamilies() const
{
    std::vector<std::string> names;
    names.reserve(m_cfHandles.size());
    for (const auto& [name, handle] : m_cfHandles) {
        names.push_back(name);
    }
    return names;
}

// ============================================================================
// 私有方法
// ============================================================================

std::vector<rocksdb::ColumnFamilyDescriptor> RocksDBDatabase::_buildColumnFamilyDescriptors(bool dbExists)
{
    rocksdb::ColumnFamilyOptions cfOptions = _createCFOptions();
    std::vector<rocksdb::ColumnFamilyDescriptor> cfDescriptors;

    if (!dbExists) {
        // 新数据库：创建所有定义的列族
        for (const auto& cfName : cf::ALL_COLUMN_FAMILIES) {
            cfDescriptors.emplace_back(cfName, cfOptions);
        }
    } else {
        // 已有数据库：打开已有列族，补充缺失的列族
        std::vector<std::string> existingCFNames;
        rocksdb::DBOptions dbOptions = m_config.createDBOptions();
        rocksdb::Status status = rocksdb::DB::ListColumnFamilies(dbOptions, m_path.string(), &existingCFNames);

        if (!status.ok()) {
            spdlog::warn("Failed to list column families, falling back to default: {}", status.ToString());
            existingCFNames = {cf::META};
        }

        spdlog::info("Found {} existing column families", existingCFNames.size());

        // 打开已有列族
        for (const auto& cfName : existingCFNames) {
            cfDescriptors.emplace_back(cfName, cfOptions);
        }

        // 补充缺失的列族
        for (const auto& cfName : cf::ALL_COLUMN_FAMILIES) {
            if (std::find(existingCFNames.begin(), existingCFNames.end(), cfName) == existingCFNames.end()) {
                spdlog::info("Adding new column family: {}", cfName);
                cfDescriptors.emplace_back(cfName, cfOptions);
            }
        }
    }

    return cfDescriptors;
}

Result<void> RocksDBDatabase::_initializeColumnFamilies(
    const std::vector<rocksdb::ColumnFamilyDescriptor>& cfDescriptors,
    const std::vector<rocksdb::ColumnFamilyHandle*>& cfHandles)
{
    MC_ASSERT_RELEASE(cfDescriptors.size() == cfHandles.size());

    for (size_t i = 0; i < cfDescriptors.size(); ++i) {
        if (cfHandles[i] == nullptr) {
            spdlog::warn("Column family handle is null for: {}", cfDescriptors[i].name);
            continue;
        }
        m_cfHandles[cfDescriptors[i].name] = cfHandles[i];
    }

    return {};
}

void RocksDBDatabase::_destroyColumnFamilyHandles()
{
    if (m_db != nullptr) {
        for (auto& [name, handle] : m_cfHandles) {
            if (handle != nullptr) {
                m_db->DestroyColumnFamilyHandle(handle);
            }
        }
    }
    m_cfHandles.clear();
}

rocksdb::ColumnFamilyOptions RocksDBDatabase::_createCFOptions() const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "RocksDBDatabase::createCFOptions");

    return m_config.createColumnFamilyOptions();
}

} // namespace mc::world::storage
