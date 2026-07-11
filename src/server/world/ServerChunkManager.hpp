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
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::server {

class ServerWorld;
class ChunkTaskScheduler;
class ChunkProgressionTask;

// 前向声明：测试类需访问 ServerChunkManager 私有成员（_enqueuePostProcess）以验证去重逻辑。
class ServerChunkManagerPostProcessTest;

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
    // （_executeStepTask / _finalizeGeneratedChunkSync / _storeChunkInMemorySync /
    //  _getOrCreateLifecycleManager / _findLifecycleManager /
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
     * @brief 获取区域互斥执行器（worker 池）
     *
     * 返回注入的 ServerWorkerPool，供运行时光照等需要 writeRadius>0 区域互斥的
     * worker 任务提交（RuntimeLightTask 经 submit(writeRadius=2) 串行化重叠区域
     * nibble 写）。启动早期或测试环境未注入时返回 nullptr，调用方应 fallback 同步路径。
     */
    [[nodiscard]] util::ServerWorkerPool* radiusAwareExecutor() noexcept { return m_workerPool; }
    [[nodiscard]] const util::ServerWorkerPool* radiusAwareExecutor() const noexcept { return m_workerPool; }

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
     * @brief 设置加载区块软上限（0 = 不限制）
     *
     * 当加载区块数超过此值时，_checkChunkUnloading 会优先卸载票级最高（最远）的区块，
     * 防止极端视距或强制加载导致内存无界增长。对齐 Moonrise maxLoaded 配置项。
     *
     * @param maxLoadedChunks 加载区块软上限，0 表示不限制
     */
    void setMaxLoadedChunks(i32 maxLoadedChunks) { m_maxLoadedChunks = maxLoadedChunks; }

    /**
     * @brief 获取当前加载区块软上限
     */
    [[nodiscard]] i32 maxLoadedChunks() const noexcept { return m_maxLoadedChunks; }

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
    void _debugDumpStuckHolders();

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
     * @brief 进行中的异步存档加载追踪表条目（去重合并）
     *
     * ownerLifecycle：发起 loadChunkAsyncCallback 的所有者 SCLM（其 Result 在 _onChunkLoadComplete 被 move）。
     * attachedWaiters：加载期间因 cancel-revive 重建的附加等待者 SCLM。所有者完成加载后，
     *   _onChunkLoadComplete 遍历 attachedWaiters 推进其状态机（命中→markLoadedFromStorageReady，
     *   缺失→noteStorageResolved(false) 走生成），避免对同一区块重复发起 RocksDB 读取。
     *   见 SCM 层加载去重设计（对齐 Moonrise chunkTasks 合并）。
     *
     * 定义前置：_fanOutAttachedWaiters 声明引用 PendingLoadEntry::AttachedWaiter，需先于其定义。
     */
    struct PendingLoadEntry {
        std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> ownerLifecycle;
        struct AttachedWaiter {
            std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> lifecycle;
        };
        std::vector<AttachedWaiter> attachedWaiters;
    };

    /**
     * @brief 执行一次存档来源解析（异步）
     *
     * 把存档读取（loadChunkAsyncCallback）投递到存储层（ServerIO 读盘 + ServerCompute 反序列化），
     * 立即返回不阻塞主线程。完成后回调在 ServerCompute 线程把结果入队 m_pendingLoadCompletes，
     * 主线程 tick() 的 _drainPendingLoadCompletes 出队执行 _onChunkLoadComplete（noteStorageResolved +
     * _storeChunkInMemorySync + _enqueuePostProcess / _advanceChunkState）。
     *
     * 异步期间 SCLM 处于 ResolvingStorage，并发 submitRequest 走 no-op（不重复触发解析）。
     * abortSignal 由 SCLM 提供，unloadChunkSync 的 cancelActiveWork 会置位取消异步加载。
     *
     * @param lifecycleManager 目标区块的生命周期管理器（必须处于 ResolvingStorage）
     */
    void _resolveChunkSourceSync(mc::world::chunk::SingleChunkLifecycleManager& lifecycleManager);

    /**
     * @brief 异步存档加载完成回调（主线程执行）
     *
     * 在 _drainPendingLoadCompletes 中出队调用。处理：
     * - 校验 SCLM 仍存活且为同一实例（异步期间可能被 unload 重建）
     * - 存档命中：_storeChunkInMemorySync + markLoadedFromStorageReady(FULL) + _completeReadyWaiters
     *   + 直接调用 onChunkLoaded/m_chunkLoadedCallback（主线程路径，不走 _enqueuePostProcess；
     *     由 m_postProcessedChunks 去重，防止重复执行）
     * - 存档缺失：noteStorageResolved(false) + _advanceChunkState（走 StorageMissing→生成链路）
     * - 从 m_pendingLoadTasks 移除追踪条目（owner 校验），扇出 attachedWaiters（命中→Ready，缺失→生成）
     * - owner 已 unload（ownerAlive=false）：跳过 owner 推进，扇出 attachedWaiters 走生成路径
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 维度
     * @param result 加载结果（成功含 ChunkData，失败/不存在为空/错误）
     * @param lifecycleHolder 异步发起时持有的 SCLM 共享指针，用于实例一致性校验
     */
    void _onChunkLoadComplete(ChunkCoord x,
        ChunkCoord z,
        mc::DimensionId dimension,
        mc::Result<std::optional<mc::ChunkData>> result,
        std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> lifecycleHolder);

    /**
     * @brief 扇出存档加载结果到附加等待者（SCM 层去重合并的等待者）
     *
     * owner 完成加载后调用。对每个仍在 m_lifecycleManagers 中的等待者 SCLM：
     *   - hit=true（存档命中）：noteStorageResolved(true) + markLoadedFromStorageReady(FULL) +
     *     _completeReadyWaiters（区块已在 m_chunks，不重复存储）。
     *   - hit=false（存档缺失/失败/owner 已卸载）：noteStorageResolved(false) + _advanceChunkState 走生成。
     *
     * 实例校验：等待者可能已被 unload 重建（_findLifecycleManager != waiter.lifecycle.get()），跳过。
     * 等待者 SCLM 被附加时其 sourceState 已是 ResolvingStorage（submitRequest 设置），noteStorageResolved 合法。
     */
    void _fanOutAttachedWaiters(
        ChunkCoord x, ChunkCoord z, std::vector<PendingLoadEntry::AttachedWaiter>& waiters, bool hit);

    /**
     * @brief 出队并执行异步存档加载完成回调（仅主线程调用）
     *
     * 在 tick() 中调用，把 worker 线程入队的 m_pendingLoadCompletes 逐个交给 _onChunkLoadComplete。
     */
    void _drainPendingLoadCompletes();

    /**
     * @brief 在调度锁下为"未解析存档来源"的 holder 发起存档解析（对齐 Moonrise getEmptyChunk=磁盘加载）
     *
     * 由 ChunkTaskScheduler::schedule 在 holder.currentChunk 为空且 sourceState==Unknown 时调用。
     * Unknown 表示该 holder 由 checkNeighbour 按需创建、尚未判定存档是否存在。对齐 Moonrise：
     * pristine holder（currentGenStatus==null）经 ChunkLoadTask 先尝试磁盘读取，磁盘缺失/失败才回落到
     * 空 ProtoChunk（getEmptyChunk）。Cubium 把"磁盘读取"拆为异步存档解析（_resolveChunkSourceSync），
     * "空 Primer 创建"拆为 executeEmptyLoad，二者绝不能对同一 holder 并发——否则 EMPTY 任务在
     * ResolvingStorage 窗口运行返回 false → onChunkGenFailed → markFailed → 永久阻塞（依赖图泄漏）。
     *
     * 本方法把 Unknown holder 推进到 ResolvingStorage 并发起异步存档读取，不创建任何生成任务。
     * 存档解析完成后（_onChunkLoadComplete）由现有管线重新驱动：
     *   - 命中：markLoadedFromStorageReady(FULL) + onLoadedFromStorageReady（解除依赖邻居阻塞）
     *   - 缺失：noteStorageResolved(false) → _scheduleGeneration → schedule（sourceState=StorageMissing）
     *     → scheduleEmptyLoad 创建空 Primer（对齐 getEmptyChunk）→ onChunkGenComplete → notifyWaitingNeighbours
     *
     * 调用者持有调度区域锁（schedule/checkNeighbour 路径）。本方法只触及 SCLM.m_mutex 与
     * m_pendingLoadTasksMutex（_resolveChunkSourceSync），不重入调度锁：Unknown→ResolvingStorage 的决策
     * 为 shouldResolveStorage=true（非 shouldScheduleGeneration），_advanceChunkState 仅调用
     * _resolveChunkSourceSync（异步投递后立即返回），不触发 _scheduleGeneration 的调度锁获取。
     *
     * 幂等：submitRequest 对 ResolvingStorage 返回 no-op（不重复触发解析）。并发调用（checkNeighbour
     * 与 _onTicketLevelChanged 同时驱动同一 Unknown holder）安全：首个 submitRequest 把 sourceState 推进
     * 到 ResolvingStorage，后续调用见到非 Unknown 走 no-op。
     *
     * @param lifecycleManager 目标 holder（调用前 sourceState==Unknown；调用后推进到 ResolvingStorage）
     * @param targetStatus 当前请求目标状态（用于 submitRequest 收敛 requestedGenStatus，解析完成后据此推进）
     */
    void _resolveStorageForScheduling(
        mc::world::chunk::SingleChunkLifecycleManager& lifecycleManager, const ChunkStatus& targetStatus);

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
     * @brief 将 SCLM 请求优先级（i32，越小越优先，INT_MAX=未设置/已取消）映射到线程池任务优先级
     *
     * _computeSchedulePriority 返回 normalizedLevel*1024 + statusPenalty*32 + spatialPenalty(0..255)，
     * 故 sclmPriority/1024 可无损恢复 normalizedLevel（票据级别，越小越靠近玩家）。
     * ≤ MAX_LOADED_LEVEL（玩家视距内 FULL 区块）→ High；附近过渡 → Normal；远处 → Low。
     * Critical/Background 保留给系统级任务，不在此使用。
     */
    [[nodiscard]] static util::TaskPriority mapSclmPriorityToTaskPriority(i32 sclmPriority);

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
     * + _postProcessChunk。primer 仍持有同一份 ChunkData 供邻居引用（FULL 完成后 primer 不释放，
     * 邻居生成任务仍可引用其 ChunkData，直到 holder 卸载）。
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
     * 调用方负责通过 ChunkData::isPostProcessingDone() 保证至多执行一次；
     * 执行完毕后置 ChunkData::setPostProcessingDone(true)。
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
     * @brief 触发区块卸载对外的通知（实体保存+移除、卸载发送、callback）
     *
     * 仅在确定移除区块时调用，确保每次卸载仅通知一次（重试路径不触发）。
     */
    void _notifyChunkUnload(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 检查并卸载无需求区块
     */
    void _checkChunkUnloading();

    /**
     * @brief 异步卸载保存完成后，主线程完成卸载收尾（stage3，对齐 Moonrise unloadStage3）
     *
     * 在 _drainPendingUnloadFinishes 中出队调用（非脏路径由 unloadChunkSync 直接调用）。处理：
     * - 复检区块是否被重新请求（shouldLoad=true）：是则中止卸载，保留区块与 holder
     *   （区块数据已保存，若再次变脏会重新保存）。
     * - 否则完成卸载：持调度锁 cancelGeneration + isSafeToUnload 复检 → 通过后
     *   _notifyChunkUnload（实体保存+移除/卸载发送/callback，仅触发一次）→
     *   移除 holder 与 m_chunks 条目、清理后处理去重标记。
     * - isSafeToUnload 为 false（保存期间邻居开始引用本 holder）：保留条目，下一 tick 重试。
     *   重试路径不触发卸载通知，避免重复通知客户端/移除实体。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 维度
     * @param lifecycleHolder 异步发起时持有的 SCLM 共享指针
     * @return true 已完成卸载（或中止卸载），条目可丢弃；false 需下一 tick 重试
     */
    bool _finalizeUnloadAfterSave(ChunkCoord x,
        ChunkCoord z,
        mc::DimensionId dimension,
        std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> lifecycleHolder);

    /**
     * @brief 出队并执行异步卸载保存完成回调（仅主线程调用）
     *
     * 在 tick() 中调用，把 ServerIO 线程入队的 m_pendingUnloadFinishes 逐个交给
     * _finalizeUnloadAfterSave。未完成（isSafeToUnload=false）的条目保留至下一 tick 重试。
     */
    void _drainPendingUnloadFinishes();

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
     * worker 线程在 FULL 完成/存档加载时不能直接调用主线程独占的世界状态
     * （onChunkLoaded / m_chunkLoadedCallback / spawnEntitiesFromChunkGeneration /
     * _postProcessChunk 触及 ServerTickList、EntityManager、setBlockState、POI、光照等，
     * 均非线程安全）。入队后由主线程 tick() 的 _drainPendingPostProcess 出队执行。
     *
     * 去重：_drainPendingPostProcess 通过 m_postProcessedChunks 保证同一区块的
     * onChunkLoaded/callback/spawn/postProcess 至多执行一次，重复入队条目会被丢弃。
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
     *
     * 通过 m_postProcessedChunks 去重：同一区块的上述四项至多执行一次。
     * 重复入队（worker 存档加载入队 + 主线程存档解析直接调用竞态，或同一区块多次入队）
     * 的条目被丢弃，避免实体重复生成、区块重复发送、光照重复初始化。
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
     * @brief 主线程后处理队列
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
     * @brief 已完成主线程后处理的区块 key 集合
     *
     * 防止同一区块因重复入队或 worker/主线程路径竞态而多次执行后处理（实体重复生成、
     * 区块重复发送、光照重复初始化）。_drainPendingPostProcess 与 _resolveChunkSourceSync
     * 在执行 onChunkLoaded/callback/spawn/postProcess 前查插此集合；命中则跳过。
     * unloadChunkSync 移除对应 key，使重新加载可重新执行后处理。
     *
     * 线程安全：由 m_pendingPostProcessMutex 保护（_drainPendingPostProcess / _resolveChunkSourceSync /
     * unloadChunkSync / shutdown 均持锁访问）。
     */
    std::unordered_set<u64> m_postProcessedChunks;

    /**
     * @brief 异步存档加载完成队列（worker→主线程回传）
     *
     * _resolveChunkSourceSync 在主线程发起异步加载（loadChunkAsyncCallback），
     * 完成回调在 ServerCompute 线程执行（存储层 _assemble 阶段），把结果入队到此队列。
     * 主线程 tick() 的 _drainPendingLoadCompletes 出队交给 _onChunkLoadComplete 处理。
     *
     * 线程安全：由 m_pendingLoadCompletesMutex 保护（ServerCompute 线程入队、主线程出队）。
     */
    struct PendingLoadComplete {
        ChunkCoord x = 0;
        ChunkCoord z = 0;
        mc::DimensionId dimension{};
        // Result 不可默认构造（Result() = delete），用 Error 占位以允许 PendingLoadComplete 默认构造。
        mc::Result<std::optional<mc::ChunkData>> result = mc::Error(mc::ErrorCode::Unknown, "");
        std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> lifecycleHolder;
    };
    std::mutex m_pendingLoadCompletesMutex;
    std::vector<PendingLoadComplete> m_pendingLoadCompletes;

    /**
     * @brief 进行中的异步存档加载追踪表（含 SCM 层去重合并）
     *
     * key = posToKey(x, z)，value = PendingLoadEntry（ownerLifecycle + attachedWaiters）。
     * 用途：
     * - _resolveChunkSourceSync 检查 key 是否已存在在途加载：若存在，附加当前 SCLM 为等待者
     *   （不重新发起 loadChunkAsyncCallback），实现 SCM 层去重（对齐 Moonrise chunkTasks 合并）。
     * - _onChunkLoadComplete 通过 ownerLifecycle 实例一致性校验防止 SCLM 被 unload 重建后的误用，
     *   完成后遍历 attachedWaiters 推进其状态机。
     * - unloadChunkSync 从 attachedWaiters 移除已卸载的 SCLM（owner 条目由 _onChunkLoadComplete 移除）。
     *
     * 线程安全：由 m_pendingLoadTasksMutex 保护（_resolveChunkSourceSync 主线程写、
     * _onChunkLoadComplete 主线程读写、unloadChunkSync 主线程读写）。
     * 注：_onChunkLoadComplete 与 _drainPendingLoadCompletes 均在主线程 tick() 串行调用，
     * 故追踪表的读写实际上单线程；abortSignal 由 SCLM 持有，unloadChunkSync→cancelActiveWork 置位。
     */
    std::mutex m_pendingLoadTasksMutex;
    std::unordered_map<u64, PendingLoadEntry> m_pendingLoadTasks;

    /**
     * @brief 异步卸载保存完成队列（ServerIO→主线程回传，stage2→stage3 衔接）
     *
     * unloadChunkSync（stage1）提交 saveChunkAsyncCallback 后，ServerIO 线程完成保存时把
     * （x, z, dimension, lifecycleHolder）入队此队列。主线程 tick() 的 _drainPendingUnloadFinishes
     * 出队交给 _finalizeUnloadAfterSave（stage3）完成卸载收尾。
     *
     * 线程安全：由 m_pendingUnloadFinishesMutex 保护（ServerIO 线程入队、主线程出队）。
     */
    struct PendingUnloadFinish {
        ChunkCoord x = 0;
        ChunkCoord z = 0;
        mc::DimensionId dimension{};
        std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> lifecycleHolder;
    };
    std::mutex m_pendingUnloadFinishesMutex;
    std::vector<PendingUnloadFinish> m_pendingUnloadFinishes;

    /**
     * @brief 进行中异步卸载保存的区块 key 集合
     *
     * unloadChunkSync（stage1）置位，_finalizeUnloadAfterSave（stage3）完成或中止后清除。
     * _checkChunkUnloading 跳过此集合中的区块，避免重复发起卸载保存。
     *
     * 线程安全：由 m_pendingUnloadFinishesMutex 保护（与 m_pendingUnloadFinishes 共用锁，
     * _checkChunkUnloading / unloadChunkSync / _drainPendingUnloadFinishes 均持锁访问）。
     */
    std::unordered_set<u64> m_unloadSaveInProgress;

    /**
     * @brief 区块生成调度核心
     *
     * 持有 ReentrantAreaLock 保证 schedule/checkNeighbour/onChunkGenComplete 的原子性。
     * nullptr 表示无 worker 池（独立/测试模式），ChunkTaskScheduler 内部在线执行任务。
     */
    std::unique_ptr<ChunkTaskScheduler> m_taskScheduler;

    mc::world::chunk::ChunkLoadTicketManager m_ticketManager;
    sync::ChunkSendManager* m_chunkSendManager = nullptr;
    util::ServerWorkerPool* m_workerPool = nullptr;

    /// 关闭标志。shutdown() 置位后，异步存档加载完成回调（ServerCompute 线程）不再入队
    /// m_pendingLoadCompletes，避免 ServerChunkManager 析构后回调访问悬空 this。
    ///
    /// 生命周期保证：生产环境 StandaloneServer/IntegratedServer 的 stop() 先调 stopCore()
    /// （m_computationWorkerPool.shutdown() + m_ioWorkerPool.shutdown() 均 join 线程，排空在途任务），
    /// 再调 shutdownManagers()（析构 ServerChunkManager）。因此 completion 回调（跑在 ServerCompute）
    /// 在析构前已由 waitForCompletion 排空，不会悬空。
    /// 此标志为 destructor 未经 stopCore 的异常/测试路径的防御性保护：shutdown() 在析构前置位，
    /// 回调检测到后立即 return 不触及 this 成员。
    std::atomic<bool> m_shuttingDown{false};

    u64 m_currentTick = 0;
    u64 m_lastUnloadCheckTick = 0;

    static constexpr u32 UNLOAD_CHECK_INTERVAL_TICKS = 20;

    /// 每 tick 卸载预算上限（对齐 Moonrise ChunkHolderManager.processUnloads）
    /// 防止单 tick 卸载过多区块造成卡顿
    static constexpr i32 MAX_UNLOADS_PER_TICK = 200;

    /// 主线程后处理队列软上限。
    ///
    /// 该队列每 tick 由 _drainPendingPostProcess 完全排空（强上界保证），且每个区块每生命周期最多
    /// 入队一次（m_postProcessedChunks 去重）。正常情况下队列长度 ≤ worker 池并发完成的区块数，
    /// 远低于此上限。当主 tick 卡顿导致 worker 在两次排空之间堆积时，超过阈值仅记录警告日志，
    /// 不拒绝入队（拒绝会丢失已生成区块的实体/后处理，导致状态机停滞），排空语义不变。
    static constexpr size_t PENDING_POST_PROCESS_WARN_THRESHOLD = 256;

    /// 异步存档加载完成队列软上限。
    ///
    /// 每项持有完整 ChunkData（~600KB）与 lifecycleHolder，每 tick 由 _drainPendingLoadCompletes
    /// 完全排空。每区块最多一个在途加载（m_pendingLoadTasks 去重），正常积压受限于视距内待加载
    /// 区块数。超过阈值记录警告（暴露主 tick 跟不上加载速率的病态积压），不拒绝入队，排空语义不变。
    static constexpr size_t PENDING_LOAD_COMPLETES_WARN_THRESHOLD = 256;

    /// 加载区块软上限（0 = 不限制）
    /// 当 m_chunks + m_lifecycleManagers 总数超过此值时，按最远票级强制卸载
    i32 m_maxLoadedChunks = 0;

    friend class ServerChunkManagerPostProcessTest;
};

} // namespace mc::server
