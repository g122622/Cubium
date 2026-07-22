/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction to do, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING THE LIABILITY,
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/concurrent/ReentrantAreaLock.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "server/world/StaticChunkCache2D.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace mc::world::chunk {
class SingleChunkLifecycleManager;
class ChunkStep;
class ChunkPrimer;
} // namespace mc::world::chunk

namespace mc::server {

class ServerChunkManager;
class ServerWorld;
class ChunkProgressionTask;

/**
 * @brief 区块生成调度核心（对齐 Moonrise ChunkTaskScheduler）
 *
 * ChunkTaskScheduler 负责把区块从当前状态一步步推进到目标状态：
 *   - `schedule(x, z, targetStatus, holder)`：一次只推进一步（currentGenStatus.next()）。
 *     检查邻居是否就绪；未就绪则建立双向依赖图并挂起；全部就绪则构建 StaticChunkCache2D
 *     并提交 ChunkProgressionTask 到执行器。
 *   - `checkNeighbour(x, z, requiredStatus, center)`：验证邻居是否达到 requiredStatus。
 *     未达到则建立依赖、递归 schedule 邻居，返回 true（未就绪）。
 *   - `onChunkGenComplete(holder, completedStatus)`：推进 holder.currentGenStatus（Cubium ChunkPrimer
 *     累积式，primer 是同一对象无需重存）、释放邻居引用计数、通知 waitingNeighbours 重新调度。
 *
 * 区域锁（ReentrantAreaLock）保证 schedule/checkNeighbour/onChunkGenComplete 的原子性：
 *   - schedule/checkNeighbour 持有 `[x±accessRadius, z±accessRadius]` 的锁
 *   - onChunkGenComplete 持有 `2 * maxAccessRadius` 的锁（覆盖邻居的邻居）
 *
 * 执行器选择：
 *   - parallelCapable 状态（blockStateWriteRadius <= 0 且无邻居写）：提交到并行池
 *   - 其他状态：提交到区域互斥池（带 writeRadius）
 */
class ChunkTaskScheduler {
public:
    /**
     * @brief 构造调度器
     *
     * @param manager 所属 ServerChunkManager（用于执行步骤、存档、holder 管理）
     * @param world 所属 ServerWorld（用于 WorldGenRegion 种子等信息）
     * @param parallelGenExecutor 并行执行器（无区域互斥，EMPTY~INITIALIZE_LIGHT 等）
     * @param radiusAwareExecutor 区域互斥执行器（FEATURES/LIGHT/SPAWN/FULL 等写方块状态）
     *
     * 两个执行器可为同一个 UniversalWorkerPool（区域互斥由 submit 的 writeRadius 重载保证）。
     * 也可为 nullptr（同步降级：在线执行任务）。
     */
    ChunkTaskScheduler(ServerChunkManager& manager,
        ServerWorld* world,
        util::UniversalWorkerPool* parallelGenExecutor,
        util::UniversalWorkerPool* radiusAwareExecutor);

    /**
     * @brief 主调度入口：把 (x, z) 推进到 targetStatus（一次一步）
     *
     * 调用者必须持有覆盖 `[x±accessRadius(targetStatus), z±accessRadius(targetStatus)]` 的区域锁。
     *
     * @param x 区块 X
     * @param z 区块 Z
     * @param targetStatus 目标状态（可能多步才能达到，本函数只推进一步）
     * @param holder 目标区块的生命周期管理器
     * @return 创建的 ChunkProgressionTask（nullptr 表示因邻居未就绪而挂起，或无需推进）
     */
    ChunkProgressionTask* schedule(ChunkCoord x,
        ChunkCoord z,
        const ChunkStatus& targetStatus,
        mc::world::chunk::SingleChunkLifecycleManager& holder);

    /**
     * @brief 邻居检查：验证 (x, z) 是否达到 requiredStatus
     *
     * 已达到 → return false（就绪）；未达到 → 建立双向依赖并递归 schedule 邻居，return true（未就绪）。
     * 调用者必须持有覆盖 (x, z) 的区域锁。
     *
     * @return true=未就绪（需等待），false=已就绪
     */
    bool checkNeighbour(ChunkCoord x,
        ChunkCoord z,
        const ChunkStatus& requiredStatus,
        mc::world::chunk::SingleChunkLifecycleManager& center);

