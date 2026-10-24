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
#include "common/util/nbt/Nbt.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/storage/backend/IStorageBackend.hpp"
#include "server/world/storage/blockentity/BlockEntityStorageManager.hpp"
#include "server/world/storage/core/LevelDatCodec.hpp"
#include "server/world/storage/core/SaveFormat.hpp"
#include "server/world/storage/core/WorldSessionLock.hpp"
#include "server/world/storage/db/ConsistencyMode.hpp"
#include "server/world/storage/db/RocksDBConfig.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include "server/world/storage/entity/EntityStorageManager.hpp"
#include "server/world/storage/player/PlayerDataManager.hpp"
#include "server/world/storage/player/PlayerSaveData.hpp"
#include "server/world/storage/section/SectionManager.hpp"
#include "server/world/storage/snapshot/BackupManager.hpp"
#include "server/world/storage/task/StorageTask.hpp"
#include "server/world/storage/task/StorageTaskManager.hpp"
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <cstddef>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::scoreboard {
class ScoreboardDataManager;
}

namespace mc::server {
class ServerChunkManager;
}

namespace mc::world::storage {

/**
 * @brief 单存档运行时存储配置
 *
 * 该配置控制单个已打开存档的持久化行为，
 * 包括一致性模式、备份开关和 RocksDB 参数。
 */
struct SingleLevelStorageConfig {
    ConsistencyMode consistencyMode = ConsistencyMode::Eventual;
    bool enableBackup = true;
    std::optional<RocksDBConfig> rocksdbConfig;
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
 * - 提供批量落盘原语，供上层（区块层）编排保存时机
 *
 * 该类不持有任何常驻数据缓存：区块数据常驻于世界层的内存区块表，由区块层
 * 决定何时序列化并调用本类的批量落盘入口；自动保存因此也由服务器驱动。
 *
 * 该类不负责存档发现、世界列表和目录选择；
 * 这类跨存档能力应由 `GlobalStorageManager` 负责。
 */
/**
 * @brief 一次进行中的区块卸载保存
 *
 * 作为保存任务的句柄在完成时移除追踪表中的条目：owner 的标识与表中的条目比对，
 * 确保只移除本次保存的条目（同一区块可能已被新的保存覆盖），随后唤醒等待该区块读盘的加载任务。
 */
struct PendingChunkSave {
    /// 区块坐标按 (x, z, dimension) 记录；读盘任务以同样的键登记等待
    struct Owner {
        ChunkCoord x = 0;
        ChunkCoord z = 0;
        DimensionId dimension = 0;
    };

    std::shared_ptr<const Owner> owner;

    [[nodiscard]] bool empty() const noexcept
    {
        return owner == nullptr;
    }
};

/// 追踪表中等待某个区块保存结束后再读盘的加载任务
struct PendingChunkSaveEntry {
    std::shared_ptr<const PendingChunkSave::Owner> save;
    std::vector<std::unique_ptr<StorageTask>> waiters;
};

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
     * 该方法不负责隐式保存；调用方必须在关闭前自行决定是否执行保存。
     */
    void close();

    /**
     * @brief 检查当前是否已打开存档
     * @return 已打开返回 true
     */
    [[nodiscard]] bool isOpen() const
    {
        return m_db != nullptr || m_backend != nullptr;
    }

    /**
     * @brief 从内存区块直接构建一个批次并提交，返回保存的段数。
     * @param dimension 目标维度
     * @param chunks 当前调用期间保持稳定的区块列表
     * @param sync 是否等待持久化完成
     */
    Result<size_t> saveChunks(DimensionId dimension, std::span<const ChunkData* const> chunks, bool sync);

    /**
     * @brief 把玩家数据缓存中标记为脏的条目落盘
     *
     * @return 成功保存的玩家数据条数
     */
    Result<size_t> flushPlayerData();

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
     * @return 若区块存在则返回已补齐高度图的区块数据，否则返回空 optional
     */
    [[nodiscard]] Result<std::optional<ChunkData>> loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension);

