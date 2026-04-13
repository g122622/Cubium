#pragma once

#include "../../common/world/chunk/ChunkData.hpp"
#include "../../common/world/chunk/SingleChunkLifecycleManager.hpp"
#include "../../common/world/chunk/ThreadedTicketLevelPropagator.hpp"
#include "../../common/world/chunk/ChunkLoadTicketManager.hpp"
#include "../../common/world/gen/chunk/IChunkGenerator.hpp"
#include "../../common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "../../common/util/concurrent/ReentrantAreaLock.hpp"
#include "chunk/ChunkTaskScheduler.hpp"
#include "chunk/ChunkHolderManager.hpp"
#include <unordered_map>
#include <memory>
#include <future>
#include <functional>

namespace mc::server {

// 前向声明
class ServerWorld;

namespace sync {
class ChunkSendManager;
}

/**
 * @brief 服务端区块管理器
 *
 * 参考 Moonrise 的 ChunkTaskScheduler 架构，协调区块加载、生成、卸载。
 * 使用多执行器异步生成区块，不阻塞主循环。
 *
 * 架构概览：
 * - ChunkTaskScheduler: 管理并行生成执行器、半径感知调度器、主线程任务队列
 * - ChunkHolderManager: 集中管理区块持有者、票据传播、卸载队列
 * - ThreadedTicketLevelPropagator: Section 分组的高效票据级别传播
 *
 * 使用方法：
 * @code
 * ServerChunkManager manager(world, std::move(generator));
 * manager.initialize();
 *
 * // 获取区块（同步）
 * ChunkData* chunk = manager.getChunk(x, z);
 *
 * // 获取区块（异步）
 * auto future = manager.getChunkAsync(x, z, &ChunkStatuses::FULL);
 *
 * // 主循环
 * manager.tick();
 *
 * // 关闭
 * manager.shutdown();
 * @endcode
 */
class ServerChunkManager {
public:
    // ============================================================================
    // 构造与析构
    // ============================================================================

    /**
     * @brief 创建区块管理器（带 ServerWorld）
     * @param world 服务端世界引用
     * @param generator 区块生成器
     */
    ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator);

    /**
     * @brief 创建区块管理器（无 ServerWorld，用于 IntegratedServer）
     * @param generator 区块生成器
     */
    explicit ServerChunkManager(std::unique_ptr<IChunkGenerator> generator);

    ~ServerChunkManager();

    // 禁止拷贝
    ServerChunkManager(const ServerChunkManager&) = delete;
    ServerChunkManager& operator=(const ServerChunkManager&) = delete;

    // ============================================================================
    // 生命周期
    // ============================================================================

    /**
     * @brief 初始化区块管理器
     */
    [[nodiscard]] Result<void> initialize();

    /**
     * @brief 关闭区块管理器
     */
    void shutdown();

    // ============================================================================
    // Worker 管理
    // ============================================================================

    /**
     * @brief 启动 Worker 线程
     */
    void startWorkers();

    /**
     * @brief 停止 Worker 线程
     */
    void stopWorkers();

    /**
     * @brief 检查 Worker 是否运行
     */
    [[nodiscard]] bool workersRunning() const;

    // ============================================================================
    // 区块访问（同步）
    // ============================================================================

