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
#include "world/storage/backend/IStorageBackend.hpp"
#include "world/storage/core/SaveFormat.hpp"
#include "world/storage/core/WorldSessionLock.hpp"
#include "world/storage/db/ConsistencyMode.hpp"
#include "world/storage/db/RocksDBConfig.hpp"
#include "world/storage/db/SectionKey.hpp"
#include "world/storage/entity/EntityStorageManager.hpp"
#include "world/storage/player/PlayerDataManager.hpp"
#include "world/storage/section/SectionManager.hpp"
#include "world/storage/snapshot/BackupManager.hpp"
#include "world/storage/task/StorageTaskManager.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>

namespace mc::scoreboard {
class ScoreboardDataManager;
}

namespace mc::world::storage {

struct AutoSaveConfig;
class AutoSave;

/**
 * @brief 单存档运行时存储配置
 *
 * 该配置控制单个已打开存档的持久化行为，
 * 包括一致性模式、Section 缓存容量、备份开关和 RocksDB 参数。
 */
struct SingleLevelStorageConfig {
    ConsistencyMode consistencyMode = ConsistencyMode::Eventual;
    size_t sectionCacheCapacity = 1024;
    bool enableBackup = true;
    std::optional<RocksDBConfig> rocksdbConfig;
    bool computeHash = false;
    bool readonly = false;
    SaveFormatInfo formatInfo;
};

/**
 * @brief 单存档运行时存储门面
 *
 * 一个实例只对应一个已打开的存档。
 *
 * 主要职责：
 * - 打开/关闭单个存档目录
 * - 管理 RocksDB 数据库与会话锁生命周期
 * - 提供完整区块读写门面
 * - 暴露玩家数据、记分板、备份、异步存储任务等单存档子服务
 * - 内聚自动保存、手动保存和定期脏数据刷盘逻辑
 *
 * 该类不负责存档发现、世界列表和目录选择；
 * 这类跨存档能力应由 `GlobalStorageManager` 负责。
 */
class SingleLevelStorageManager {
    friend class mc::scoreboard::ScoreboardDataManager;

public:
    /**
     * @brief 构造未打开的单存档存储门面
     */
    SingleLevelStorageManager();

    /**
     * @brief 析构并关闭存档
     *
     * 如果当前仍处于打开状态，会自动执行 `close()` 释放资源。
     *
     * 析构函数不负责隐式保存；保存职责必须由上层显式编排。
     */
    ~SingleLevelStorageManager();

    SingleLevelStorageManager(const SingleLevelStorageManager&) = delete;
    SingleLevelStorageManager& operator=(const SingleLevelStorageManager&) = delete;
    SingleLevelStorageManager(SingleLevelStorageManager&&) noexcept;
    SingleLevelStorageManager& operator=(SingleLevelStorageManager&&) noexcept;

    /**
     * @brief 打开指定存档目录
     *
     * 会创建目录结构、获取 `session.lock`、打开数据库、
     * 初始化玩家数据、记分板、备份和存储任务相关状态。
     *
     * @param worldPath 存档目录路径
     * @param config 单存档存储配置
     * @return 成功或错误
     */
    Result<void> open(const std::filesystem::path& worldPath, const SingleLevelStorageConfig& config);

    /**
     * @brief 关闭当前存档
     *
     * 只负责关闭数据库/后端、停止自动保存并释放会话锁等资源。
     *
     * 该方法不负责隐式 `flushAllDirty()` 或 `saveAll()`；
     * 调用方必须在关闭前自行决定是否执行保存。
     */
    void close();

    /**
     * @brief 检查当前是否已打开存档
     * @return 已打开返回 true
     */
    [[nodiscard]] bool isOpen() const { return m_db != nullptr || m_backend != nullptr; }

    /**
     * @brief 刷新所有脏数据
     *
     * 仅刷新脏 Section 与脏玩家数据，适合自动保存与关闭流程。
     *
     * @return 成功刷新的数据数量
     */
    Result<size_t> flushAllDirty();

    /**
     * @brief 全量保存所有缓存数据
     *
     * 与 `flushAllDirty()` 不同，该方法会遍历所有缓存 Section 并强制写盘。
     *
     * @return 成功保存的数据数量
     */
    Result<size_t> saveAll();