    /**
     * @brief 异步读取完整区块（Moonrise 式异步 I/O）
     *
     * Native 格式：两路并行提交到 ServerIO 线程池——
     *   路径 A：区块 section 数据（SectionManager::loadSectionsSync，24 个 section 批量读取）
     *   路径 B：方块实体（BlockEntityStorageManager::loadBlockEntitiesInChunk）
     * 两路 I/O 完成后在调用方线程（通常 Compute 池）组装 ChunkData。
     * 外来格式（JavaAnvil/BedrockLDB）：在 ServerIO worker 内加 m_foreignReadMutex 串行同步执行 m_backend->loadChunk。
     *
     * 与 loadChunk 的区别：loadChunk 在调用线程同步阻塞；loadChunkAsync 立即返回 future，
     * I/O 在 ServerIO 池完成，不阻塞主线程。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 目标维度
     * @param abortSignal 取消令牌，传入后任务执行前/执行中可取消
     * @param priority 任务优先级，传播到 ServerIO/ServerCompute 线程池任务（玩家附近区块 High，远处 Low）
     * @return 未来的区块数据，若区块不存在返回空 optional
     */
    std::future<Result<std::optional<ChunkData>>> loadChunkAsync(ChunkCoord x,
        ChunkCoord z,
        DimensionId dimension,
        std::shared_ptr<std::atomic<bool>> abortSignal = nullptr,
        util::TaskPriority priority = util::TaskPriority::Normal);

    /**
     * @brief 异步读取完整区块（回调版本）
     *
     * 与 loadChunkAsync（future 版）功能相同，但完成时通过 callback 回传结果，而非返回 future。
     * 回调签名为 (x, z, dimension, result)，result 为加载结果。
     *
     * 线程语义（Native 格式）：两路 I/O（section + blockEntity）在 ServerIO 线程并行读取，
     * 最后完成的一路把反序列化组装、生物群系合并和缺失高度图重建投递到 ServerCompute
     * 线程池（m_computeWorkerPool），completion 在 ServerCompute 线程调用。无 Compute 池（测试/独立模式）
     * 时降级为在 ServerIO worker 内联组装并调用 completion。
     * 外来格式：completion 在 ServerIO worker 线程调用（m_foreignReadMutex 串行化）。
     *
     * 调用方负责在回调中将结果投递回主线程（如通过 ServerChunkManager 的 _pendingLoadCompletes 队列）。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 目标维度
     * @param callback 完成回调（Native: ServerCompute 线程；外来格式: ServerIO 线程；无池降级: 调用线程）
     * @param abortSignal 取消令牌，传播到 ServerIO StorageTask 与 ServerCompute FunctionTask
     * @param priority 任务优先级，传播到 ServerIO/ServerCompute 线程池任务（玩家附近区块 High，远处 Low）
     */
    void loadChunkAsyncCallback(ChunkCoord x,
        ChunkCoord z,
        DimensionId dimension,
        std::function<void(ChunkCoord, ChunkCoord, DimensionId, Result<std::optional<ChunkData>>)> callback,
        std::shared_ptr<std::atomic<bool>> abortSignal = nullptr,
        util::TaskPriority priority = util::TaskPriority::Normal);

