#pragma once

#include "../core/SessionLock.hpp"
#include "../io/FileUtil.hpp"
#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include <filesystem>
#include <memory>
#include <string>

namespace mc::world::save {

/**
 * @brief 单个世界的存档操作
 *
 * 管理世界目录结构，提供路径访问。
 * 参考 MC 1.16.5 SaveFormat.LevelSave
 *
 * ## 目录结构
 * ```
 * <world_name>/
 * ├── level.dat          # 世界元数据
 * ├── level.dat_old      # 备份
 * ├── session.lock       # 会话锁
 * ├── icon.png           # 世界图标（可选）
 * ├── playerdata/        # 玩家数据
 * │   └── <uuid>.dat
 * ├── stats/             # 统计
 * │   └── <uuid>.json
 * ├── advancements/      # 成就
 * │   └── <uuid>.json
 * ├── data/              # 维度数据
 * ├── region/            # 主世界区块
 * │   └── r.<x>.<z>.mca
 * ├── DIM-1/             # 下界
 * │   └── region/
 * └── DIM1/              # 末地
 *     └── region/
 * ```
 *
 * ## 使用示例
 * ```cpp
 * // 创建新世界
 * auto createResult = LevelSave::create("saves/", "MyWorld");
 * if (createResult.success()) {
 *     auto& levelSave = createResult.value();
 *     // 使用 levelSave...
 * }
 *
 * // 打开现有世界
 * auto openResult = LevelSave::open("saves/MyWorld");
 * if (openResult.success()) {
 *     auto& levelSave = openResult.value();
 * }
 * ```
 */
class LevelSave {
public:
    /**
     * @brief 创建新世界存档目录
     *
     * @param savesDir 存档根目录
     * @param worldName 世界名称
     * @return 成功返回 LevelSave，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<LevelSave>>
    create(const std::filesystem::path& savesDir, const String& worldName);

    /**
     * @brief 打开现有世界存档
     *
     * @param worldDir 世界目录路径
     * @return 成功返回 LevelSave，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<LevelSave>>
    open(const std::filesystem::path& worldDir);

    ~LevelSave() = default;

    // 禁止拷贝
    LevelSave(const LevelSave&) = delete;
    LevelSave& operator=(const LevelSave&) = delete;

    // 允许移动
    LevelSave(LevelSave&& other) noexcept = default;
    LevelSave& operator=(LevelSave&& other) noexcept = default;

    // ========== 路径访问 ==========

    /**
     * @brief 获取世界目录路径
     */
    [[nodiscard]] std::filesystem::path worldDir() const { return m_worldDir; }

    /**
     * @brief 获取世界名称
     */
    [[nodiscard]] const String& worldName() const { return m_worldName; }

    /**
     * @brief 获取 level.dat 路径
     */
    [[nodiscard]] std::filesystem::path levelDatPath() const {
        return m_worldDir / "level.dat";
    }

    /**
     * @brief 获取 level.dat_old 路径
     */
    [[nodiscard]] std::filesystem::path levelDatOldPath() const {
        return m_worldDir / "level.dat_old";
    }

    /**
     * @brief 获取会话锁路径
     */
    [[nodiscard]] std::filesystem::path sessionLockPath() const {
        return m_worldDir / "session.lock";
    }

    /**
     * @brief 获取世界图标路径
     */
    [[nodiscard]] std::filesystem::path iconPath() const {
        return m_worldDir / "icon.png";
    }

    // ========== 玩家数据 ==========

    /**
     * @brief 获取玩家数据目录
     */
    [[nodiscard]] std::filesystem::path playerDataDir() const {
        return m_worldDir / "playerdata";
    }

    /**
     * @brief 获取玩家数据文件路径
     *
     * @param playerId 玩家 UUID
     * @return 玩家数据文件路径
     */
    [[nodiscard]] std::filesystem::path playerDataPath(const String& playerId) const {
        return playerDataDir() / (playerId + ".dat");
    }

    /**
     * @brief 获取统计数据目录
     */
    [[nodiscard]] std::filesystem::path statsDir() const {
        return m_worldDir / "stats";
    }

    /**
     * @brief 获取统计数据文件路径
     */
    [[nodiscard]] std::filesystem::path statsPath(const String& playerId) const {
        return statsDir() / (playerId + ".json");
    }

    /**
     * @brief 获取成就数据目录
     */
    [[nodiscard]] std::filesystem::path advancementsDir() const {
        return m_worldDir / "advancements";
    }

    /**
     * @brief 获取成就数据文件路径
     */
    [[nodiscard]] std::filesystem::path advancementsPath(const String& playerId) const {
        return advancementsDir() / (playerId + ".json");
    }

    // ========== 维度数据 ==========

    /**
     * @brief 获取维度 Region 目录
     *
     * @param dimension 维度 ID（0=主世界, -1=下界, 1=末地）
     * @return Region 目录路径
     */
    [[nodiscard]] std::filesystem::path regionDir(i32 dimension) const {
        if (dimension == 0) {
            return m_worldDir / "region";
        } else if (dimension == -1) {
            return m_worldDir / "DIM-1" / "region";
        } else if (dimension == 1) {
            return m_worldDir / "DIM1" / "region";
        } else {
            return m_worldDir / ("DIM" + std::to_string(dimension)) / "region";
        }
    }

    /**
     * @brief 获取维度数据目录
     *
     * @param dimension 维度 ID
     * @return 数据目录路径
     */
    [[nodiscard]] std::filesystem::path dataDir(i32 dimension) const {
        if (dimension == 0) {
            return m_worldDir / "data";
        } else if (dimension == -1) {
            return m_worldDir / "DIM-1" / "data";
        } else if (dimension == 1) {
            return m_worldDir / "DIM1" / "data";
        } else {
            return m_worldDir / ("DIM" + std::to_string(dimension)) / "data";
        }
    }

    /**
     * @brief 获取维度目录
     *
     * @param dimension 维度 ID
     * @return 维度目录路径
     */
    [[nodiscard]] std::filesystem::path dimensionDir(i32 dimension) const {
        if (dimension == 0) {
            return m_worldDir;
        } else if (dimension == -1) {
            return m_worldDir / "DIM-1";
        } else if (dimension == 1) {
            return m_worldDir / "DIM1";
        } else {
            return m_worldDir / ("DIM" + std::to_string(dimension));
        }
    }

    // ========== 备份与删除 ==========

    /**
     * @brief 创建世界备份
     *
     * @return 成功返回备份文件路径，失败返回错误
     */
    [[nodiscard]] Result<std::filesystem::path> createBackup();

    /**
     * @brief 删除整个世界
     *
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] Result<void> deleteWorld();

    // ========== 目录创建 ==========

    /**
     * @brief 确保所有必要的目录存在
     *
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] Result<void> ensureDirectoriesExist();

private:
    explicit LevelSave(std::filesystem::path worldDir, String worldName);

    std::filesystem::path m_worldDir;
    String m_worldName;
};

} // namespace mc::world::save
