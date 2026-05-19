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
#include "world/storage/core/WorldSessionLock.hpp"
#include "world/storage/core/WorldStoragePaths.hpp"
#include "world/storage/db/ConsistencyMode.hpp"
#include "world/storage/db/RocksDBConfig.hpp"
#include "world/storage/db/SectionKey.hpp"
#include "world/storage/list/WorldListService.hpp"
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

/**
 * @brief 存储配置
 *
 * 配置 WorldStorageService 的行为。
 */
struct WorldStorageConfig {
    /// 一致性模式（默认：最终一致）
    ConsistencyMode consistencyMode = ConsistencyMode::Eventual;

    /// 每维度 Section 缓存容量（默认：1024）
    size_t sectionCacheCapacity = 1024;

    /// 是否启用备份功能（默认：启用）
    bool enableBackup = true;

    /// RocksDB 详细配置（可选）
    std::optional<RocksDBConfig> rocksdbConfig;

    /// 是否计算内容哈希（用于去重，默认：关闭）
    bool computeHash = false;
};

/**
 * @brief 存储系统门面类 - 存储模块的唯一对外接口
 *
 * 职责：
 * - 管理 RocksDBDatabase 的生命周期
 * - 协调各子服务（SectionManager、WorldListService、BackupManager等）
 * - 提供统一的初始化和关闭接口
 * - 管理多维度存储
 *
 * 访问控制原则：
 * - 外部模块只能通过此类访问存储功能
 * - 内部模块（SectionManager、RocksDBDatabase等）不对外暴露
 *
 * 使用示例：
 * @code
 * WorldStorageService storage;
 *
 * WorldStorageConfig config;
 * config.consistencyMode = ConsistencyMode::Eventual;
 * config.sectionCacheCapacity = 2048;
 *
 * auto result = storage.open("/path/to/world", config);
 * if (result.success()) {
 *     // 通过子服务访问
 *     auto& sectionMgr = storage.sectionManager(DimensionId::Overworld);
 *     auto data = sectionMgr.loadSectionSync(key);
 *
 *     // 关闭存储
 *     storage.close();
 * }
 * @endcode
 */
class WorldStorageService {
    friend class mc::scoreboard::ScoreboardDataManager;

public:
    /**
     * @brief 构造函数
     */
    WorldStorageService();

    /**
     * @brief 析构函数
     *
     * 自动调用 close() 刷新数据并关闭数据库。
     */
    ~WorldStorageService();

    // 禁止拷贝
    WorldStorageService(const WorldStorageService&) = delete;
    WorldStorageService& operator=(const WorldStorageService&) = delete;

    // 允许移动
    WorldStorageService(WorldStorageService&&) noexcept;
    WorldStorageService& operator=(WorldStorageService&&) noexcept;

    // ========== 生命周期管理 ==========

    /**
     * @brief 打开世界存储
     *
     * 打开指定路径的世界存储，创建必要的目录结构，
     * 初始化数据库和所有子服务。
     *
     * @param worldPath 世界目录路径
     * @param config 配置选项
     * @return 成功或错误
     */
    Result<void> open(const std::filesystem::path& worldPath, const WorldStorageConfig& config);

    /**
     * @brief 关闭世界存储
     *
     * 刷新所有脏数据、释放资源、关闭数据库、释放会话锁。
     */
    void close();

    /**
     * @brief 检查是否已打开
     * @return true 如果存储已打开
     */
    [[nodiscard]] bool isOpen() const { return m_db != nullptr; }

    /**
     * @brief 刷新所有脏数据
     *
     * 遍历所有维度的 SectionManager，将脏 Section 写入磁盘。
     * 通常在自动保存或服务器关闭时调用。
     *
     * @return 成功刷新的 Section 数量，或错误
     */
    Result<size_t> flushAllDirty();

    /**
     * @brief 保存所有缓存数据
     *
     * 遍历所有维度的 SectionManager，将所有缓存的 Section 写入磁盘。
     * 用于 /save-all 或强制全量保存场景。
     *
     * @return 成功保存的 Section 数量，或错误
     */
    Result<size_t> saveAll();