    /**
     * @brief 异步保存完整区块（回调版本）
     *
     * 在调用线程读取区块紧凑数组并直接构建拥有数据的 WriteBatch 快照，随后交给
     * ServerIO 提交；后台任务不借用区块数组，主线程后续修改不影响已捕获的数据。
     * 无 taskManager 时在调用线程同步保存；外来后端仍为只读空操作。
     * 回调由 ServerIO 调用，调用方负责将结果投递回主线程。
     *
     * @param chunk 区块数据（共享所有权，保存期间不可变）
     * @param dimension 目标维度
     * @param callback 完成回调（ServerIO 线程；无池降级时为调用线程）。参数为
     *        保存失败区块的坐标与结果；入参为空区块时没有可用的真实坐标，
     *        第一个参数为空 optional，调用方应据此区分"坐标未知的失败"
     * @param priority 任务优先级，传播到 ServerIO 线程池任务
     */
    void saveChunkAsyncCallback(std::shared_ptr<const ChunkData> chunk,
        DimensionId dimension,
        std::function<void(std::optional<ChunkPos>, Result<void>)> callback,
        util::TaskPriority priority = util::TaskPriority::Normal);

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
     * @param spawnData 世界出生数据（写入 spawn 复合标签）
     * @param clearWeatherTime 清晰天气剩余时间（刻）
     * @param rainTime 下雨剩余时间（刻）
     * @param raining 是否正在下雨
     * @param thunderTime 雷暴剩余时间（刻）
     * @param thundering 是否正在雷暴
     * @param initialized 世界是否已完成首次出生点初始化（false 时下次启动会重新计算出生点）
     * @param difficulty 当前世界难度（写回 Data.Difficulty）
     * @param difficultyLocked 难度是否锁定（写回 Data.DifficultyLocked）
     * @return 成功或错误
     */
    Result<void> saveLevelData(i64 gameTime,
        i64 dayTime,
        const RespawnData& spawnData,
        i32 clearWeatherTime,
        i32 rainTime,
        bool raining,
        i32 thunderTime,
        bool thundering,
        bool initialized,
        Difficulty difficulty,
        bool difficultyLocked);

    /**
     * @brief 读取调度事件列表
     *
     * 从 level.dat 的 ScheduledEvents 字段读取 NBT 事件列表。
     * 如果字段不存在，返回空列表。
     *
     * @return 成功返回 NBT 复合列表，失败返回错误
     */
    [[nodiscard]] Result<std::unique_ptr<nbt::tags::compound_list_tag>> loadScheduledEvents();

    /**
     * @brief 保存调度事件列表
     *
     * 将调度事件 NBT 列表写入 level.dat 的 ScheduledEvents 字段。
     *
     * @param events 调度事件 NBT 列表（可为空）
     * @return 成功或错误
     */
    Result<void> saveScheduledEvents(const nbt::tags::compound_list_tag& events);

    /**
     * @brief 读取末影龙战斗数据
     *
     * 从存档目录的 data/end_dragon_fight.json 文件读取末影龙战斗状态。
     * 如果文件不存在，返回 nullopt（新世界首次创建）。
     *
     * @return 成功返回 JSON 数据，失败返回错误
     */
    [[nodiscard]] Result<std::optional<nlohmann::json>> loadDragonFightData();

    /**
     * @brief 保存末影龙战斗数据
     *
     * 将末影龙战斗状态写入存档目录的 data/end_dragon_fight.json 文件。
     *
     * @param data 末影龙战斗数据的 JSON 对象
     * @return 成功返回空，失败返回错误
     */
    Result<void> saveDragonFightData(const nlohmann::json& data);

    /** @brief 从本存档的原生元数据读取世界 addon 属性，缺失时返回空值。 */
    [[nodiscard]] Result<std::optional<nlohmann::json>> loadAddonWorldProperties();

    /** @brief 原子写入世界 addon 属性；sync 表示等待 WAL 刷盘。 */
    Result<void> saveAddonWorldProperties(const nlohmann::json& data, bool sync);

    /**
     * @brief 注入存储 IO 线程池
     *
     * 注入后会同步创建/更新 `StorageTaskManager`，并传播到所有已创建的 `SectionManager`。
     *
     * @param workerPool IO 线程池指针
     */
    void setIoWorkerPool(util::UniversalWorkerPool* workerPool);

    /**
     * @brief 注入反序列化线程池（ServerCompute）
     *
     * loadChunkAsync 的区块解码、组装和缺失高度图重建在此线程池执行，与 IO 读取分离。
     * 未注入时降级为在 IO worker 内联执行反序列化。
     *
     * @param workerPool 反序列化线程池指针，可为 nullptr（降级内联）
     */
    void setComputeWorkerPool(util::UniversalWorkerPool* workerPool)
    {
        m_computeWorkerPool = workerPool;
    }