    /**
     * @brief 任务完成回调：由 ChunkProgressionTask 完成时调用
     *
     * 持有 `2 * maxAccessRadius` 的锁，推进 holder.currentGenStatus（Cubium ChunkPrimer 累积式，
     * primer 是同一对象，无需重存），通知 waitingNeighbours 重新调度。
     *
     * @param holder 完成任务的生命周期管理器
     * @param completedStatus 本次完成的状态
     */
    void onChunkGenComplete(mc::world::chunk::SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus);

    /**
     * @brief 任务失败回调（对齐 Moonrise：失败即不可恢复）
     *
     * 标记 holder 失败，通知等待者失败。
     */
    void onChunkGenFailed(mc::world::chunk::SingleChunkLifecycleManager& holder);

    /**
     * @brief 任务身份感知的失败回调
     *
     * 由 submitTask 回调调用。仅当 holder.m_generationTask 仍指向 task 时才执行失败处理
     * （markFailed + clearGenerationTask + _failWaiters），避免以下竞态：
     *   - 任务 A 被取消（abortSignal=true），execute 返回 false
     *   - cancelActiveWork 清空 m_generationTask 后，新 submitRequest 调度任务 B 并 setGenerationTask(B)
     *   - onCancel→cancelGeneration(holder, A) 是 no-op（generationTask()==B ≠ A）
     *   - 回调若用 hasGenerationTask() 判定（B 非空）会误触发 onChunkGenFailed，markFailed 一个仍有
     *     活跃任务 B 的 holder 并 clearGenerationTask 清掉 B → 依赖图永久阻塞 → 测试挂起。
     * 用 generationTask()==task 比对：B ≠ A → no-op，B 继续正常运行。
     *
     * 持调度锁做身份比对，消除 execute 返回与回调之间的 TOCTOU 窗口。
     *
     * @param holder 任务所属的生命周期管理器
     * @param task 调用回调的任务（rawTask），用于身份比对
     */
    void onChunkGenFailed(
        mc::world::chunk::SingleChunkLifecycleManager& holder, mc::server::ChunkProgressionTask* task);

    /**
     * @brief 取消 holder 的生成任务并清理依赖图（对齐 Moonrise cancelGenTask + onChunkGenComplete(null)）
     *
     * 由 unloadChunkSync（holder 卸载）和 onCancel（任务被 abortSignal 取消）调用。
     * 与 onChunkGenFailed 的关键区别：不 markFailed（取消不是失败，holder 可被重新创建并重新生成）。
     *
     * 清理步骤（持调度锁）：
     *   1. clearGenerationTask（abortSignal 已由 cancelActiveWork 设置，运行中的任务检测到后 onCancel 调用本方法）
     *   2. releaseNeighbourRefCounts：补偿释放任务未执行的邻居引用计数（任务取消时 onChunkGenComplete 未运行）
     *   3. 出向依赖：遍历 m_blockingNeighbours（本 holder 等待的邻居），从每个邻居的 m_waitingNeighbours 移除本 holder
     *   4. 入向依赖：遍历 m_waitingNeighbours（等本 holder 的邻居），从每个邻居的 m_blockingNeighbours 移除本 holder，
     *      若邻居不再阻塞且有请求目标，重新调度邻居（邻居的 schedule→checkNeighbour 会通过 getOrCreateHolder
     *      重建本 holder——旧的已从 m_lifecycleManagers 移除）
     *   5. 清空本 holder 的 m_blockingNeighbours / m_waitingNeighbours
     *   6. _failWaiters(takeAllWaiters)：通知请求等待者失败
     *
     * 清理后 isSafeToUnload 返回 true（依赖图空、无生成任务），holder 可安全卸载。
     * 被解除阻塞的邻居通过 rescheduleChunk 重新推进（若仍有请求目标）。
     */
    void cancelGeneration(mc::world::chunk::SingleChunkLifecycleManager& holder);