    /**
     * @brief 获取区块（同步，阻塞直到完成）
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 区块数据指针，如果不存在返回 nullptr
     */
    [[nodiscard]] ChunkData* getChunk(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取区块（const 版本）
     */
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查区块是否存在
     */
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取或生成区块（同步，阻塞直到完成）
     *
     * 如果区块不存在，会阻塞生成到 FULL 状态。
     * 注意：此方法会阻塞调用线程，建议仅用于必要场景。
     * 优先使用 getChunkAsync() 进行异步生成。
     */
    [[nodiscard]] ChunkData* getChunkSync(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 卸载区块
     */
    void unloadChunk(ChunkCoord x, ChunkCoord z);

    // ============================================================================
    // 区块访问（异步）
    // ============================================================================

    /**
     * @brief 区块生成回调类型
     * @param success 是否成功
     * @param chunk 生成的区块数据（如果成功）
     */
    using ChunkCallback = std::function<void(bool success, ChunkData* chunk)>;

    /**
     * @brief 实体生成回调类型
     * @param entities 从区块生成的实体数据列表
     */
    using EntitySpawnCallback = std::function<void(const std::vector<SpawnedEntityData>& entities)>;

    /**
     * @brief 区块加载回调类型
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    using ChunkLoadedCallback = std::function<void(ChunkCoord x, ChunkCoord z)>;

    /**
     * @brief 设置实体生成回调
     *
     * 当没有 ServerWorld 时（如 IntegratedServer），通过此回调通知实体生成。
     */
    void setEntitySpawnCallback(EntitySpawnCallback callback) {
        // 若已有实体生成回调，直接抛错
        if (m_entitySpawnCallback) {
            throw std::logic_error("Entity spawn callback already set");
        }
        m_entitySpawnCallback = std::move(callback);
    }

    /**
     * @brief 设置区块加载回调
     *
     * 当区块完成加载/生成后调用，用于初始化光照等系统。
     */
    void setChunkLoadedCallback(ChunkLoadedCallback callback) {
        // 若已有加载回调，直接抛错
        if (m_chunkLoadedCallback) {
            throw std::logic_error("Chunk load callback already set");
        }
        m_chunkLoadedCallback = std::move(callback);
     }

    /**
     * @brief 设置区块卸载回调
     *
     * 当区块即将卸载前调用，用于通知系统清理相关数据。
     */
    void setChunkUnloadedCallback(ChunkLoadedCallback callback) {
        // 若已有卸载回调，直接抛错
        if (m_chunkUnloadedCallback) {
            throw std::logic_error("Chunk unload callback already set");
        }
        m_chunkUnloadedCallback = std::move(callback);
    }

    /**
     * @brief 设置区块发送管理器
     *
     * 区块卸载前会通知 ChunkSendManager 发送卸载包给客户端。
     */
    void setChunkSendManager(sync::ChunkSendManager* manager) { m_chunkSendManager = manager; }

    /**
     * @brief 获取区块 Future
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 目标状态
     * @return 区块 Future
     */
    [[nodiscard]] std::future<ChunkData*> getChunkAsync(ChunkCoord x, ChunkCoord z,
                                                         const ChunkStatus* targetStatus = &ChunkStatuses::FULL);

    /**
     * @brief 异步获取区块（回调版本）
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param callback 完成回调
     * @param targetStatus 目标状态
     */
    void getChunkAsync(ChunkCoord x, ChunkCoord z, ChunkCallback callback,
                       const ChunkStatus* targetStatus = &ChunkStatuses::FULL);

    // ============================================================================
    // 区块持有者
    // ============================================================================

    /**
     * @brief 获取或创建区块持有者
     */
    [[nodiscard]] SingleChunkLifecycleManager* getOrCreateChunkHolder(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取区块持有者
     */
    [[nodiscard]] SingleChunkLifecycleManager* getChunkHolder(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const SingleChunkLifecycleManager* getChunkHolder(ChunkCoord x, ChunkCoord z) const;

    // ============================================================================
    // 向后兼容别名
    // ============================================================================

    /**
     * @brief 向后兼容别名
     * @deprecated 使用 getOrCreateChunkHolder() 替代
     */
    [[nodiscard]] SingleChunkLifecycleManager* getOrCreateSingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z) {
        return getOrCreateChunkHolder(x, z);
    }

    /**
     * @brief 向后兼容别名
     * @deprecated 使用 getChunkHolder() 替代
     */
    [[nodiscard]] SingleChunkLifecycleManager* getSingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z) {
        return getChunkHolder(x, z);
    }

    /**
     * @brief 向后兼容别名
     * @deprecated 使用 getChunkHolder() 替代
     */
    [[nodiscard]] const SingleChunkLifecycleManager* getSingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z) const {
        return getChunkHolder(x, z);
    }

    // ============================================================================
    // 票据管理
    // ============================================================================

    /**
     * @brief 更新玩家位置
     *
     * 会自动更新区块加载票据。
     */
    void updatePlayerPosition(PlayerId player, f64 x, f64 z);

    /**
     * @brief 移除玩家
     */
    void removePlayer(PlayerId player);

    /**
     * @brief 强制加载区块
     */
    void forceChunk(ChunkCoord x, ChunkCoord z, bool force);

    /**
     * @brief 设置视距
     */
    void setViewDistance(i32 distance);

    /**
     * @brief 检查区块是否应该被加载
     */
    [[nodiscard]] bool shouldChunkLoad(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 处理票据更新
     */
    void processTicketUpdates();

    /**
     * @brief 获取票据级别
     */
    [[nodiscard]] i32 getTicketLevel(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取票据传播器
     */
    [[nodiscard]] world::ThreadedTicketLevelPropagator& getTicketPropagator() { return m_ticketPropagator; }
    [[nodiscard]] const world::ThreadedTicketLevelPropagator& getTicketPropagator() const { return m_ticketPropagator; }

    /**
     * @brief 获取票据管理器
     */
    [[nodiscard]] ChunkHolderManager& getHolderManager() { return *m_holderManager; }
    [[nodiscard]] const ChunkHolderManager& getHolderManager() const { return *m_holderManager; }

    /**
     * @brief 获取票据管理器（向后兼容）
     * @deprecated 使用 getHolderManager() 或 getTicketPropagator() 替代
     */
    [[nodiscard]] world::ChunkLoadTicketManager& ticketManager() { return m_ticketManager; }
    [[nodiscard]] const world::ChunkLoadTicketManager& ticketManager() const { return m_ticketManager; }

    /**
     * @brief 获取区块持有者数量（向后兼容）
     * @deprecated 使用 holderCount() 替代
     */
    [[nodiscard]] size_t singleChunkLifecycleManagerCount() const { return holderCount(); }

    [[nodiscard]] i32 viewDistance() const { return m_viewDistance; }

    // ============================================================================
    // 主循环
    // ============================================================================

    /**
     * @brief 每刻调用
     *
     * 处理区块加载/卸载、完成异步任务等。
     */
    void tick();

    // ============================================================================
    // 统计
    // ============================================================================

    /**
     * @brief 获取已加载区块数量
     */
    [[nodiscard]] size_t loadedChunkCount() const;

    /**
     * @brief 获取区块持有者数量
     */
    [[nodiscard]] size_t holderCount() const;

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] size_t pendingTaskCount() const;

    /**
     * @brief 获取生成器
     */
    [[nodiscard]] IChunkGenerator* generator() { return m_generator.get(); }
    [[nodiscard]] const IChunkGenerator* generator() const { return m_generator.get(); }

    /**
     * @brief 获取任务调度器
     */
    [[nodiscard]] ChunkTaskScheduler& getTaskScheduler() { return *m_taskScheduler; }
    [[nodiscard]] const ChunkTaskScheduler& getTaskScheduler() const { return *m_taskScheduler; }

private:
    // ============================================================================
    // 内部方法
    // ============================================================================

    /**
     * @brief 调度区块生成
     */
    void scheduleGeneration(SingleChunkLifecycleManager& holder, const ChunkStatus& targetStatus);

    /**
     * @brief 请求区块异步生成（同坐标请求自动合并）
     */
    void requestChunkGeneration(ChunkCoord x, ChunkCoord z,
                                const ChunkStatus& targetStatus,
                                Priority priority,
                                ChunkCallback callback,
                                std::shared_ptr<std::promise<ChunkData*>> promise);

    /**
     * @brief 取消指定区块的挂起请求
     */
    void cancelPendingGeneration(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 区块加载级别变化处理
     */
    void onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);

    /**
     * @brief 计算调度优先级（越小越高）
     */
    [[nodiscard]] Priority computeSchedulePriority(ChunkCoord x, ChunkCoord z,
                                                   const ChunkStatus& targetStatus,
                                                   i32 ticketLevel) const;

    /**
     * @brief 执行生成任务
     */
    void executeGenerationTask(SingleChunkLifecycleManager& holder, const ChunkStatus& status);

    /**
     * @brief 检查邻居区块状态
     * @return true 如果所有需要的邻居都已完成
     */
    [[nodiscard]] bool checkNeighborsReady(ChunkCoord x, ChunkCoord z, const ChunkStatus& status) const;

    /**
     * @brief 获取邻居区块
     * @param neighbors 输出数组（中心 + 8 个邻居 = 9 个区块）
     * 索引顺序：0=NW, 1=N, 2=NE, 3=W, 4=中心, 5=E, 6=SW, 7=S, 8=SE
     */
    void getNeighborChunks(
        ChunkCoord x,
        ChunkCoord z,
        std::array<IChunk*, 9>& neighbors,
        std::array<std::unique_ptr<IChunk>, 9>& neighborAdapters);

    /**
     * @brief 获取缓存区块的共享所有权
     */
    [[nodiscard]] std::shared_ptr<ChunkData> getChunkShared(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] std::shared_ptr<const ChunkData> getChunkShared(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 存储已生成区块并同步更新持有者状态
     * @return 缓存中的区块指针，失败返回 nullptr
     */
    [[nodiscard]] ChunkData* storeGeneratedChunk(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data);

    /**
     * @brief 处理完成的异步任务
     */
    void processCompletedTasks();

    /**
     * @brief 检查区块卸载
     */
    void checkChunkUnloading();

    /**
     * @brief 完成区块生成后的处理
     *
     * 提取 ChunkPrimer 中的实体数据，转换为 ChunkData，
     * 存入缓存，并将实体添加到世界。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param primer 已完成的区块生成器
     * @return 缓存中的区块指针，失败返回 nullptr
     */
    ChunkData* finalizeChunkGeneration(ChunkCoord x, ChunkCoord z, ChunkPrimer& primer);

    /**
     * @brief 区块坐标转键
     */
    [[nodiscard]] static u64 posToKey(ChunkCoord x, ChunkCoord z) {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z);
    }

    /**
     * @brief 从键解析坐标
     */
    static void keyToPos(u64 key, ChunkCoord& x, ChunkCoord& z) {
        x = static_cast<ChunkCoord>(static_cast<i32>(key >> 32));
        z = static_cast<ChunkCoord>(static_cast<i32>(key & 0xFFFFFFFF));
    }

    // ============================================================================
    // 成员变量
    // ============================================================================

    ServerWorld* m_world = nullptr;  // 可选，IntegratedServer 不需要
    std::unique_ptr<IChunkGenerator> m_generator;
    EntitySpawnCallback m_entitySpawnCallback;  // 实体生成回调（用于 IntegratedServer）
    ChunkLoadedCallback m_chunkLoadedCallback;  // 区块加载回调（用于光照初始化等）
    ChunkLoadedCallback m_chunkUnloadedCallback;  // 区块卸载回调（用于清理 POI 等）

    // 已完成的区块数据缓存
    std::unordered_map<u64, std::shared_ptr<ChunkData>> m_chunks;
    mutable std::mutex m_chunksMutex;

    // 同步生成保护，避免多线程同时对同一持有者执行同步生成
    mutable std::mutex m_syncGenerationMutex;

    struct PendingGeneration {
        u64 generation = 0;
        std::shared_ptr<std::atomic<bool>> cancelToken;
        std::vector<ChunkCallback> callbacks;
        std::vector<std::shared_ptr<std::promise<ChunkData*>>> promises;
    };
    std::unordered_map<u64, PendingGeneration> m_pendingGenerations;
    mutable std::mutex m_pendingGenerationsMutex;

    // 票据传播器（Section 分组）
    world::ThreadedTicketLevelPropagator m_ticketPropagator;

    // 票据管理器（向后兼容，用于玩家追踪等功能）
    world::ChunkLoadTicketManager m_ticketManager;

    // 区块持有者管理器
    std::unique_ptr<ChunkHolderManager> m_holderManager;

    // 任务调度器
    std::unique_ptr<ChunkTaskScheduler> m_taskScheduler;

    // 玩家追踪
    std::unordered_map<PlayerId, u64> m_playerPositions;  // PlayerId -> posToKey(x, z)
    std::unordered_map<u64, std::unordered_set<PlayerId>> m_playersByChunk;  // posToKey -> players
    mutable std::mutex m_playerMutex;
    i32 m_viewDistance = 10;

    // 强制加载区块
    std::unordered_set<u64> m_forcedChunks;
    mutable std::mutex m_forcedMutex;

    // 区块发送管理器（可选，由 MinecraftServer 设置）
    sync::ChunkSendManager* m_chunkSendManager = nullptr;

    // 统计
    u64 m_currentTick = 0;
    u64 m_lastUnloadCheck = 0;

    // 卸载检查间隔（单位：tick）
    static constexpr u32 UNLOAD_CHECK_INTERVAL = 20; // 1秒（20 tick/秒）
};

} // namespace mc::server