    /**
     * @brief 获取玩家数据管理器
     * @return 玩家数据管理器指针；未打开时返回 nullptr
     */
    [[nodiscard]] PlayerDataManager* playerDataManager()
    {
        return m_playerDataManager.get();
    }

    [[nodiscard]] const PlayerDataManager* playerDataManager() const
    {
        return m_playerDataManager.get();
    }

    /**
     * @brief 获取实体存储管理器
     * @return 实体存储管理器指针；未打开时返回 nullptr
     */
    [[nodiscard]] EntityStorageManager* entityStorage()
    {
        return m_entityStorage.get();
    }

    [[nodiscard]] const EntityStorageManager* entityStorage() const
    {
        return m_entityStorage.get();
    }

    /**
     * @brief 获取方块实体存储管理器
     * @return 方块实体存储管理器指针；未打开时返回 nullptr
     */
    [[nodiscard]] BlockEntityStorageManager* blockEntityStorage()
    {
        return m_blockEntityStorage.get();
    }

    [[nodiscard]] const BlockEntityStorageManager* blockEntityStorage() const
    {
        return m_blockEntityStorage.get();
    }

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
     * @brief 获取当前存储配置
     * @return 配置只读引用
     */
    [[nodiscard]] const SingleLevelStorageConfig& config() const
    {
        return m_config;
    }

    /**
     * @brief 修改一致性模式
     * @param mode 新一致性模式
     */
    void setConsistencyMode(ConsistencyMode mode);

    /**
     * @brief 获取已打开存档目录
     * @return 存档目录路径
     */
    [[nodiscard]] const std::filesystem::path& worldPath() const
    {
        return m_worldPath;
    }

    /**
     * @brief 获取当前已创建的维度列表
     * @return 已打开维度 ID 列表
     */
    [[nodiscard]] std::vector<DimensionId> getOpenDimensions() const;

    /**
     * @brief 获取存档格式信息
     * @return 格式信息
     */
    [[nodiscard]] const SaveFormatInfo& formatInfo() const
    {
        return m_config.formatInfo;
    }

    /**
     * @brief 检查是否使用外来格式后端
     * @return 使用外来格式返回 true
     */
    [[nodiscard]] bool isForeignFormat() const
    {
        return m_backend != nullptr;
    }

    /// 区块卸载保存的收尾需要清理进行中保存追踪表，该表是本类的内部状态，
    /// 只有掌握"该区块保存已完成且无新保存启动"这一前提的 ServerChunkManager 才可调用。
    friend class mc::server::ServerChunkManager;

private:
    /** @brief 登记由本存档持有的异步请求，覆盖读盘、恢复与保存收尾。 */
    void _beginAsyncOperation();
    /** @brief 请求不再访问存档或持有固定读取句柄后通知关闭等待者。 */
    void _finishAsyncOperation();

    std::mutex m_asyncOperationMutex;
    std::condition_variable m_asyncOperationCondition;
    size_t m_asyncOperationCount = 0;

    SectionManager& _sectionManager(DimensionId dimension);
    const SectionManager& _sectionManager(DimensionId dimension) const;
    [[nodiscard]] bool _hasSectionManager(DimensionId dimension) const;
    SectionManager* _createSectionManager(DimensionId dimension);

    Result<void> _openNativeFormat(const std::filesystem::path& worldPath);
    Result<void> _openForeignFormat(const std::filesystem::path& worldPath);

    /// loadChunkAsync 的核心实现：完成回调在 ServerIO worker 线程调用。
    /// future 版与 callback 版均委托到此。
    /// priority 传播到 ServerIO/ServerCompute 的 4 个 submit 站点（外来格式加载、section 加载、
    /// blockEntity 加载、Compute 组装）。
    void _loadChunkAsyncCore(ChunkCoord x,
        ChunkCoord z,
        DimensionId dimension,
        std::shared_ptr<std::atomic<bool>> abortSignal,
        std::function<void(Result<std::optional<ChunkData>>)> completion,
        util::TaskPriority priority = util::TaskPriority::Normal);

