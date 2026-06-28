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

#include "ChunkTaskScheduler.hpp"
#include "SingleChunkLifecycleManager.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::server {

class ServerWorld;
class ChunkTaskScheduler;
class ChunkProgressionTask;

namespace sync {
class ChunkSendManager;
}

/**
 * @brief 区块步骤依赖信息
 *
 * 用于缓存 ChunkStep 的直接依赖查询结果。
 */
struct ChunkStepDependencyInfo {
    bool hasDependencies = false; ///< 是否有任何邻居依赖
    i32 maxDirectRadius = 0;      ///< 直接依赖的最大半径
};

/**
 * @brief 服务端区块管理器
 *
 * 新实现把"区块请求""存档来源解析""邻居依赖推进""异步生成执行"
 * 明确拆成统一的状态机推进流程：
 *
 * 1. 外部通过 `requestChunkAsync()` / `requestChunkSync()` 提交需求
 * 2. `mc::world::chunk::SingleChunkLifecycleManager` 合并请求并产出下一步动作
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
    // ChunkTaskScheduler 和 ChunkProgressionTask 需要访问私有方法
    // （_executeStepTask / _finalizeGeneratedChunkSync / _tryToLoadChunkFromStorageSync /
    //  _storeChunkInMemorySync / _getOrCreateLifecycleManager / _findLifecycleManager /
    //  _failWaiters / _completeReadyWaiters）以驱动区块生成
    friend class ChunkTaskScheduler;
    friend class ChunkProgressionTask;

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
     * 同时创建/重建区块生成调度器 ChunkTaskScheduler，把同一 worker 池同时作为
     * parallelGenExecutor 和 radiusAwareExecutor（区域互斥由 submit 的 writeRadius
     * 重载保证，无需两个独立池）。
     *
     * @param workerPool 由服务器统一持有的工作线程池
     */
    void setWorkerPool(util::ServerWorkerPool* workerPool)
    {
        m_workerPool = workerPool;
        // 调度器持有 worker 池指针。同一池既用于 parallelCapable 状态（无区域互斥 submit），
        // 也用于写方块状态（带 writeRadius 的区域互斥 submit）。
        m_taskScheduler = std::make_unique<ChunkTaskScheduler>(*this, m_world, workerPool, workerPool);
    }

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
    void setTicketLevelChangeCallback(mc::world::chunk::ChunkLoadTicketManager::LevelChangeCallback callback)
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
    [[nodiscard]] mc::world::chunk::ChunkLoadTicketManager& ticketManager() { return m_ticketManager; }

    /**
     * @brief 获取票据管理器（const 版本）
     */
    [[nodiscard]] const mc::world::chunk::ChunkLoadTicketManager& ticketManager() const { return m_ticketManager; }

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
     * @brief 获取区块发送管理器
     * @return 区块发送管理器指针，可能为 nullptr
     */
    [[nodiscard]] sync::ChunkSendManager* chunkSendManager() { return m_chunkSendManager; }
    [[nodiscard]] const sync::ChunkSendManager* chunkSendManager() const { return m_chunkSendManager; }

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
     * @brief 调试：转储所有无法安全卸载的 holder 状态（用于诊断死锁/泄漏）
     *
     * 遍历 m_lifecycleManagers，输出 isSafeToUnload()==false 的 holder 的关键状态：
     * 坐标、currentGenStatus、requestedGenStatus、hasGenerationTask、blockingNeighbourCount、
     * waitingNeighbourCount、neighboursUsingThisChunkCount、hasFailedGeneration、sourceState、ticketCount。
     * 仅用于测试诊断，生产代码不应调用。
     */
    void _debugDumpStuckHolders() const;

    /**
     * @brief 获取当前区块生成器（const 版本）
     */
    [[nodiscard]] const IChunkGenerator* generator() const { return m_generator.get(); }

    /**
     * @brief 获取关联的 ServerWorld（可能为 nullptr，独立模式无 world）
     */
    [[nodiscard]] ServerWorld* world() { return m_world; }
    [[nodiscard]] const ServerWorld* world() const { return m_world; }

    /**
     * @brief 是否关联了 ServerWorld
     */
    [[nodiscard]] bool hasWorld() const { return m_world != nullptr; }

