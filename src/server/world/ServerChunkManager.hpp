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

#include "ChunkGenerateTask.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/world/chunk/SingleChunkLifecycleManager.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::server {

class ServerWorld;

namespace sync {
class ChunkSendManager;
}

/**
 * @brief 服务端区块管理器
 *
 * 新实现把“区块请求”“存档来源解析”“邻居依赖推进”“异步生成执行”
 * 明确拆成统一的状态机推进流程：
 *
 * 1. 外部通过 `requestChunkAsync()` / `requestChunkSync()` 提交需求
 * 2. `SingleChunkLifecycleManager` 合并请求并产出下一步动作
 * 3. `ServerChunkManager` 按动作执行一次性存档解析或异步生成
 * 4. 邻居区块状态推进后，只唤醒受影响的阻塞请求，不再重新走完整请求入口
 *
 * 该类仍然是服务端区块子系统的唯一协调器，负责：
 * - 连接票据系统与单区块生命周期状态机
 * - 驱动来源解析（存档加载）
 * - 驱动异步生成任务提交与完成回收
 * - 维护内存区块缓存
 * - 处理区块卸载与回调通知
 */
class ServerChunkManager {
public:
    /**
     * @brief 区块请求完成回调
     *
     * @param success 请求是否成功完成
     * @param chunk 成功时返回区块指针；失败时返回 nullptr
     */
    using ChunkCallback = std::function<void(bool success, ChunkData* chunk)>;

    /**
     * @brief 区块生成阶段产生的实体回调
     *
     * 当当前管理器不直接挂在 `ServerWorld` 上时，
     * 使用该回调把区块生成过程中产生的实体交给外部处理。
     *
     * @param entities 本次区块生成产生的实体数据
     */
    using EntitySpawnCallback = std::function<void(const std::vector<SpawnedEntityData>& entities)>;

