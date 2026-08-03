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

#include "common/world/storage/list/WorldListService.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include "common/world/storage/core/WorldSessionLock.hpp"
#include "common/world/storage/core/WorldStoragePaths.hpp"
#include "common/world/storage/list/WorldListEntry.hpp"
#include "common/world/storage/list/WorldNameSanitizer.hpp"
#include "common/world/storage/reader/bedrock/BedrockLevelDatReader.hpp"
#include "common/world/storage/reader/java/JavaLevelDatReader.hpp"
#include "common/world/storage/request/WorldRequests.hpp"
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <archive.h>
#include <archive_entry.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

WorldListService::WorldListService(WorldStoragePaths paths)
    : m_paths(std::move(paths))
{}

Result<std::vector<WorldListEntry>> WorldListService::listWorlds()
{
    auto dirsResult = _enumerateWorldDirectories();
    if (!dirsResult.success()) {
        return dirsResult.error();
    }

    std::vector<WorldListEntry> entries;
    entries.reserve(dirsResult.value().size());

    for (const auto& levelId : dirsResult.value()) {
        std::filesystem::path worldDir = m_paths.worldDir(levelId);
        entries.push_back(_tryReadWorldSummary(levelId, worldDir));
    }

    // 按 lastPlayedMs 降序、levelId 升序排序
    sortWorldEntries(entries);

    return entries;
}

Result<WorldListEntry> WorldListService::getWorldSummary(const std::string& levelId)
{
    std::filesystem::path worldDir = m_paths.worldDir(levelId);

    std::error_code ec;
    if (!std::filesystem::exists(worldDir, ec)) {
        return Error(ErrorCode::FileNotFound, "World not found: " + levelId);
    }

    return _tryReadWorldSummary(levelId, worldDir);
}

bool WorldListService::worldExists(const std::string& levelId)
{
    std::error_code ec;
    return std::filesystem::exists(m_paths.worldDir(levelId), ec);
}

Result<std::string> WorldListService::createWorld(const CreateWorldRequest& request)
{
    // 确保 saves 目录存在
    if (!m_paths.ensureSavesDirExists()) {
        return Error(ErrorCode::FileWriteFailed, "Failed to create saves directory");
    }

    // 确定可用的 levelId
    std::string levelId;
    if (!request.requestedLevelId.empty()) {
        levelId = request.requestedLevelId;
        // 检查是否已被占用
        if (worldExists(levelId)) {
            return Error(ErrorCode::AlreadyExists, "World already exists: " + levelId);
        }
    } else {
        // 自动生成可用目录名
        auto idResult = WorldNameSanitizer::findAvailableLevelId(m_paths.savesDir(), request.displayName);
        if (!idResult.success()) {
            return idResult.error();
        }
        levelId = idResult.value();
    }

    // 创建世界目录
    std::filesystem::path worldDir = m_paths.worldDir(levelId);
    std::error_code ec;
    if (!std::filesystem::create_directory(worldDir, ec)) {
        return Error(ErrorCode::FileWriteFailed, "Failed to create world directory: " + worldDir.string());
    }

    // 获取会话锁
    auto lockResult = WorldSessionLock::acquire(worldDir);
    if (!lockResult.success()) {
        // 清理已创建的目录
        std::filesystem::remove(worldDir, ec);
        return lockResult.error();
    }
    auto lock = std::move(lockResult.value());

    // 写入初始 level.dat
    auto writeResult = LevelDatCodec::writeInitial(worldDir, request);
    if (!writeResult.success()) {
        lock.release();
        std::filesystem::remove_all(worldDir, ec);
        return writeResult.error();
    }

    // 释放锁
    lock.release();

    spdlog::info("Created world '{}' at {}", request.displayName, worldDir.string());
    return levelId;
}

Result<void> WorldListService::deleteWorld(const std::string& levelId)
{
    std::filesystem::path worldDir = m_paths.worldDir(levelId);

    std::error_code ec;
    if (!std::filesystem::exists(worldDir, ec)) {
        return Error(ErrorCode::FileNotFound, "World not found: " + levelId);
    }

    // 检查锁定状态
    if (WorldSessionLock::isLocked(worldDir)) {
        return Error(ErrorCode::PermissionDenied, "Cannot delete locked world: " + levelId);
    }

    // 递归删除目录
    if (!std::filesystem::remove_all(worldDir, ec)) {
        return Error(ErrorCode::FileWriteFailed, "Failed to delete world directory: " + ec.message());
    }

    spdlog::info("Deleted world: {}", levelId);
    return {};
}

