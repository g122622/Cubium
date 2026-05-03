#include "WorldStoragePaths.hpp"
#include <filesystem>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

WorldStoragePaths::WorldStoragePaths(
    std::filesystem::path savesDir,
    std::filesystem::path backupsDir
)
    : m_savesDir(std::move(savesDir))
    , m_backupsDir(std::move(backupsDir))
{
}

WorldStoragePaths WorldStoragePaths::defaultPaths()
{
    return WorldStoragePaths(
        std::filesystem::current_path() / "saves",
        std::filesystem::current_path() / "backups"
    );
}

// ============================================================================
// 基础路径
// ============================================================================

const std::filesystem::path& WorldStoragePaths::savesDir() const noexcept
{
    return m_savesDir;
}

const std::filesystem::path& WorldStoragePaths::backupsDir() const noexcept
{
    return m_backupsDir;
}

std::filesystem::path WorldStoragePaths::worldDir(const std::string& levelId) const
{
    return m_savesDir / levelId;
}

// ============================================================================
// 传统文件路径
// ============================================================================

std::filesystem::path WorldStoragePaths::levelDatPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "level.dat";
}

std::filesystem::path WorldStoragePaths::levelDatOldPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "level.dat_old";
}

std::filesystem::path WorldStoragePaths::sessionLockPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "session.lock";
}

std::filesystem::path WorldStoragePaths::iconPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "icon.png";
}

// ============================================================================
// RocksDB 数据库路径（自有格式）
// ============================================================================

std::filesystem::path WorldStoragePaths::dbPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "db";
}

std::filesystem::path WorldStoragePaths::dbChunksPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "db" / "chunks";
}

std::filesystem::path WorldStoragePaths::dbEntitiesPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "db" / "entities";
}

std::filesystem::path WorldStoragePaths::dbPoiPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "db" / "poi";
}

std::filesystem::path WorldStoragePaths::dbSnapshotsMetaPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "db" / "snapshots";
}

// ============================================================================
// 快照路径（版本控制）
// ============================================================================

std::filesystem::path WorldStoragePaths::snapshotsPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "snapshots";
}

std::filesystem::path WorldStoragePaths::snapshotPath(
    const std::string& levelId,
    const std::string& snapshotId
) const
{
    return m_savesDir / levelId / "snapshots" / snapshotId;
}

std::filesystem::path WorldStoragePaths::snapshotManifestPath(
    const std::string& levelId,
    const std::string& snapshotId
) const
{
    return m_savesDir / levelId / "snapshots" / snapshotId / "manifest.json";
}

std::filesystem::path WorldStoragePaths::snapshotDeltaPath(
    const std::string& levelId,
    const std::string& snapshotId
) const
{
    return m_savesDir / levelId / "snapshots" / snapshotId / "delta";
}

// ============================================================================
// 导入路径（格式转换）
// ============================================================================

std::filesystem::path WorldStoragePaths::importPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "import";
}

std::filesystem::path WorldStoragePaths::importJavaPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "import" / "java";
}

std::filesystem::path WorldStoragePaths::importBedrockPath(const std::string& levelId) const
{
    return m_savesDir / levelId / "import" / "bedrock";
}

// ============================================================================
// 目录创建
// ============================================================================

bool WorldStoragePaths::ensureSavesDirExists() const
{
    std::error_code ec;
    if (std::filesystem::exists(m_savesDir, ec)) {
        return true;
    }
    return std::filesystem::create_directories(m_savesDir, ec);
}

bool WorldStoragePaths::ensureBackupsDirExists() const
{
    std::error_code ec;
    if (std::filesystem::exists(m_backupsDir, ec)) {
        return true;
    }
    return std::filesystem::create_directories(m_backupsDir, ec);
}

bool WorldStoragePaths::ensureWorldDirExists(const std::string& levelId) const
{
    std::error_code ec;
    const auto worldDir = this->worldDir(levelId);
    if (std::filesystem::exists(worldDir, ec)) {
        return true;
    }
    return std::filesystem::create_directories(worldDir, ec);
}

bool WorldStoragePaths::ensureDbDirExists(const std::string& levelId) const
{
    std::error_code ec;
    const auto dbDir = dbPath(levelId);
    if (std::filesystem::exists(dbDir, ec)) {
        return true;
    }

    // 创建数据库目录及其子目录
    if (!std::filesystem::create_directories(dbDir, ec)) {
        return false;
    }
    if (!std::filesystem::create_directories(dbChunksPath(levelId), ec)) {
        return false;
    }
    if (!std::filesystem::create_directories(dbEntitiesPath(levelId), ec)) {
        return false;
    }
    if (!std::filesystem::create_directories(dbPoiPath(levelId), ec)) {
        return false;
    }
    if (!std::filesystem::create_directories(dbSnapshotsMetaPath(levelId), ec)) {
        return false;
    }
    return true;
}

} // namespace mc::world::storage
