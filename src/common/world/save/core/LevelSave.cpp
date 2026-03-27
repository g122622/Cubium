#include "LevelSave.hpp"
#include "SessionLock.hpp"
#include "io/FileUtil.hpp"
#include <filesystem>
#include <chrono>

namespace mc::world::save {

// ========== 构造函数 ==========

LevelSave::LevelSave(std::filesystem::path worldDir, String worldName)
    : m_worldDir(std::move(worldDir))
    , m_worldName(std::move(worldName))
{
}

// ========== 静态工厂方法 ==========

Result<std::unique_ptr<LevelSave>>
LevelSave::create(const std::filesystem::path& savesDir, const String& worldName) {
    // 验证世界名称
    if (worldName.empty()) {
        return Error(ErrorCode::InvalidArgument, "World name cannot be empty");
    }

    // 构建世界目录路径
    std::filesystem::path worldDir = savesDir / worldName;

    // 检查目录是否已存在
    if (std::filesystem::exists(worldDir)) {
        return Error(ErrorCode::AlreadyExists,
                     "World directory already exists: " + worldDir.string());
    }

    // 创建目录结构
    try {
        std::filesystem::create_directories(worldDir);
        std::filesystem::create_directories(worldDir / "region");
        std::filesystem::create_directories(worldDir / "playerdata");
        std::filesystem::create_directories(worldDir / "stats");
        std::filesystem::create_directories(worldDir / "advancements");
        std::filesystem::create_directories(worldDir / "data");
        std::filesystem::create_directories(worldDir / "DIM-1" / "region");
        std::filesystem::create_directories(worldDir / "DIM1" / "region");
    } catch (const std::filesystem::filesystem_error& e) {
        return Error(ErrorCode::FileWriteFailed,
                     "Failed to create world directories: " + String(e.what()));
    }

    // 创建会话锁
    auto lockResult = SessionLock::create(worldDir);
    if (lockResult.failed()) {
        // 清理已创建的目录
        try {
            std::filesystem::remove_all(worldDir);
        } catch (...) {
            // 忽略清理错误
        }
        return lockResult.error();
    }

    auto levelSave = std::unique_ptr<LevelSave>(
        new LevelSave(std::move(worldDir), worldName)
    );

    return levelSave;
}

Result<std::unique_ptr<LevelSave>>
LevelSave::open(const std::filesystem::path& worldDir) {
    // 检查目录是否存在
    if (!std::filesystem::exists(worldDir)) {
        return Error(ErrorCode::FileNotFound,
                     "World directory does not exist: " + worldDir.string());
    }

    if (!std::filesystem::is_directory(worldDir)) {
        return Error(ErrorCode::InvalidArgument,
                     "Path is not a directory: " + worldDir.string());
    }

    // 检查 level.dat 是否存在
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    if (!std::filesystem::exists(levelDatPath)) {
        return Error(ErrorCode::FileNotFound,
                     "level.dat not found in world directory");
    }

    // 获取世界名称
    String worldName = worldDir.filename().string();

    // 创建会话锁
    auto lockResult = SessionLock::create(worldDir);
    if (lockResult.failed()) {
        return lockResult.error();
    }

    auto levelSave = std::unique_ptr<LevelSave>(
        new LevelSave(worldDir, std::move(worldName))
    );

    return levelSave;
}

// ========== 备份与删除 ==========

Result<std::filesystem::path> LevelSave::createBackup() {
    // 生成备份文件名
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now_time);
#else
    localtime_r(&now_time, &tm_buf);
#endif

    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm_buf);

    std::filesystem::path backupPath = m_worldDir.parent_path() /
        (m_worldName + "_backup_" + timestamp);

    // 检查备份路径是否已存在
    if (std::filesystem::exists(backupPath)) {
        return Error(ErrorCode::AlreadyExists,
                     "Backup already exists: " + backupPath.string());
    }

    // 复制目录
    try {
        std::filesystem::copy(m_worldDir, backupPath,
            std::filesystem::copy_options::recursive |
            std::filesystem::copy_options::copy_symlinks);
    } catch (const std::filesystem::filesystem_error& e) {
        return Error(ErrorCode::FileWriteFailed,
                     "Failed to create backup: " + String(e.what()));
    }

    return backupPath;
}

Result<void> LevelSave::deleteWorld() {
    // 删除整个世界目录
    try {
        std::filesystem::remove_all(m_worldDir);
    } catch (const std::filesystem::filesystem_error& e) {
        return Error(ErrorCode::FileWriteFailed,
                     "Failed to delete world: " + String(e.what()));
    }

    return {};
}

// ========== 目录创建 ==========

Result<void> LevelSave::ensureDirectoriesExist() {
    try {
        // 主世界目录
        if (!std::filesystem::exists(m_worldDir / "region")) {
            std::filesystem::create_directories(m_worldDir / "region");
        }

        // 玩家数据目录
        if (!std::filesystem::exists(playerDataDir())) {
            std::filesystem::create_directories(playerDataDir());
        }

        // 统计目录
        if (!std::filesystem::exists(statsDir())) {
            std::filesystem::create_directories(statsDir());
        }

        // 成就目录
        if (!std::filesystem::exists(advancementsDir())) {
            std::filesystem::create_directories(advancementsDir());
        }

        // 主世界数据目录
        if (!std::filesystem::exists(dataDir(0))) {
            std::filesystem::create_directories(dataDir(0));
        }

        // 下界目录
        if (!std::filesystem::exists(regionDir(-1))) {
            std::filesystem::create_directories(regionDir(-1));
        }
        if (!std::filesystem::exists(dataDir(-1))) {
            std::filesystem::create_directories(dataDir(-1));
        }

        // 末地目录
        if (!std::filesystem::exists(regionDir(1))) {
            std::filesystem::create_directories(regionDir(1));
        }
        if (!std::filesystem::exists(dataDir(1))) {
            std::filesystem::create_directories(dataDir(1));
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return Error(ErrorCode::FileWriteFailed,
                     "Failed to create directories: " + String(e.what()));
    }

    return {};
}

} // namespace mc::world::save