    /**
     * @brief 将区块写入指定维度的 section 存储
     *
     * 该接口把区块与 section 编解码细节收口在存储门面内部，
     * 外部世界运行时不再直接依赖 SectionKey / SectionCodec。
     *
     * @param chunk 要写入的区块
     * @param dimension 目标维度
     * @return 成功或错误
     */
    Result<void> saveChunk(const ChunkData& chunk, DimensionId dimension);

    /**
     * @brief 从指定维度的 section 存储读取区块
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 目标维度
     * @return 若存在则返回区块数据，否则返回空 optional
     */
    [[nodiscard]] Result<std::optional<ChunkData>> loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension);

    // ========== 子服务访问 ==========

    /**
     * @brief 获取指定维度的 SectionManager
     *
     * 如果该维度的 SectionManager 尚未创建，会自动创建。
     *
     * @param dimension 维度ID
     * @return SectionManager 引用
     * @throws std::runtime_error 如果存储未打开
     */
    [[nodiscard]] SectionManager& sectionManager(DimensionId dimension);
    [[nodiscard]] const SectionManager& sectionManager(DimensionId dimension) const;

    /**
     * @brief 检查指定维度的 SectionManager 是否存在
     * @param dimension 维度ID
     * @return true 如果存在
     */
    [[nodiscard]] bool hasSectionManager(DimensionId dimension) const;

    /**
     * @brief 获取世界列表服务
     *
     * WorldListService 用于枚举、创建、删除世界。
     * 此服务不依赖于特定的世界存储，可以在 open() 之前使用。
     *
     * @return WorldListService 引用
     */
    [[nodiscard]] WorldListService& worldListService() { return *m_worldListService; }
    [[nodiscard]] const WorldListService& worldListService() const { return *m_worldListService; }

    /**
     * @brief 获取会话锁（如果持有）
     *
     * 会话锁在 open() 时获取，在 close() 时释放。
     * 如果存储未打开，返回 nullptr。
     *
     * @return 会话锁指针，可能为空
     */
    [[nodiscard]] WorldSessionLock* sessionLock()
    {
        return m_sessionLock.has_value() ? &m_sessionLock.value() : nullptr;
    }
    [[nodiscard]] const WorldSessionLock* sessionLock() const
    {
        return m_sessionLock.has_value() ? &m_sessionLock.value() : nullptr;
    }

    /**
     * @brief 获取备份管理器（如果可用）
     *
     * 备份功能需要在配置中启用（enableBackup = true）。
     *
     * @return 备份管理器指针，可能为空
     */
    [[nodiscard]] BackupManager* backupManager() { return m_backupManager.get(); }
    [[nodiscard]] const BackupManager* backupManager() const { return m_backupManager.get(); }

    /**
     * @brief 获取 IO Worker 池
     */
    [[nodiscard]] util::ServerWorkerPool* ioWorkerPool() { return m_ioWorkerPool; }
    [[nodiscard]] const util::ServerWorkerPool* ioWorkerPool() const { return m_ioWorkerPool; }

    /**
     * @brief 设置 IO Worker 池
     */
    void setIoWorkerPool(util::ServerWorkerPool* workerPool);

    /**
     * @brief 获取存储任务管理器
     */
    [[nodiscard]] StorageTaskManager* taskManager() { return m_taskManager.get(); }
    [[nodiscard]] const StorageTaskManager* taskManager() const { return m_taskManager.get(); }

    /**
     * @brief 获取玩家数据管理器
     *
     * 用于加载、保存和管理玩家数据。
     * 玩家数据存储在 players 列族中。
     *
     * @return 玩家数据管理器指针，如果存储未打开则为空
     */
    [[nodiscard]] PlayerDataManager* playerDataManager() { return m_playerDataManager.get(); }
    [[nodiscard]] const PlayerDataManager* playerDataManager() const { return m_playerDataManager.get(); }

    /**
     * @brief 获取记分板数据管理器
     *
     * 记分板持久化属于单存档状态，必须经由 WorldStorageService 访问。
     *
     * @return 记分板数据管理器指针，如果存储未打开则为空
     */
    [[nodiscard]] mc::scoreboard::ScoreboardDataManager* scoreboardDataManager() { return m_scoreboardDataManager.get(); }
    [[nodiscard]] const mc::scoreboard::ScoreboardDataManager* scoreboardDataManager() const
    {
        return m_scoreboardDataManager.get();
    }

    /**
     * @brief 创建备份
     *
     * 便捷方法，创建世界快照。
     *
     * @param name 备份名称
     * @param description 可选描述
     * @return 备份ID，或错误
     */
    Result<BackupID> createBackup(const std::string& name, const std::string& description = "");

    /**
     * @brief 清理旧备份
     *
     * 保留最近N个备份。
     *
     * @param keepCount 保留数量
     * @return 删除的备份数量，或错误
     */
    Result<size_t> pruneOldBackups(size_t keepCount);

    // ========== 配置 ==========

    /**
     * @brief 获取当前配置
     */
    [[nodiscard]] const WorldStorageConfig& config() const { return m_config; }

    /**
     * @brief 设置一致性模式
     *
     * 更新一致性模式并应用到所有后续写操作。
     * 已缓存的脏数据不受影响。
     *
     * @param mode 新的一致性模式
     */
    void setConsistencyMode(ConsistencyMode mode);

    /**
     * @brief 获取存储路径
     */
    [[nodiscard]] const std::filesystem::path& worldPath() const { return m_worldPath; }

    /**
     * @brief 根据世界名解析世界目录
     *
     * 目录布局规则属于存储门面职责，业务层不应直接依赖 WorldStoragePaths。
     *
     * @param worldName 存档目录名
     * @return 世界目录路径
     */
    [[nodiscard]] std::filesystem::path resolveWorldPath(const std::string& worldName) const;

    /**
     * @brief 返回默认 saves 目录
     *
     * 仅供通过门面间接访问存档根目录的上层逻辑使用。
     *
     * @return 默认 saves 目录路径
     */
    [[nodiscard]] std::filesystem::path savesDirectory() const;

    // ========== 统计信息 ==========

    /**
     * @brief 获取所有维度的缓存统计
     *
     * @return 维度ID到缓存统计的映射
     */
    [[nodiscard]] std::unordered_map<DimensionId, SectionCache::CacheStats> getCacheStats() const;

    /**
     * @brief 获取所有维度的脏Section总数量
     */
    [[nodiscard]] size_t getTotalDirtyCount() const;

    /**
     * @brief 获取当前打开的维度列表
     */
    [[nodiscard]] std::vector<DimensionId> getOpenDimensions() const;

    // ========== Section 缓存管理 ==========

    /**
     * @brief 设置指定维度的缓存容量
     *
     * @param dimension 维度ID
     * @param capacity 新的缓存容量
     */
    void setCacheCapacity(DimensionId dimension, size_t capacity);

    /**
     * @brief 清空指定维度的缓存
     *
     * 警告：这会丢弃所有未保存的数据！
     *
     * @param dimension 维度ID
     */
    void clearCache(DimensionId dimension);

    /**
     * @brief 清空所有维度的缓存
     *
     * 警告：这会丢弃所有未保存的数据！
     */
    void clearAllCaches();

