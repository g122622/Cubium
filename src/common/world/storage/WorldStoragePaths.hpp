#pragma once

#include <filesystem>
#include <string>

namespace mc::world::storage {

/**
 * @brief 存档目录路径配置
 *
 * 提供 saves 和 backups 目录的路径解析。第一版使用工作目录下的 saves/，
 * 后续可迁移到平台用户数据目录。
 */
class WorldStoragePaths {
public:
    /**
     * @brief 构造存档路径配置
     * @param savesDir 存档根目录路径，如果为空则使用工作目录下的 "saves"
     * @param backupsDir 备份目录路径，如果为空则使用 savesDir 同级的 "backups"
     */
    explicit WorldStoragePaths(
        std::filesystem::path savesDir,
        std::filesystem::path backupsDir
    );

    /**
     * @brief 使用默认路径构造
     *
     * saves 目录为当前工作目录下的 "saves"，
     * backups 目录为 saves 同级的 "backups"。
     */
    static WorldStoragePaths defaultPaths();

    /**
     * @brief 获取存档根目录
     */
    [[nodiscard]] const std::filesystem::path& savesDir() const noexcept;

    /**
     * @brief 获取备份目录
     */
    [[nodiscard]] const std::filesystem::path& backupsDir() const noexcept;

    /**
     * @brief 获取指定世界目录路径
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path worldDir(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的 level.dat 路径
     */
    [[nodiscard]] std::filesystem::path levelDatPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的 level.dat_old 备份路径
     */
    [[nodiscard]] std::filesystem::path levelDatOldPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的 session.lock 路径
     */
    [[nodiscard]] std::filesystem::path sessionLockPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的图标路径
     */
    [[nodiscard]] std::filesystem::path iconPath(const std::string& levelId) const;

    /**
     * @brief 确保存档根目录存在
     * @return 成功返回 true，失败返回 false
     */
    bool ensureSavesDirExists() const;

    /**
     * @brief 确保备份目录存在
     * @return 成功返回 true，失败返回 false
     */
    bool ensureBackupsDirExists() const;

private:
    std::filesystem::path m_savesDir;
    std::filesystem::path m_backupsDir;
};

} // namespace mc::world::storage
