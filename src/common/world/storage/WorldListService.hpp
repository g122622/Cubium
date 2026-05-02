#pragma once

#include "LevelDatCodec.hpp"
#include "WorldListEntry.hpp"
#include "WorldRequests.hpp"
#include "WorldSessionLock.hpp"
#include "WorldStoragePaths.hpp"
#include "../../core/Result.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc::world::storage {

/**
 * @brief 世界列表服务
 *
 * 枚举 saves 目录下的所有世界，解析摘要，提供搜索和排序。
 */
class WorldListService {
public:
    /**
     * @brief 构造世界列表服务
     *
     * @param paths 存档路径配置
     */
    explicit WorldListService(WorldStoragePaths paths);

    /**
     * @brief 列出所有世界
     *
     * 枚举 savesDir 下的子目录，尝试读取 level.dat/level.dat_old，
     * 解析摘要，检测锁定状态，按 lastPlayedMs 排序。
     *
     * @return 世界列表，失败返回错误
     */
    Result<std::vector<WorldListEntry>> listWorlds();

    /**
     * @brief 获取单个世界的摘要
     *
     * @param levelId 世界目录名
     * @return 成功返回摘要，失败返回错误
     */
    Result<WorldListEntry> getWorldSummary(const std::string& levelId);

    /**
     * @brief 检查世界是否存在
     *
     * @param levelId 世界目录名
     * @return 存在返回 true
     */
    bool worldExists(const std::string& levelId);

    /**
     * @brief 创建新世界目录并写入初始 level.dat
     *
     * @param request 创建世界请求
     * @return 成功返回实际使用的 levelId，失败返回错误
     */
    Result<std::string> createWorld(const CreateWorldRequest& request);

    /**
     * @brief 删除世界目录
     *
     * 必须先获取锁。递归删除整个目录。
     *
     * @param levelId 世界目录名
     * @return 成功返回空，失败返回错误
     */
    Result<void> deleteWorld(const std::string& levelId);

    /**
     * @brief 重命名世界显示名
     *
     * 只修改 level.dat 中的 LevelName，不改变目录名。
     *
     * @param levelId 世界目录名
     * @param newDisplayName 新的显示名称
     * @return 成功返回空，失败返回错误
     */
    Result<void> renameWorld(const std::string& levelId, const std::string& newDisplayName);

    /**
     * @brief 更新世界的最后游玩时间
     *
     * @param levelId 世界目录名
     * @param lastPlayedMs 时间戳（默认为当前时间）
     * @return 成功返回空，失败返回错误
     */
    Result<void> updateLastPlayed(const std::string& levelId, i64 lastPlayedMs);

    /**
     * @brief 创建世界备份
     *
     * 将世界目录打包为 zip，存放在 backups 目录。
     *
     * @param request 备份请求
     * @return 成功返回备份结果，失败返回错误
     */
    Result<BackupWorldResult> backupWorld(const BackupWorldRequest& request);

    /**
     * @brief 获取存档路径配置
     */
    [[nodiscard]] const WorldStoragePaths& paths() const noexcept;

private:
    WorldStoragePaths m_paths;

    /**
     * @brief 枚举所有世界目录名
     */
    Result<std::vector<std::string>> enumerateWorldDirectories();

    /**
     * @brief 尝试从目录读取世界摘要
     *
     * 失败时返回损坏状态而非错误。
     */
    WorldListEntry tryReadWorldSummary(
        const std::string& levelId,
        const std::filesystem::path& worldDir
    );

    /**
     * @brief 检测目录是否被锁定
     */
    bool detectLock(const std::filesystem::path& worldDir);

    /**
     * @brief 检查图标是否存在
     */
    std::filesystem::path detectIconPath(const std::filesystem::path& worldDir);
};

} // namespace mc::world::storage