    /**
     * @brief 区块加载/完成回调
     *
     * 该回调用于区块从存档恢复或异步生成完成后通知上层。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    using ChunkLoadedCallback = std::function<void(ChunkCoord x, ChunkCoord z)>;

    /**
     * @brief 创建服务端区块管理器
     *
     * @param world 所属服务端世界
     * @param generator 区块生成器
     */
    ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator);

    /**
     * @brief 创建独立区块管理器
     *
     * 该构造路径主要用于不直接依附 `ServerWorld` 的场景，
     * 例如某些测试或集成式运行时。
     *
     * @param generator 区块生成器
     */
    explicit ServerChunkManager(std::unique_ptr<IChunkGenerator> generator);

    /**
     * @brief 析构函数
     */
    ~ServerChunkManager();

    ServerChunkManager(const ServerChunkManager&) = delete;
    ServerChunkManager& operator=(const ServerChunkManager&) = delete;

    /**
     * @brief 初始化区块管理器
     *
     * 当前实现不在这里启动 worker 池，
     * 线程池生命周期由 `MinecraftServer` 统一管理。
     *
     * @return 初始化结果
     */
    [[nodiscard]] Result<void> initialize();

    /**
     * @brief 关闭区块管理器
     *
     * 该函数会：
     * - 取消当前所有活跃请求
     * - 清理生命周期状态机中的等待者
     * - 清空内存区块缓存
     * - 释放所有单区块生命周期管理器
     */
    void shutdown();

    /**
     * @brief 注入外部计算线程池
     *
     * @param workerPool 由服务器统一持有的工作线程池
     */
    void setWorkerPool(util::ServerWorkerPool* workerPool) { m_workerPool = workerPool; }

    /**
     * @brief 获取当前已缓存的区块
     *
     * 该函数只查询内存缓存，不会触发存档解析或异步生成。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 内存中的区块指针；若不存在则返回 nullptr
     */
    [[nodiscard]] ChunkData* tryToGetChunkInMem(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取当前已缓存的区块（const 版本）
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 内存中的区块指针；若不存在则返回 nullptr
     */
    [[nodiscard]] const ChunkData* tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取当前已缓存区块的共享所有权
     *
     * 该接口用于跨线程安全持有区块数据快照。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 已缓存区块的共享指针；若不存在则返回空
     */
    [[nodiscard]] std::shared_ptr<ChunkData> tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取当前已缓存区块的共享所有权（const 版本）
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 已缓存区块的共享指针；若不存在则返回空
     */
    [[nodiscard]] std::shared_ptr<const ChunkData> tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 判断区块是否已存在于内存缓存中
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若该区块已缓存则返回 true
     */
    [[nodiscard]] bool hasChunkInMem(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 以同步方式请求区块
     *
     * 该函数会走与异步请求完全相同的状态机，
     * 区别只在于当前线程会阻塞等待最终结果。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 请求目标状态
     * @return 成功时返回区块指针；失败时返回 nullptr
     */
    [[nodiscard]] ChunkData* requestChunkSync(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus);

    /**
     * @brief 以同步方式请求 FULL 区块
     *
     * 这是最常用的同步便捷入口。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 成功时返回区块指针；失败时返回 nullptr
     */
    [[nodiscard]] ChunkData* requestFullChunkSync(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 以同步方式请求 FULL 区块
     *
     * 这是旧调用点迁移期间保留的薄别名，内部直接转发到 `requestFullChunkSync()`。
     */
    [[nodiscard]] ChunkData* getChunkSync(ChunkCoord x, ChunkCoord z) { return requestFullChunkSync(x, z); }

    /**
     * @brief 以异步 Future 方式请求区块
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 请求目标状态
     * @return 对应区块结果的 future
     */
    [[nodiscard]] std::future<ChunkData*> requestChunkAsync(
        ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus);

    /**
     * @brief 以异步 Future 方式请求 FULL 区块
     *
     * 这是旧调用点迁移期间保留的薄别名。
     */
    [[nodiscard]] std::future<ChunkData*> getChunkAsync(ChunkCoord x, ChunkCoord z)
    {
        return requestChunkAsync(x, z, ChunkStatuses::FULL);
    }

    /**
     * @brief 以异步 Future 方式请求区块
     *
     * 这是旧调用点迁移期间保留的薄别名。
     */
    [[nodiscard]] std::future<ChunkData*> getChunkAsync(ChunkCoord x, ChunkCoord z, const ChunkStatus* targetStatus)
    {
        MC_ASSERT_RELEASE(targetStatus != nullptr);
        return requestChunkAsync(x, z, *targetStatus);
    }

    /**
     * @brief 以异步回调方式请求区块
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 请求目标状态
     * @param callback 完成回调
     */
    void requestChunkAsync(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, ChunkCallback callback);

    /**
     * @brief 以异步回调方式请求 FULL 区块
     *
     * 这是旧调用点迁移期间保留的薄别名。
     */
    void getChunkAsync(ChunkCoord x, ChunkCoord z, ChunkCallback callback)
    {
        requestChunkAsync(x, z, ChunkStatuses::FULL, std::move(callback));
    }

    /**
     * @brief 以异步回调方式请求区块
     *
     * 这是旧调用点迁移期间保留的薄别名。
     */
    void getChunkAsync(ChunkCoord x, ChunkCoord z, ChunkCallback callback, const ChunkStatus* targetStatus)
    {
        MC_ASSERT_RELEASE(targetStatus != nullptr);
        requestChunkAsync(x, z, *targetStatus, std::move(callback));
    }

    /**
     * @brief 卸载指定区块
     *
     * 该函数会：
     * - 保存脏区块
     * - 取消活跃工作
     * - 清理等待者
     * - 移除内存缓存和生命周期状态机
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    void unloadChunkSync(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 更新玩家位置
     *
     * 该函数只更新票据系统，不直接做区块生成。
     * 真正的调度推进会在票据级别变化回调与 `tick()` 中完成。
     *
     * @param player 玩家 ID
     * @param x 世界坐标 X
     * @param z 世界坐标 Z
     */
    void updatePlayerPosition(PlayerId player, f64 x, f64 z);

    /**
     * @brief 移除玩家及其相关加载票据
     *
     * @param player 玩家 ID
     */
    void removePlayer(PlayerId player);

    /**
     * @brief 设置强制加载状态
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param force 是否强制加载
     */
    void forceChunk(ChunkCoord x, ChunkCoord z, bool force);

    /**
     * @brief 设置票据系统视距
     *
     * @param distance 新的视距
     */
    void setViewDistance(i32 distance);

    /**
     * @brief 设置票据级别变化回调
     *
     * @param callback 票据级别变化回调
     */
    void setTicketLevelChangeCallback(world::ChunkLoadTicketManager::LevelChangeCallback callback)
    {
        m_ticketManager.setLevelChangeCallback(std::move(callback));
    }

    /**
     * @brief 判断指定区块当前是否应当保持加载
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若票据系统判定该区块应加载则返回 true
     */
    [[nodiscard]] bool shouldChunkLoad(ChunkCoord x, ChunkCoord z) const
    {
        return m_ticketManager.shouldChunkLoad(x, z);
    }

    /**
     * @brief 显式处理票据系统积压更新
     */
    void processTicketUpdatesSync() { m_ticketManager.processUpdates(); }

    /**
     * @brief 获取票据管理器
     */
    [[nodiscard]] world::ChunkLoadTicketManager& ticketManager() { return m_ticketManager; }

    /**
     * @brief 获取票据管理器（const 版本）
     */
    [[nodiscard]] const world::ChunkLoadTicketManager& ticketManager() const { return m_ticketManager; }

    /**
     * @brief 获取当前视距
     */
    [[nodiscard]] i32 viewDistance() const { return m_ticketManager.viewDistance(); }

    /**
     * @brief 设置实体生成回调
     *
     * @param callback 回调函数；允许传入空回调以清空
     */
    void setEntitySpawnCallback(EntitySpawnCallback callback) { m_entitySpawnCallback = std::move(callback); }

    /**
     * @brief 设置区块加载完成回调
     *
     * @param callback 回调函数
     */
    void setChunkLoadedCallback(ChunkLoadedCallback callback) { m_chunkLoadedCallback = std::move(callback); }

    /**
     * @brief 设置区块卸载前回调
     *
     * @param callback 回调函数
     */
    void setChunkUnloadedCallback(ChunkLoadedCallback callback) { m_chunkUnloadedCallback = std::move(callback); }

    /**
     * @brief 设置区块发送管理器
     *
     * @param manager 外部区块发送管理器
     */
    void setChunkSendManager(sync::ChunkSendManager* manager) { m_chunkSendManager = manager; }

    /**
     * @brief 每 tick 推进区块管理状态
     *
     * 该函数会：
     * - 处理票据更新
     * - 检查是否需要卸载无需求区块
     * - 保持区块状态机持续前进
     */
    void tick();

    /**
     * @brief 获取当前已加载区块数量
     */
    [[nodiscard]] size_t loadedChunkCount() const;

    /**
     * @brief 遍历所有已加载区块
     *
     * @param callback 回调函数，返回 false 时停止遍历
     */
    void forEachLoadedChunk(const std::function<bool(ChunkData&)>& callback);

    /**
     * @brief 遍历所有已加载区块（const 版本）
     *
     * @param callback 回调函数，返回 false 时停止遍历
     */
    void forEachLoadedChunk(const std::function<bool(const ChunkData&)>& callback) const;

    /**
     * @brief 获取单区块生命周期管理器数量
     */
    [[nodiscard]] size_t lifecycleManagerCount() const;

    /**
     * @brief 获取单区块生命周期管理器数量
     *
     * 这是对外统计接口的兼容命名别名，语义与 `lifecycleManagerCount()` 完全一致。
     */
    [[nodiscard]] size_t singleChunkLifecycleManagerCount() const { return lifecycleManagerCount(); }

    /**
     * @brief 获取当前 worker 队列中的待处理任务数量
     *
     * @return 若未注入 worker 池则返回 0
     */
    [[nodiscard]] size_t pendingTaskCount() const;

    /**
     * @brief 获取当前区块生成器
     */
    [[nodiscard]] IChunkGenerator* generator() { return m_generator.get(); }

    /**
     * @brief 获取当前区块生成器（const 版本）
     */
    [[nodiscard]] const IChunkGenerator* generator() const { return m_generator.get(); }

private:
    /**
     * @brief 获取或创建单区块生命周期管理器
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 对应区块的生命周期管理器
     */
    [[nodiscard]] SingleChunkLifecycleManager& getOrCreateLifecycleManager(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 查询现有单区块生命周期管理器
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回指针，否则返回 nullptr
     */
    [[nodiscard]] SingleChunkLifecycleManager* findLifecycleManager(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 查询现有单区块生命周期管理器（const 版本）
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回指针，否则返回 nullptr
     */
    [[nodiscard]] const SingleChunkLifecycleManager* findLifecycleManager(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 统一提交区块请求
     *
     * 这是同步/异步请求的公共入口。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 请求目标状态
     * @param callback 异步回调，可为空
     * @param promise future 对应的 promise，可为空
     */
    void submitChunkRequest(ChunkCoord x,
        ChunkCoord z,
        const ChunkStatus& targetStatus,
        ChunkCallback callback,
        std::shared_ptr<std::promise<ChunkData*>> promise);

    /**
     * @brief 推进指定区块状态机
     *
     * 根据生命周期管理器返回的动作决策，
     * 执行存档解析、异步生成排队或等待者完成。
     *
     * @param lifecycleManager 要推进的单区块状态机
     * @param decision 生命周期管理器产出的动作决策
     */
    void advanceChunkState(
        SingleChunkLifecycleManager& lifecycleManager, const SingleChunkLifecycleManager::EnqueueDecision& decision);

    /**
     * @brief 执行一次存档来源解析
     *
     * @param lifecycleManager 目标区块的生命周期管理器
     */
    void resolveChunkSourceSync(SingleChunkLifecycleManager& lifecycleManager);

    /**
     * @brief 尝试排队一个异步区块生成任务
     *
     * @param lifecycleManager 目标区块的生命周期管理器
     * @param decision 当前调度决策
     */
    void enqueueChunkGenerationAsync(
        SingleChunkLifecycleManager& lifecycleManager, const SingleChunkLifecycleManager::EnqueueDecision& decision);

    /**
     * @brief 完成所有已就绪等待者
     *
     * @param lifecycleManager 目标区块的生命周期管理器
     */
    void completeReadyWaiters(SingleChunkLifecycleManager& lifecycleManager);

    /**
     * @brief 以失败方式完成指定等待者集合
     *
     * @param waiters 等待者集合
     */
    void failWaiters(std::vector<SingleChunkLifecycleManager::Waiter> waiters);

    /**
     * @brief 计算给定区块当前调度优先级
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 请求目标状态
     * @param ticketLevel 当前票据级别
     * @return 优先级，数值越小越优先
     */
    [[nodiscard]] i32 computeSchedulePriority(
        ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, i32 ticketLevel) const;

    /**
     * @brief 根据目标阶段计算需要先满足的邻居前置阶段
     *
     * @param targetStatus 请求目标阶段
     * @return 邻居必须达到的前置阶段；若无邻居依赖则返回 nullptr
     */
    [[nodiscard]] const ChunkStatus* getNeighborPrerequisiteStatus(const ChunkStatus& targetStatus) const;

    /**
     * @brief 检查给定区块的邻居依赖是否满足
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param prerequisiteStatus 邻居必须达到的前置阶段
     * @return 若所有依赖邻居均已满足则返回 true
     */
    [[nodiscard]] bool areNeighborsReady(ChunkCoord x, ChunkCoord z, const ChunkStatus& prerequisiteStatus) const;

    /**
     * @brief 在区块完成推进后，唤醒其影响范围内阻塞的邻居请求
     *
     * @param x 已推进区块的 X 坐标
     * @param z 已推进区块的 Z 坐标
     */
    void wakeBlockedNeighborsAsync(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 处理票据级别变化
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param oldLevel 旧级别
     * @param newLevel 新级别
     */
    void onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);

    /**
     * @brief 获取某阶段生成所需的邻居区块窗口
     *
     * @param x 中心区块 X 坐标
     * @param z 中心区块 Z 坐标
     * @param radius 邻域半径
     * @param centerChunk 中心区块视图
     * @param neighbors 输出区块窗口
     * @param loadedNeighbors 已加载邻居区块的共享持有容器
     * @param missingNeighbors 缺失邻居区块的临时占位容器
     */
    void collectNeighborChunks(ChunkCoord x,
        ChunkCoord z,
        i32 radius,
        IChunk* centerChunk,
        std::vector<IChunk*>& neighbors,
        std::vector<std::shared_ptr<ChunkData>>& loadedNeighbors,
        std::vector<std::unique_ptr<ChunkPrimer>>& missingNeighbors);

    /**
     * @brief 执行同步区块生成
     *
     * 该函数主要供同步请求路径使用，用于在当前线程直接推进到目标阶段。
     *
     * @param lifecycleManager 目标区块的生命周期管理器
     * @param targetStatus 目标生成阶段
     */
    void executeGenerationSync(SingleChunkLifecycleManager& lifecycleManager, const ChunkStatus& targetStatus);

    /**
     * @brief 完成一次异步生成结果并写入内存缓存
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param primer 生成完成的 ChunkPrimer
     * @return 成功发布后的缓存区块指针；失败时返回 nullptr
     */
    [[nodiscard]] ChunkData* finalizeGeneratedChunkSync(ChunkCoord x, ChunkCoord z, ChunkPrimer& primer);

    /**
     * @brief 把已生成区块放入内存缓存
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param data 区块数据所有权
     * @return 缓存中的区块指针
     */
    [[nodiscard]] ChunkData* storeChunkInMemorySync(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data);

    /**
     * @brief 保存一个脏区块的所有 section
     *
     * @param chunk 要保存的区块
     */
    void saveChunkSectionsSync(const ChunkData& chunk);

    /**
     * @brief 从存档同步加载一个区块
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回区块数据，否则返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<ChunkData> tryToLoadChunkFromStorageSync(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 检查并卸载无需求区块
     */
    void checkChunkUnloading();

    /**
     * @brief 将坐标转换为内部哈希键
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 64 位区块键
     */
    [[nodiscard]] static u64 posToKey(ChunkCoord x, ChunkCoord z) { return ChunkId(x, z, 0).toId(); }

    ServerWorld* m_world = nullptr;
    std::unique_ptr<IChunkGenerator> m_generator;
    EntitySpawnCallback m_entitySpawnCallback;
    ChunkLoadedCallback m_chunkLoadedCallback;
    ChunkLoadedCallback m_chunkUnloadedCallback;

    std::unordered_map<u64, std::unique_ptr<SingleChunkLifecycleManager>> m_lifecycleManagers;
    mutable std::mutex m_lifecycleManagersMutex;

    std::unordered_map<u64, std::shared_ptr<ChunkData>> m_chunks;
    mutable std::mutex m_chunksMutex;

    mutable std::mutex m_syncGenerationMutex;
    world::ChunkLoadTicketManager m_ticketManager;
    sync::ChunkSendManager* m_chunkSendManager = nullptr;
    util::ServerWorkerPool* m_workerPool = nullptr;

    u64 m_currentTick = 0;
    u64 m_lastUnloadCheckTick = 0;

    static constexpr u32 UNLOAD_CHECK_INTERVAL_TICKS = 20;
};

} // namespace mc::server