private:
    /**
     * @brief 获取或创建单区块生命周期管理器
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 对应区块的生命周期管理器
     */
    [[nodiscard]] mc::world::chunk::SingleChunkLifecycleManager& _getOrCreateLifecycleManager(
        ChunkCoord x, ChunkCoord z);

    /**
     * @brief 查询现有单区块生命周期管理器
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回指针，否则返回 nullptr
     */
    [[nodiscard]] mc::world::chunk::SingleChunkLifecycleManager* _findLifecycleManager(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 查询现有单区块生命周期管理器（const 版本）
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回指针，否则返回 nullptr
     */
    [[nodiscard]] const mc::world::chunk::SingleChunkLifecycleManager* _findLifecycleManager(
        ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 查询现有单区块生命周期管理器，以共享所有权返回
     *
     * 用于需要跨线程持有 holder 的场景（如 ChunkProgressionTask 在 worker 线程执行期间持锁 holder）。
     * 返回的 shared_ptr 保证 holder 在调用方释放前不被 unloadChunkSync 销毁。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回 shared_ptr，否则返回 nullptr
     */
    [[nodiscard]] std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> _findLifecycleManagerShared(
        ChunkCoord x, ChunkCoord z);

    /**
     * @brief 查询现有单区块生命周期管理器的共享实现
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回指针，否则返回 nullptr
     */
    [[nodiscard]] mc::world::chunk::SingleChunkLifecycleManager* _doFindLifecycleManager(
        ChunkCoord x, ChunkCoord z) const;

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
    void _submitChunkRequest(ChunkCoord x,
        ChunkCoord z,
        const ChunkStatus& targetStatus,
        ChunkCallback callback,
        std::shared_ptr<std::promise<ChunkData*>> promise);

    /**
     * @brief 推进指定区块状态机
     *
     * 根据生命周期管理器返回的动作决策，
     * 执行存档解析、生成调度或等待者完成。
     *
     * @param lifecycleManager 要推进的单区块状态机
     * @param decision 生命周期管理器产出的动作决策
     */
    void _advanceChunkState(mc::world::chunk::SingleChunkLifecycleManager& lifecycleManager,
        const mc::world::chunk::SingleChunkLifecycleManager::EnqueueDecision& decision);

    /**
     * @brief 执行一次存档来源解析
     *
     * @param lifecycleManager 目标区块的生命周期管理器
     */
    void _resolveChunkSourceSync(mc::world::chunk::SingleChunkLifecycleManager& lifecycleManager);

    /**
     * @brief 在调度锁保护下推进区块生成调度
     *
     * 持有 schedulingLockArea(x, z, getMaxAccessRadius()) 后调用
     * `ChunkTaskScheduler::schedule`，把区块推进到下一个状态。生成调度完全委托给
     * `ChunkTaskScheduler`（邻居检查、任务提交、完成回调都在其中）。
     *
     * @param lifecycleManager 目标区块的生命周期管理器
     * @param targetStatus 目标生成状态
     */
    void _scheduleGeneration(
        mc::world::chunk::SingleChunkLifecycleManager& lifecycleManager, const ChunkStatus& targetStatus);

    /**
     * @brief 完成所有已就绪等待者
     *
     * @param lifecycleManager 目标区块的生命周期管理器
     */
    void _completeReadyWaiters(mc::world::chunk::SingleChunkLifecycleManager& lifecycleManager);

    /**
     * @brief 以失败方式完成指定等待者集合
     *
     * @param waiters 等待者集合
     */
    void _failWaiters(std::vector<mc::world::chunk::SingleChunkLifecycleManager::Waiter> waiters);

    /**
     * @brief 计算给定区块当前调度优先级
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 请求目标状态
     * @param ticketLevel 当前票据级别
     * @return 优先级，数值越小越优先
     */
    [[nodiscard]] i32 _computeSchedulePriority(
        ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, i32 ticketLevel) const;

    /**
     * @brief 处理票据级别变化
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param oldLevel 旧级别
     * @param newLevel 新级别
     */
    void _onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);

    /**
     * @brief 对单个区块执行单个生成阶段的任务
     *
     * 由 ChunkProgressionTask 在执行器线程中调用。根据 status 分派到 IChunkGenerator 的
     * 对应方法（generateStructureStarts/generateBiomes/...）。
     *
     * @param chunk 目标区块 Primer（中心区块，可变）
     * @param status 当前要执行的生成阶段
     * @param region 世界生成区域（邻居为可变 ChunkPrimer，由 StaticChunkCache2D 构造）
     */
    void _executeStepTask(ChunkPrimer& chunk, const ChunkStatus& status, WorldGenRegion& region);

    /**
     * @brief 完成一次生成结果并写入内存缓存
     *
     * 由 ChunkProgressionTask 在 FULL 完成时调用：primer.toChunkData（非破坏性，返回 shared_ptr 共享
     * 同一份 ChunkData）+ _storeChunkInMemorySync（共享所有权重载）+ spawnEntitiesFromChunkGeneration
     * + _postProcessChunk。primer 仍持有同一份 ChunkData 供邻居引用（对齐 Moonrise currentChunk 生命周期）。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param primer 生成完成的 ChunkPrimer
     * @return 成功发布后的缓存区块指针；失败时返回 nullptr
     */
    [[nodiscard]] ChunkData* _finalizeGeneratedChunkSync(ChunkCoord x, ChunkCoord z, ChunkPrimer& primer);

    /**
     * @brief 达到请求目标状态时唤醒等待者（非 FULL 路径）
     *
     * 由 ChunkTaskScheduler::onChunkGenComplete 在 currentGenStatus >= requestedGenStatus 时调用。
     * FULL 已由 _finalizeGeneratedChunkSync 处理（存入 m_chunks + markLoadedFromStorageReady +
     * _completeReadyWaiters），本方法处理非 FULL 目标：用 primer 的 ChunkData 直接完成等待者
     * （promise/callback），不存入 m_chunks（m_chunks 仅保留 FULL 区块，保证 tryToGetChunkInMem
     * 快速路径不返回中间状态区块）。markGenerationReady 标记 Ready，使后续相同/更低状态请求走
     * submitRequest 快速路径。重复调用安全（Ready 后跳过）。
     *
     * @param holder 达到目标状态的生命周期管理器
     * @param completedStatus 本次完成的状态
     */
    void _publishGeneratedChunk(
        mc::world::chunk::SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus);

    /**
     * @brief 对已完成的区块执行后处理生成
     *
     * 遍历区块的后处理位置，对流体方块调度流体 tick，
     * 对需要形状更新的方块执行 updateFromNeighbourShapes。
     *
     * @param chunk 已完成的区块数据
     */
    void _postProcessChunk(ChunkData& chunk);

    /**
     * @brief 把已生成区块放入内存缓存
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param data 区块数据所有权
     * @return 缓存中的区块指针
     */
    [[nodiscard]] ChunkData* _storeChunkInMemorySync(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data);

    /**
     * @brief 把已生成区块放入内存缓存（共享所有权重载）
     *
     * 由 ChunkProgressionTask 在 FULL 完成时调用：primer.toChunkData() 返回 shared_ptr（非破坏性，
     * primer 仍持有同一份 ChunkData 供邻居引用），直接发布到内存缓存，与 primer 共享所有权。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param data 区块数据共享所有权
     * @return 缓存中的区块指针
     */
    [[nodiscard]] ChunkData* _storeChunkInMemorySync(ChunkCoord x, ChunkCoord z, std::shared_ptr<ChunkData> data);

    /**
     * @brief 保存一个脏区块的所有 section
     *
     * @param chunk 要保存的区块
     */
    void _saveChunkSectionsSync(const ChunkData& chunk);

    /**
     * @brief 从存档同步加载一个区块
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若存在则返回区块数据，否则返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<ChunkData> _tryToLoadChunkFromStorageSync(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 检查并卸载无需求区块
     */
    void _checkChunkUnloading();

    /**
     * @brief 增加有玩家附近的区块的居住时间
     *
     * 每个游戏 tick 对有玩家追踪的区块调用 incrementInhabitedTime(1)，
     * 这与 MC Java 版的 ChunkHolder.tick() 中增加 InhabitedTime 的逻辑一致。
     */
    void _incrementInhabitedTime();

    /**
     * @brief 入队主线程后处理任务（worker 线程调用）
     *
     * 对齐 Moonrise offThreadPendingFullLoadUpdate：worker 线程在 FULL 完成/存档加载时
     * 不能直接调用主线程独占的世界状态（onChunkLoaded / m_chunkLoadedCallback /
     * spawnEntitiesFromChunkGeneration / _postProcessChunk 触及 ServerTickList、EntityManager、
     * setBlockState、POI、光照等，均非线程安全）。入队后由主线程 tick() 的
     * _drainPendingPostProcess 出队执行。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param spawnedEntities 区块生成产生的实体（生成路径传入，存档加载路径传空）
     * @param needsPostProcess 是否需要 _postProcessChunk（生成路径 true，存档加载路径 false）
     */
    void _enqueuePostProcess(
        ChunkCoord x, ChunkCoord z, std::vector<SpawnedEntityData>&& spawnedEntities, bool needsPostProcess);

    /**
     * @brief 出队并执行主线程后处理任务（仅主线程调用）
     *
     * 在 tick() 中调用。依次执行 onChunkLoaded / m_chunkLoadedCallback /
     * spawnEntitiesFromChunkGeneration / _postProcessChunk，全部在主线程完成。
     */
    void _drainPendingPostProcess();

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

    std::unordered_map<u64, std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager>> m_lifecycleManagers;
    mutable std::mutex m_lifecycleManagersMutex;

    std::unordered_map<u64, std::shared_ptr<ChunkData>> m_chunks;
    mutable std::mutex m_chunksMutex;

    /**
     * @brief 主线程后处理队列（对齐 Moonrise offThreadPendingFullLoadUpdate）
     *
     * worker 线程在 FULL 完成（_finalizeGeneratedChunkSync）或存档加载（executeEmptyLoad）时
     * 入队，主线程 tick() 出队执行。onChunkLoaded / m_chunkLoadedCallback /
     * spawnEntitiesFromChunkGeneration / _postProcessChunk 触及的主线程独占状态
     * （ServerTickList、EntityManager、setBlockState、POI、光照）必须在主线程执行，
     * 不能在 worker 线程调用（会导致 ServerTickList 数据竞争等崩溃）。
     */
    struct PendingPostProcess {
        ChunkCoord x = 0;
        ChunkCoord z = 0;
        std::vector<SpawnedEntityData> spawnedEntities;
        bool needsPostProcess = false; ///< true: 生成完成（需 _postProcessChunk）；false: 仅 onChunkLoaded+callback
    };
    std::mutex m_pendingPostProcessMutex;
    std::vector<PendingPostProcess> m_pendingPostProcess;

    /**
     * @brief 区块生成调度核心（对齐 Moonrise ChunkTaskScheduler）
     *
     * 持有 ReentrantAreaLock 保证 schedule/checkNeighbour/onChunkGenComplete 的原子性。
     * nullptr 表示无 worker 池（独立/测试模式），ChunkTaskScheduler 内部在线执行任务。
     */
    std::unique_ptr<ChunkTaskScheduler> m_taskScheduler;

    mc::world::chunk::ChunkLoadTicketManager m_ticketManager;
    sync::ChunkSendManager* m_chunkSendManager = nullptr;
    util::ServerWorkerPool* m_workerPool = nullptr;

    u64 m_currentTick = 0;
    u64 m_lastUnloadCheckTick = 0;

    static constexpr u32 UNLOAD_CHECK_INTERVAL_TICKS = 20;
};

} // namespace mc::server