    /**
     * @brief 保存完整区块
     * @param chunk 区块数据
     * @param dimension 目标维度
     * @return 成功或错误
     */
    Result<void> saveChunk(const ChunkData& chunk, DimensionId dimension);

    /**
     * @brief 读取完整区块
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 目标维度
     * @return 若区块存在则返回区块数据，否则返回空 optional
     */
    [[nodiscard]] Result<std::optional<ChunkData>> loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension);

    /**
     * @brief 读取玩家存档数据
     * @param uuid 玩家 UUID；基岩本地玩家可传 `~local_player`
     * @return 玩家数据，不存在返回空 optional
     */
    [[nodiscard]] Result<std::optional<PlayerSaveData>> loadPlayer(const std::string& uuid);

    /**
     * @brief 读取世界运行时元数据
     * @return level.dat 对应的运行时数据
     */
    [[nodiscard]] Result<LevelRuntimeData> loadLevelData();

    /**
     * @brief 保存世界运行时元数据
     *
     * 将时间、天气、出生点等运行时数据写入 level.dat。
     *
     * @param gameTime 游戏时间（刻）
     * @param dayTime 日光时间（刻）
     * @param spawnX 出生点 X
     * @param spawnY 出生点 Y
     * @param spawnZ 出生点 Z
     * @param spawnAngle 出生点朝向（角度）
     * @param clearWeatherTime 清晰天气剩余时间（刻）
     * @param rainTime 下雨剩余时间（刻）
     * @param raining 是否正在下雨
     * @param thunderTime 雷暴剩余时间（刻）
     * @param thundering 是否正在雷暴
     * @return 成功或错误
     */
    Result<void> saveLevelData(i64 gameTime,
        i64 dayTime,
        i32 spawnX,
        i32 spawnY,
        i32 spawnZ,
        f32 spawnAngle,
        i32 clearWeatherTime,
        i32 rainTime,
        bool raining,
        i32 thunderTime,
        bool thundering);

    /**
     * @brief 注入存储 IO 线程池
     *
     * 注入后会同步创建/更新 `StorageTaskManager`，并传播到所有已创建的 `SectionManager`。
     *
     * @param workerPool IO 线程池指针
     */
    void setIoWorkerPool(util::ServerWorkerPool* workerPool);

    /**
     * @brief 获取玩家数据管理器
     * @return 玩家数据管理器指针；未打开时返回 nullptr
     */
    [[nodiscard]] PlayerDataManager* playerDataManager() { return m_playerDataManager.get(); }
    [[nodiscard]] const PlayerDataManager* playerDataManager() const { return m_playerDataManager.get(); }

    /**
     * @brief 获取实体存储管理器
     * @return 实体存储管理器指针；未打开时返回 nullptr
     */
    [[nodiscard]] EntityStorageManager* entityStorage() { return m_entityStorage.get(); }
    [[nodiscard]] const EntityStorageManager* entityStorage() const { return m_entityStorage.get(); }

    /**
     * @brief 获取记分板数据管理器
     * @return 记分板数据管理器指针；未打开时返回 nullptr
     */
    [[nodiscard]] mc::scoreboard::ScoreboardDataManager* scoreboardDataManager()
    {
        return m_scoreboardDataManager.get();
    }
    [[nodiscard]] const mc::scoreboard::ScoreboardDataManager* scoreboardDataManager() const
    {
        return m_scoreboardDataManager.get();
    }

    /**
     * @brief 创建备份
     * @param name 备份名称
     * @param description 备份描述
     * @return 备份 ID，失败返回错误
     */
    Result<BackupID> createBackup(const std::string& name, const std::string& description = "");

    /**
     * @brief 清理旧备份
     * @param keepCount 保留数量
     * @return 删除的备份数量
     */
    Result<size_t> pruneOldBackups(size_t keepCount);

    /**
     * @brief 初始化自动保存
     * @param config 自动保存配置
     */
    void initializeAutoSave(const AutoSaveConfig& config);

    /**
     * @brief 关闭自动保存并执行收尾保存
     */
    void shutdownAutoSave();

    /**
     * @brief 启动自动保存
     */
    void startAutoSave();

    /**
     * @brief 停止自动保存
     */
    void stopAutoSave();