    /// 注册一个进行中的区块卸载保存（saveChunkAsyncCallback 调用）。
    /// 登记条目并返回该保存的句柄，由保存任务在完成时调用 _completePendingChunkSave 唤醒等待者。
    /// 后续对同一区块的 loadChunkAsync 在读盘前会等待该保存完成，
    /// 确保读到保存后的新数据（对齐 Moonrise GenericDataLoadTask 等待 UnloadTask）。
    PendingChunkSave _registerPendingChunkSave(ChunkCoord x, ChunkCoord z, DimensionId dimension);

    /// 保存任务完成时调用（ServerIO 线程，任何成功/失败/取消路径都必经）。
    /// 移除条目并把等待该区块读盘的加载任务投递到 ServerIO 池；
    /// 没有等待者时直接移除条目。
    void _completePendingChunkSave(PendingChunkSave save);

    /// 保存任务从未启动时的兜底清理（池未启动/队列被清空清空），唤醒等待者避免永久挂起。
    void _abandonPendingChunkSave(PendingChunkSave save);

    /// 移除指定区块的进行中卸载保存条目（保存完成后由 ServerChunkManager stage3 调用）。
    /// 仅在 ServerChunkManager 保证该区块保存已完成且无新保存启动时调用（主线程 stage3）。
    void _removePendingChunkSave(ChunkCoord x, ChunkCoord z, DimensionId dimension);

private:
    std::unique_ptr<RocksDBDatabase> m_db;
    std::optional<WorldSessionLock> m_sessionLock;
    std::unique_ptr<BackupManager> m_backupManager;
    std::unique_ptr<PlayerDataManager> m_playerDataManager;
    std::unique_ptr<EntityStorageManager> m_entityStorage;
    std::unique_ptr<BlockEntityStorageManager> m_blockEntityStorage;
    std::unique_ptr<mc::scoreboard::ScoreboardDataManager> m_scoreboardDataManager;
    util::UniversalWorkerPool* m_ioWorkerPool = nullptr;
    std::unique_ptr<StorageTaskManager> m_taskManager;

    /// 区块恢复线程池（ServerCompute）。loadChunkAsync 的紧凑数组恢复与
    /// biomes 组装 + ChunkData 合并阶段在此池执行。nullptr 时降级为 IO worker 内联。
    util::UniversalWorkerPool* m_computeWorkerPool = nullptr;

    mutable std::mutex m_sectionManagersMutex;
    std::unordered_map<DimensionId, std::unique_ptr<SectionManager>> m_sectionManagers;

    std::unique_ptr<IStorageBackend> m_backend;

    SingleLevelStorageConfig m_config;
    std::filesystem::path m_worldPath;

    /// 外来格式（JavaAnvil/BedrockLDB）读取互斥锁。
    /// 外来格式后端非线程安全，loadChunkAsync 在 ServerIO worker 内加此锁串行化。
    mutable std::mutex m_foreignReadMutex;

    /// 进行中的区块卸载保存追踪表（对齐 Moonrise NewChunkHolder.chunkDataUnload）。
    /// key = 区块描述 SectionKey{x, z, 0, dimension}，value = 等待该区块读盘的任务列表。
    /// 加载的 section 读任务在池线程里登记等待者而不阻塞，保存任务完成时在回调里把
    /// 等待者重新投递到 ServerIO，避免池线程自阻塞导致线程饥饿死锁。
    /// 线程安全：由 m_pendingChunkSavesMutex 保护。
    mutable std::mutex m_pendingChunkSavesMutex;
    std::unordered_map<SectionKey, PendingChunkSaveEntry, SectionKey::Hash> m_pendingChunkSaves;

    [[nodiscard]] RocksDBDatabase* _database()
    {
        return m_db.get();
    }

    [[nodiscard]] const RocksDBDatabase* _database() const
    {
        return m_db.get();
    }
};

} // namespace mc::world::storage