Result<void> WorldListService::renameWorld(const std::string& levelId, const std::string& newDisplayName)
{
    std::filesystem::path worldDir = m_paths.worldDir(levelId);

    std::error_code ec;
    if (!std::filesystem::exists(worldDir, ec)) {
        return Error(ErrorCode::FileNotFound, "World not found: " + levelId);
    }

    // 获取锁
    auto lockResult = WorldSessionLock::acquire(worldDir);
    if (!lockResult.success()) {
        return lockResult.error();
    }
    auto lock = std::move(lockResult.value());

    // 更新 level.dat 中的 LevelName
    auto result = LevelDatCodec::updateDisplayName(worldDir, newDisplayName);

    lock.release();

    if (!result.success()) {
        return result.error();
    }

    spdlog::info("Renamed world {} to '{}'", levelId, newDisplayName);
    return {};
}

Result<void> WorldListService::updateLastPlayed(const std::string& levelId, i64 lastPlayedMs)
{
    std::filesystem::path worldDir = m_paths.worldDir(levelId);

    std::error_code ec;
    if (!std::filesystem::exists(worldDir, ec)) {
        return Error(ErrorCode::FileNotFound, "World not found: " + levelId);
    }

    // 获取锁
    auto lockResult = WorldSessionLock::acquire(worldDir);
    if (!lockResult.success()) {
        return lockResult.error();
    }
    auto lock = std::move(lockResult.value());

    auto result = LevelDatCodec::updateLastPlayed(worldDir, lastPlayedMs);

    lock.release();

    return result;
}

Result<BackupWorldResult> WorldListService::backupWorld(const BackupWorldRequest& request)
{
    std::filesystem::path worldDir = m_paths.worldDir(request.levelId);

    std::error_code ec;
    if (!std::filesystem::exists(worldDir, ec)) {
        return Error(ErrorCode::FileNotFound, "World not found: " + request.levelId);
    }

    // 确保备份目录存在
    if (!m_paths.ensureBackupsDirExists()) {
        return Error(ErrorCode::FileWriteFailed, "Failed to create backups directory");
    }

    // 生成备份文件名
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::string backupName = request.levelId + "_" + std::to_string(nowMs) + ".zip";
    std::filesystem::path backupPath = m_paths.backupsDir() / backupName;

    // 使用 LibArchive 创建 zip 文件
    struct archive* a = archive_write_new();
    if (!a) {
        return Error(ErrorCode::FileWriteFailed, "Failed to create archive writer");
    }

    archive_write_set_format_zip(a);
    archive_write_zip_set_compression_deflate(a);

    int r = archive_write_open_filename(a, backupPath.string().c_str());
    if (r != ARCHIVE_OK) {
        std::string err = archive_error_string(a) ? archive_error_string(a) : "Unknown error";
        archive_write_free(a);
        return Error(ErrorCode::FileWriteFailed, "Failed to open backup file: " + err);
    }

    bool success = true;
    std::string errorMsg;

    // 递归添加目录内容
    for (const auto& entry : std::filesystem::recursive_directory_iterator(worldDir, ec)) {
        if (ec) {
            success = false;
            errorMsg = "Failed to iterate world directory: " + ec.message();
            break;
        }

        if (!entry.is_regular_file()) {
            continue;
        }

        // 跳过 session.lock
        if (entry.path().filename() == "session.lock") {
            continue;
        }

        // 计算相对路径
        std::filesystem::path relative = std::filesystem::relative(entry.path(), worldDir, ec);
        if (ec) {
            success = false;
            errorMsg = "Failed to compute relative path: " + ec.message();
            break;
        }

        std::string relativeStr = relative.string();

        // 读取文件内容
        std::ifstream file(entry.path(), std::ios::binary);
        if (!file) {
            success = false;
            errorMsg = "Failed to read file: " + entry.path().string();
            break;
        }

        std::vector<char> content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // 创建 archive entry
        struct archive_entry* entry_out = archive_entry_new();
        archive_entry_set_pathname(entry_out, relativeStr.c_str());
        archive_entry_set_size(entry_out, content.size());
        archive_entry_set_filetype(entry_out, AE_IFREG);
        archive_entry_set_perm(entry_out, 0644);

        r = archive_write_header(a, entry_out);
        if (r != ARCHIVE_OK) {
            archive_entry_free(entry_out);
            success = false;
            errorMsg = "Failed to write header for: " + relativeStr;
            break;
        }

        la_ssize_t written = archive_write_data(a, content.data(), content.size());
        if (written < 0 || static_cast<size_t>(written) != content.size()) {
            archive_entry_free(entry_out);
            success = false;
            errorMsg = "Failed to write data for: " + relativeStr;
            break;
        }

        archive_entry_free(entry_out);
    }

    archive_write_close(a);
    archive_write_free(a);

    if (!success) {
        std::filesystem::remove(backupPath, ec);
        return Error(ErrorCode::FileWriteFailed, errorMsg);
    }

    // 获取备份文件大小
    auto backupSize = std::filesystem::file_size(backupPath, ec);
    if (ec) {
        backupSize = 0;
    }

    spdlog::info("Created backup of world {} at {} ({} bytes)", request.levelId, backupPath.string(), backupSize);

    return BackupWorldResult(backupPath, backupSize);
}