    /**
     * @brief 取消指定任务的生成并清理依赖图（任务身份感知版本）
     *
     * 由 ChunkProgressionTask::onCancel 调用。仅当 holder.m_generationTask 仍指向 task 时才执行清理，
     * 避免旧任务的 onCancel 误清新任务的依赖图/状态（cancelActiveWork 清空 m_generationTask 后，
     * submitRequest 可能已调度新任务 B 并 setGenerationTask(B)，旧任务 A 的 onCancel 不应干扰 B）。
     *
     * 若 m_generationTask != task（已被清除或被新任务取代），本方法为 no-op：
     *   - 旧任务 A 的邻居引用计数已在 A 真正完成（onChunkGenComplete）或 holder 卸载（cancelGeneration(holder)）时释放
     *   - 旧任务 A 的依赖图条目（m_blockingNeighbours/m_waitingNeighbours）同理
     *   - 旧任务 A 的等待者已在 cancelActiveWork/_failWaiters 时通知
     *
     * @param holder 任务所属的生命周期管理器
     * @param task 调用 onCancel 的任务（this），用于身份比对
     */
    void cancelGeneration(
        mc::world::chunk::SingleChunkLifecycleManager& holder, mc::server::ChunkProgressionTask* task);

    /**
     * @brief 存档命中完成通知：解除等待该 holder 的邻居的阻塞并重新调度它们
     *
     * 由 ServerChunkManager::_onChunkLoadComplete（存档命中分支）在 holder 经
     * markLoadedFromStorageReady(FULL) 达到 Ready 后调用。存档命中路径不走生成任务，
     * 故不会触发 onChunkGenComplete→notifyWaitingNeighbours；若不显式通知，经 checkNeighbour
     * 注册到本 holder m_waitingNeighbours 的依赖邻居（如中心区块生成时 checkNeighbour 发现
     * 本 holder 在 ResolvingStorage，schedule 返回 nullptr 挂起等待）将永久阻塞——本 holder
     * 已就绪但邻居永不被唤醒，依赖图泄漏，表现为 worker 空闲（pending=0/running=0）但请求
     * future 永不完成（死锁）。
     *
     * 与 onChunkGenComplete 的区别：本 holder 无生成任务（不 clearGenerationTask）、存档命中
     * 未增加邻居引用计数（不 releaseNeighbourRefCounts）、holder 已达 FULL（不自推进、不
     * _publishGeneratedChunk——存档路径已存入 m_chunks 并 _completeReadyWaiters）。仅复用
     * notifyWaitingNeighbours 解除依赖邻居阻塞 + 释放锁后 rescheduleChunk 重新推进它们。
     *
     * @param holder 存档命中刚达到 Ready/FULL 的生命周期管理器
     */
    void onLoadedFromStorageReady(mc::world::chunk::SingleChunkLifecycleManager& holder);