    /**
     * @brief 检查自动保存是否运行
     * @return 正在运行返回 true
     */
    [[nodiscard]] bool isAutoSaveRunning() const;

    /**
     * @brief 推进自动保存逻辑
     * @param tickCount 当前服务器 tick
     */
    void tickAutoSave(u64 tickCount);

    /**
     * @brief 立即执行一次脏数据保存
     * @return 保存的数据数量
     */
    Result<size_t> saveNow();

    /**
     * @brief 立即执行一次保存并附带快照
     * @param snapshotName 快照名称
     * @return 保存的数据数量
     */
    Result<size_t> saveNowWithSnapshot(const std::string& snapshotName);

    /**
     * @brief 获取当前存储配置
     * @return 配置只读引用
     */
    [[nodiscard]] const SingleLevelStorageConfig& config() const { return m_config; }

    /**
     * @brief 修改一致性模式
     * @param mode 新一致性模式
     */
    void setConsistencyMode(ConsistencyMode mode);

    /**
     * @brief 获取已打开存档目录
     * @return 存档目录路径
     */
    [[nodiscard]] const std::filesystem::path& worldPath() const { return m_worldPath; }

    /**
     * @brief 获取所有已打开维度的缓存统计
     * @return 维度到缓存统计的映射
     */
    [[nodiscard]] std::unordered_map<DimensionId, SectionCache::CacheStats> getCacheStats() const;

    /**
     * @brief 获取所有维度的脏数据总量
     * @return 脏数据总数
     */
    [[nodiscard]] size_t getTotalDirtyCount() const;

    /**
     * @brief 获取当前已创建的维度列表
     * @return 已打开维度 ID 列表
     */
    [[nodiscard]] std::vector<DimensionId> getOpenDimensions() const;

    /**
     * @brief 修改指定维度缓存容量
     * @param dimension 维度 ID
     * @param capacity 新缓存容量
     */
    void setCacheCapacity(DimensionId dimension, size_t capacity);

    /**
     * @brief 清空指定维度缓存
     * @param dimension 维度 ID
     */
    void clearCache(DimensionId dimension);

    /**
     * @brief 清空所有维度缓存
     */
    void clearAllCaches();

    /**
     * @brief 获取存档格式信息
     * @return 格式信息
     */
    [[nodiscard]] const SaveFormatInfo& formatInfo() const { return m_config.formatInfo; }

    /**
     * @brief 检查是否使用外来格式后端
     * @return 使用外来格式返回 true
     */
    [[nodiscard]] bool isForeignFormat() const { return m_backend != nullptr; }

private:
    SectionManager& sectionManager(DimensionId dimension);
    const SectionManager& sectionManager(DimensionId dimension) const;
    [[nodiscard]] bool hasSectionManager(DimensionId dimension) const;
    SectionManager* createSectionManager(DimensionId dimension);

    Result<void> openNativeFormat(const std::filesystem::path& worldPath);
    Result<void> openForeignFormat(const std::filesystem::path& worldPath);

private:
    std::unique_ptr<RocksDBDatabase> m_db;
    std::optional<WorldSessionLock> m_sessionLock;
    std::unique_ptr<BackupManager> m_backupManager;
    std::unique_ptr<PlayerDataManager> m_playerDataManager;
    std::unique_ptr<EntityStorageManager> m_entityStorage;
    std::unique_ptr<mc::scoreboard::ScoreboardDataManager> m_scoreboardDataManager;
    std::unique_ptr<AutoSave> m_autoSave;
    bool m_autoSaveInitialized = false;
    util::ServerWorkerPool* m_ioWorkerPool = nullptr;
    std::unique_ptr<StorageTaskManager> m_taskManager;

    mutable std::mutex m_sectionManagersMutex;
    std::unordered_map<DimensionId, std::unique_ptr<SectionManager>> m_sectionManagers;

    std::unique_ptr<IStorageBackend> m_backend;

    SingleLevelStorageConfig m_config;
    std::filesystem::path m_worldPath;

    [[nodiscard]] RocksDBDatabase* database() { return m_db.get(); }
    [[nodiscard]] const RocksDBDatabase* database() const { return m_db.get(); }
};

} // namespace mc::world::storage