const WorldStoragePaths& WorldListService::paths() const noexcept
{
    return m_paths;
}

Result<std::vector<std::string>> WorldListService::_enumerateWorldDirectories()
{
    std::error_code ec;

    if (!m_paths.ensureSavesDirExists()) {
        return Error(ErrorCode::FileWriteFailed, "Failed to create saves directory");
    }

    std::vector<std::string> dirs;

    for (const auto& entry : std::filesystem::directory_iterator(m_paths.savesDir(), ec)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::string dirName = entry.path().filename().string();

        // 跳过隐藏目录
        if (!dirName.empty() && dirName[0] == '.') {
            continue;
        }

        dirs.push_back(dirName);
    }

    if (ec) {
        return Error(ErrorCode::FileReadFailed, "Failed to enumerate saves directory: " + ec.message());
    }

    return dirs;
}

WorldListEntry WorldListService::_tryReadWorldSummary(const std::string& levelId, const std::filesystem::path& worldDir)
{
    WorldListEntry entry;
    entry.levelId = levelId;
    entry.worldDir = worldDir;

    // 检测锁定状态
    entry.locked = _detectLock(worldDir);

    // 检测图标
    entry.iconPath = _detectIconPath(worldDir);

    Result<LevelSummaryData> summaryResult = LevelDatCodec::readSummary(worldDir);
    auto formatResult = SaveFormatDetector::detect(worldDir);
    if (formatResult.success()) {
        switch (formatResult.value().format) {
            case SaveFormat::JavaAnvil:
                summaryResult = reader::java::JavaLevelDatReader::readSummary(worldDir);
                break;
            case SaveFormat::BedrockLDB:
                summaryResult = reader::bedrock::BedrockLevelDatReader::readSummary(worldDir);
                break;
            case SaveFormat::Native:
                break;
        }
    }

    if (summaryResult.success()) {
        const auto& summary = summaryResult.value();
        entry.displayName = summary.displayName;
        entry.lastPlayedMs = summary.lastPlayedMs;
        entry.seed = summary.seed;
        entry.worldType = summary.worldType;
        entry.worldPresetId = summary.worldPresetId;
        entry.gameMode = summary.gameMode;
        entry.difficulty = summary.difficulty;
        entry.hardcore = summary.hardcore;
        entry.allowCommands = summary.allowCommands;
        entry.versionName = summary.version.versionName;
        entry.dataVersion = summary.dataVersion;
        entry.compatibility = summary.compatibility;
        entry.errorMessage = summary.errorMessage;
        return entry;
    }

    entry.compatibility = WorldCompatibility::Corrupted;
    entry.errorMessage = summaryResult.failed() ? summaryResult.error().message() : "Failed to parse level.dat";
    return entry;
}

bool WorldListService::_detectLock(const std::filesystem::path& worldDir)
{
    return WorldSessionLock::isLocked(worldDir);
}

std::filesystem::path WorldListService::_detectIconPath(const std::filesystem::path& worldDir)
{
    std::error_code ec;
    std::filesystem::path iconPath = worldDir / "icon.png";

    if (std::filesystem::exists(iconPath, ec) && !ec) {
        return iconPath;
    }

    return {};
}

} // namespace mc::world::storage