    /**
     * @brief 获取或创建 holder（委托给 ServerChunkManager 的 m_lifecycleManagers）
     */
    [[nodiscard]] mc::world::chunk::SingleChunkLifecycleManager& getOrCreateHolder(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 查询现有 holder
     */
    [[nodiscard]] mc::world::chunk::SingleChunkLifecycleManager* findHolder(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 区域锁访问
     */
    [[nodiscard]] util::ReentrantAreaLock& schedulingLockArea() { return m_schedulingLockArea; }

    /**
     * @brief 标记调度器正在关闭
     *
     * 关闭期间 cancelGeneration 不重新调度被解除阻塞的邻居（避免无限重调度循环），
     * 因为关闭时所有 holder 的 abortSignal 已失效，重调度的任务无法被取消。
     * ServerChunkManager::shutdown 在 cancelActiveWork 之前调用此方法。
     */
    void setShuttingDown() { m_shuttingDown.store(true, std::memory_order::release); }

    [[nodiscard]] bool isShuttingDown() const { return m_shuttingDown.load(std::memory_order::acquire); }

    /**
     * @brief 访问半径（= step.accumulatedRadius()），用于区域锁范围
     */
    [[nodiscard]] static i32 getAccessRadius(const mc::world::chunk::ChunkStatus& status);

    /**
     * @brief 最大访问半径（FULL 的 accumulatedRadius，当前为 11）
     */
    [[nodiscard]] static i32 getMaxAccessRadius();

private:
    /**
     * @brief 选择执行器：parallelCapable 状态走并行池，其他走区域互斥池
     *
     * 返回 nullptr 时调用方应在线执行任务（无 worker 池的同步降级模式）。
     */
    [[nodiscard]] util::UniversalWorkerPool* selectExecutor(const mc::world::chunk::ChunkStatus& status);

    /**
     * @brief 提交任务到执行器（无 worker 池时在线执行）
     *
     * @param task 任务（所有权转移）
     * @param status 目标状态（用于选择执行器）
     * @param x 区块 X
     * @param z 区块 Z
     * @param writeRadius 区域互斥写入半径
     * @param abortSignal 取消令牌
     * @param holder 中心区块生命周期管理器的共享所有权。回调与 onCancel 持有此 shared_ptr，
     *                保证任务执行完成后的回调期间 holder 不被卸载销毁。
     */
    void submitTask(std::unique_ptr<ChunkProgressionTask> task,
        const mc::world::chunk::ChunkStatus& status,
        ChunkCoord x,
        ChunkCoord z,
        i32 writeRadius,
        std::shared_ptr<std::atomic<bool>> abortSignal,
        std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> holder);

    /**
     * @brief 推进单步状态（schedule 的核心实现）
     *
     * 检查邻居就绪性，构建邻居缓存，创建并提交 ChunkProgressionTask。
     * 调用者必须持有覆盖 [x±accessRadius(toStatus), z±accessRadius(toStatus)] 的区域锁。
     *
     * @param x 区块 X
     * @param z 区块 Z
     * @param holder 目标区块的生命周期管理器
     * @param toStatus 要推进到的目标状态（= currentGenStatus.next()）
     * @return 创建的 ChunkProgressionTask（nullptr 表示因邻居未就绪而挂起）
     */
    ChunkProgressionTask* scheduleStatusStep(
        ChunkCoord x, ChunkCoord z, mc::world::chunk::SingleChunkLifecycleManager& holder, const ChunkStatus& toStatus);

    /**
     * @brief 构建邻居 holder 共享所有权缓存
     *
     * 遍历 step.neighbourReadRadius() 范围，取各邻居 holder 的 shared_ptr（对齐 Moonrise
     * StaticCache2D<GenerationChunkHolder>，存存活引用非快照拷贝），构造 StaticChunkCache2D。
     * 调用前必须已通过 checkNeighbour 确认所有邻居就绪。
     *
     * 邻居 holder 的 shared_ptr 由 worker 线程在 executeStatusStep 期间持有，保证邻居 holder 及其
     * m_currentChunk（ChunkPrimer）不被主线程 unloadChunkSync 销毁（消除裸 ChunkPrimer* 的
     * use-after-free 竞态）。仅靠 m_neighboursUsingThisChunk 引用计数不足以防止：worker 执行
     * executeStatusStep 时不持调度锁，主线程 cancelGeneration→removeNeighbourUsingChunk→
     * isSafeToUnload→erase 可与 worker 解引用裸指针并发。
     *
     * 对齐 Moonrise：邻居是可变 ChunkPrimer（与中心同类型），WorldGenRegion 通过它读写邻居
     * （FEATURES writeRadius=1 写邻居、STRUCTURE_REFERENCES 读邻居结构起点）。并发安全由
     * ReentrantAreaLock 覆盖 [center±writeRadius] 保证（等价 Moonrise AreaDependentQueue）。
     */
    [[nodiscard]] StaticChunkCache2D<std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager>>
    buildNeighbourCache(ChunkCoord x, ChunkCoord z, const mc::world::chunk::ChunkStep& step);

    /**
     * @brief 待重调度的区块（onChunkGenComplete/cancelGeneration 在锁内收集，释放锁后 rescheduleChunk）
     *
     * 对齐 Moonrise needsScheduling：持锁期间只做依赖图变更 + 收集待重调度列表，
     * 释放区域锁后再 rescheduleChunk，避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁。
     * target 为 requestedGenStatus() 的指针（ChunkStatus 静态单例，释放锁后仍有效）。
     */
    struct PendingReschedule {
        ChunkCoord x;
        ChunkCoord z;
        const mc::world::chunk::ChunkStatus* target;
    };

    /**
     * @brief 在 onChunkGenComplete 后通知等待该区块的邻居重新调度
     *
     * 持锁期间仅做依赖图变更（removeBlockingNeighbour/clearSatisfiedWaitingNeighbours），
     * 对就绪的邻居不立即 rescheduleChunk，而是收集到 pending 列表（对齐 Moonrise needsScheduling）。
     * 调用方（onChunkGenComplete）在释放区域锁后统一 rescheduleChunk，避免持锁期间嵌套获取邻居的
     * 2*maxAccessRadius 锁（ReentrantAreaLock::lock 竞争的主要放大器）。
     *
     * @param holder 完成任务的生命周期管理器
     * @param completedStatus 本次完成的状态
     * @param pending 输出：就绪邻居的待重调度列表（{x, z, target}），调用方在释放锁后遍历 rescheduleChunk。
     *            target 为 neighbour->requestedGenStatus() 的指针（静态单例，释放锁后仍有效）。
     */
    void notifyWaitingNeighbours(mc::world::chunk::SingleChunkLifecycleManager& holder,
        const mc::world::chunk::ChunkStatus& completedStatus,
        std::vector<PendingReschedule>& pending);

    /**
     * @brief 释放邻居引用计数
     *
     * 任务完成后，遍历该步的邻居读取半径范围，对每个使用的邻居调用 removeNeighbourUsingChunk。
     * 与 scheduleStatusStep 中的 addNeighbourUsingChunk 一一对应。
     *
     * @param holder 完成任务的生命周期管理器
     * @param completedStatus 本次完成的状态
     */
    void releaseNeighbourRefCounts(
        mc::world::chunk::SingleChunkLifecycleManager& holder, const mc::world::chunk::ChunkStatus& completedStatus);

    /**
     * @brief 创建 EMPTY 加载任务（从存档加载或新建空 Primer）
     *
     * EMPTY 推进不调用 IChunkGenerator，只解析存档来源。完成后调用 onChunkGenComplete(EMPTY)。
     */
    ChunkProgressionTask* scheduleEmptyLoad(
        ChunkCoord x, ChunkCoord z, mc::world::chunk::SingleChunkLifecycleManager& holder);

    /**
     * @brief 重调度一个区块（自推进或邻居完成通知触发）
     *
     * 在异步（worker 池运行）模式下立即调用 schedule。
     * 在同步（无 worker 池、在线执行）模式下，将重调度请求入队，由最外层 submitTask
     * 在当前任务执行完毕后统一 drain。这避免 onChunkGenComplete 的重调度在
     * scheduleStatusStep 的邻居扫描期间重入（重入会导致中心区块被过早重调度，
     * 产生指数级重复扫描）。
     *
     * @param x 区块 X
     * @param z 区块 Z
     * @param target 目标状态（holder.requestedGenStatus()）
     */
    void rescheduleChunk(ChunkCoord x, ChunkCoord z, const mc::world::chunk::ChunkStatus& target);

    /**
     * @brief 同步执行上下文：在线执行模式下延迟重调度
     *
     * submitTask 的在线执行路径会把任务执行（含 onChunkGenComplete 的重调度）包裹在
     * 同步上下文中。rescheduleChunk 检测到上下文激活时入队而非立即调度，
     * 最外层（depth 归零）drain 队列。这把递归的 schedule 压平为迭代，避免
     * scheduleStatusStep 邻居扫描期间的重入。
     */
    struct SyncSchedulingContext {
        /// 当前线程的同步调度深度（>0 表示处于在线执行模式）
        int depth = 0;
        /// 延迟重调度队列（复用 PendingReschedule）
        std::vector<PendingReschedule> pending;
    };

    /**
     * @brief 获取当前线程的同步调度上下文（thread_local）
     */
    [[nodiscard]] static SyncSchedulingContext& currentSyncContext();

    /**
     * @brief 在同步上下文中执行一次任务，并在最外层 drain 延迟重调度队列
     *
     * 由 submitTask 的在线执行路径调用。递增 depth，执行 callable（其中可能触发
     * onChunkGenComplete → rescheduleChunk 入队），depth 归零后 drain 队列：
     * 对每个 PendingReschedule 持锁调用 schedule，drain 期间产生的新重调度继续入队，
     * 直到队列为空。
     */
    void runInlineAndDrain(std::function<void()>&& callable);

    ServerChunkManager& m_manager;
    ServerWorld* m_world;
    util::UniversalWorkerPool* m_parallelGenExecutor;
    util::UniversalWorkerPool* m_radiusAwareExecutor;
    util::ReentrantAreaLock m_schedulingLockArea;
    std::atomic<bool> m_shuttingDown{false};
};

} // namespace mc::server