private:
    /**
     * @brief 创建指定维度的 SectionManager
     */
    SectionManager* createSectionManager(DimensionId dimension);

    // 内部数据库
    std::unique_ptr<RocksDBDatabase> m_db;

    // 子服务
    std::unique_ptr<WorldListService> m_worldListService;
    std::optional<WorldSessionLock> m_sessionLock;
    std::unique_ptr<BackupManager> m_backupManager;
    std::unique_ptr<PlayerDataManager> m_playerDataManager;
    std::unique_ptr<mc::scoreboard::ScoreboardDataManager> m_scoreboardDataManager;
    util::ServerWorkerPool* m_ioWorkerPool = nullptr;
    std::unique_ptr<StorageTaskManager> m_taskManager;

    // 每维度一个 SectionManager
    mutable std::mutex m_sectionManagersMutex;
    std::unordered_map<DimensionId, std::unique_ptr<SectionManager>> m_sectionManagers;

    // 配置
    WorldStorageConfig m_config;
    std::filesystem::path m_worldPath;

    /**
     * @brief 获取内部数据库（仅供内部使用）
     *
     * 注意：此方法仅供存储模块内部使用，外部代码不应直接访问数据库。
     *
     * @return 数据库指针，可能为空
     */
    [[nodiscard]] RocksDBDatabase* database() { return m_db.get(); }
    [[nodiscard]] const RocksDBDatabase* database() const { return m_db.get(); }
};

} // namespace mc::world::storage
