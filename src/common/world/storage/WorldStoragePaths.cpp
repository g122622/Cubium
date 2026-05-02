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

} // namespace mc::world::storage
