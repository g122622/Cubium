/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING THE ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/profiler/MemoryTracking.hpp"
#include "common/world/chunk/base/ChunkId.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/chunk/load/ChunkLoadTicket.hpp"
#include <atomic>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::world::chunk {
class ChunkPrimer;
class ChunkData;
} // namespace mc::world::chunk

namespace mc::server {
class ChunkProgressionTask;
} // namespace mc::server

namespace mc::world::chunk {

/**
 * @brief 单区块生命周期管理器（对齐 Moonrise NewChunkHolder）
 *
 * 重构后的 SingleChunkLifecycleManager 采用 Moonrise 的可变区块 + 双向邻居依赖图模型：
 * - `m_currentChunk`：当前可变 ChunkPrimer（累积所有已完成状态的数据，对齐 NewChunkHolder.currentChunk）
 * - `m_currentGenStatus`：当前已达到的最高生成状态（对齐 NewChunkHolder.currentGenStatus）
 * - `m_blockingNeighbours` / `m_waitingNeighbours`：双向邻居依赖图
 * - `m_generationTask`：当前进行中的 ChunkProgressionTask（一次只推进一步状态）
 *
 * 与 Moonrise 的差异：Cubium 的 ChunkPrimer 累积式（同一对象贯穿所有状态，_executeStepTask 修改同一 primer），
 * 而 Moonrise 每状态独立 ChunkAccess（chunkCompletions[] 数组）。因此 Cubium 只需 m_currentChunk 单一指针，
 * 无需每状态数组。getChunkIfPresentUnchecked(status) 检查 m_currentGenStatus.isAtLeast(status) 后返回 m_currentChunk。
 *
 * 职责划分：
 * - 本类：持有状态（currentChunk/依赖图/请求聚合/等待者）、存档来源解析状态、票据与玩家追踪
 * - `ChunkTaskScheduler`：调度决策（schedule/checkNeighbour/onChunkGenComplete）、区域锁、执行器选择
 * - `ServerChunkManager`：请求入口、存储、卸载、tick、票据驱动
 *
 * 生成调度（何时推进、推进一步）由 `ChunkTaskScheduler` 负责，本类只保存状态并回答
 * "当前状态是什么"、"哪个区块可用"、"谁在等我"。生成完成后由 `ChunkTaskScheduler`
 * 调用 `onChunkGenComplete` 推进 currentGenStatus 并通知等待者。
 */
class SingleChunkLifecycleManager {
public:
    /**
     * @brief 区块来源解析状态
     *
     * 描述"区块数据从哪里来"：存档恢复还是生成。
     * 与生成调度解耦：SourceState 决定来源链路，生成调度由 ChunkTaskScheduler 驱动。
     */
    enum class SourceState {
        Unknown,           ///< 尚未判定该区块是否存在于存档中
        ResolvingStorage,  ///< 正在执行一次性的存档来源解析
        StorageMissing,    ///< 已确认存档中不存在该区块，后续走生成链路
        LoadedFromStorage, ///< 已从存档恢复出区块数据，但尚未发布到 Ready
        Ready              ///< 区块已经在内存中可用，可直接满足请求
    };

    /**
     * @brief 挂起中的等待者
     *
     * 同一个区块的多个同步/异步请求最终都会收敛到这里，
     * 生命周期管理器负责在区块就绪或失败时统一完成它们。
     */
    struct Waiter {
        std::function<void(bool, ChunkData*)> callback;
        std::shared_ptr<std::promise<ChunkData*>> promise;
    };

    /**
     * @brief 状态机推进后给调度器的动作决策
     *
     * 生命周期管理器本身只保存状态，不直接执行 IO 或提交 worker。
     * 每次状态变化后，由 ServerChunkManager 读取该结构并执行对应副作用。
     *
     * 重构后生成调度委托给 ChunkTaskScheduler，EnqueueDecision 只保留来源解析与等待者完成两个动作。
     */
    struct EnqueueDecision {
        bool shouldResolveStorage = false;              ///< 是否需要执行一次存档来源解析
        bool shouldWakeReadyWaiters = false;            ///< 是否应立即完成所有等待者
        bool shouldScheduleGeneration = false;          ///< 是否应调用 ChunkTaskScheduler 推进生成
        const ChunkStatus* targetStatus = nullptr;      ///< 当前请求目标状态
        const ChunkStatus* completedStatus = nullptr;   ///< 本次刚完成到的阶段（可为 nullptr）
        std::shared_ptr<std::atomic<bool>> abortSignal; ///< 当前请求对应的取消令牌
    };

    /**
     * @brief 创建单区块生命周期管理器
     */
    SingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z);

    ~SingleChunkLifecycleManager() = default;

    SingleChunkLifecycleManager(const SingleChunkLifecycleManager&) = delete;
    SingleChunkLifecycleManager& operator=(const SingleChunkLifecycleManager&) = delete;
    SingleChunkLifecycleManager(SingleChunkLifecycleManager&&) noexcept = delete;
    SingleChunkLifecycleManager& operator=(SingleChunkLifecycleManager&&) noexcept = delete;

    // === 基础坐标访问 ===

    [[nodiscard]] ChunkCoord x() const { return m_x; }
    [[nodiscard]] ChunkCoord z() const { return m_z; }
    [[nodiscard]] ChunkPos pos() const { return ChunkPos(m_x, m_z); }
    [[nodiscard]] u64 id() const { return ChunkId(m_x, m_z, 0).toId(); }

    // === 生成状态（对齐 NewChunkHolder currentGenStatus / currentChunk） ===

    /**
     * @brief 获取当前已达到的最高生成状态
     *
     * = 最后一次 onChunkGenComplete 推进到的状态；若无任何生成则为 EMPTY。
     */
    [[nodiscard]] const ChunkStatus& getCurrentGenStatus() const;

    /**
     * @brief 兼容旧接口：返回当前生成状态
     */
    [[nodiscard]] const ChunkStatus& status() const { return getCurrentGenStatus(); }

    /**
     * @brief 检查当前是否至少完成到指定状态
     */
    [[nodiscard]] bool hasCompletedStatus(const ChunkStatus& status) const;

    /**
     * @brief 获取当前可变区块（对齐 NewChunkHolder.getCurrentChunk）
     *
     * 返回 m_currentChunk（累积所有已完成状态数据的 ChunkPrimer）。
     * currentGenStatus 为 EMPTY 前（尚未加载）返回 nullptr。
     * 生成任务取此作为 fromChunk 传入 WorldGenRegion 中心位置。
     */
    [[nodiscard]] ChunkPrimer* getCurrentChunk() const;

    /**
     * @brief 接管当前可变区块的所有权（对齐 NewChunkHolder currentChunk 赋值）
     *
     * EMPTY 加载/存档恢复时设置初始 ChunkPrimer。SCLM 持有 unique_ptr 所有权。
     * 生成完成后由 onChunkGenComplete 隐式保留（同一对象）。
     */
    void setCurrentChunk(std::unique_ptr<ChunkPrimer> chunk);

    /**
     * @brief 释放当前可变区块的所有权（卸载时清理用）
     *
     * FULL 完成后不再调用（对齐 Moonrise：currentChunk 保留至 holder 卸载，供邻居引用）。
     * toChunkData 非破坏性，primer 仍持有同一份 ChunkData（与 m_chunks 共享所有权）。
     *
     * @return 释放的 ChunkPrimer
     */
    [[nodiscard]] std::unique_ptr<ChunkPrimer> releaseCurrentChunk();

    /**
     * @brief 获取指定状态可用的区块（对齐 NewChunkHolder.getChunkIfPresentUnchecked）
     *
     * Cubium 的 ChunkPrimer 累积式：若 m_currentGenStatus >= status，返回 m_currentChunk（同一对象，
     * 已含所有 ≤ currentGenStatus 的状态数据）；否则返回 nullptr。
     *
     * @param status 目标状态
     * @return 可变 ChunkPrimer 指针；若未达到该状态返回 nullptr
     */
    [[nodiscard]] ChunkPrimer* getChunkIfPresentUnchecked(const ChunkStatus& status) const;

    /**
     * @brief 生成完成回调：推进 currentGenStatus（对齐 NewChunkHolder.onChunkGenComplete 的状态推进部分）
     *
     * 由 ChunkTaskScheduler.onChunkGenComplete 在持有区域锁时调用。
     * Cubium 的 ChunkPrimer 累积式：primer 是同一对象（_executeStepTask 修改同一 primer），
     * 故只需推进 m_currentGenStatus，无需重存区块（对齐 Moonrise onChunkGenComplete 的 currentGenStatus 赋值）。
     *
     * @param completedStatus 本次完成的状态
     */
    void onChunkGenComplete(const ChunkStatus& completedStatus);

    /**
     * @brief 直接设置完成状态到指定状态（存档恢复等跳过中间阶段的场景）
     *
     * 只允许向前推进。配合 markLoadedFromStorageReady 使用。
     */
    void completeStatusTo(const ChunkStatus& target);

    /**
     * @brief 兼容旧测试接口：直接设置完成状态
     */
    void setStatus(const ChunkStatus& status);

    /**
     * @brief 尝试推进生成状态（旧测试接口）
     *
     * 原子性地将完成状态从 target 的父阶段推进到 target。
     */
    bool acquireStatusBump(const ChunkStatus& target);

    // === 双向邻居依赖图（对齐 NewChunkHolder neighboursBlockingGenTask / neighboursWaitingForUs） ===

    /**
     * @brief 添加一个我正在等待的邻居
     *
     * @param neighbour 邻居生命周期管理器
     */
    void addBlockingNeighbour(SingleChunkLifecycleManager* neighbour);

    /**
     * @brief 添加一个等待我的邻居
     *
     * @param neighbour 邻居生命周期管理器
     * @param requiredStatus 邻居需要我达到的状态
     */
    void addWaitingNeighbour(SingleChunkLifecycleManager* neighbour, const ChunkStatus* requiredStatus);

    /**
     * @brief 移除一个我正在等待的邻居
     *
     * @param neighbour 邻居生命周期管理器
     */
    void removeBlockingNeighbour(SingleChunkLifecycleManager* neighbour);

    /**
     * @brief 获取等待我的邻居及所需状态（用于 onChunkGenComplete 通知）
     */
    [[nodiscard]] const std::unordered_map<SingleChunkLifecycleManager*, const ChunkStatus*>& waitingNeighbours() const
    {
        return m_waitingNeighbours;
    }

    /**
     * @brief 取出并清空等待我的邻居集合（取消/卸载清理用）
     *
     * 返回 m_waitingNeighbours 的内容并清空本端记录。调用方负责从返回的每个邻居的
     * m_blockingNeighbours 中移除本 holder（解除入向依赖）。
     */
    std::vector<std::pair<SingleChunkLifecycleManager*, const ChunkStatus*>> takeWaitingNeighbours();

    /**
     * @brief 移除一个等待我的邻居记录（取消清理用）
     *
     * @param neighbour 要移除的邻居
     */
    void removeWaitingNeighbour(SingleChunkLifecycleManager* neighbour);

    /**
     * @brief 获取我正在等待的邻居集合的副本（取消清理用）
     *
     * 返回 m_blockingNeighbours 的副本。调用方负责从每个邻居的 m_waitingNeighbours 中
     * 移除本 holder（解除出向依赖）。
     */
    [[nodiscard]] std::vector<SingleChunkLifecycleManager*> blockingNeighbours() const;

    /**
     * @brief 移除已满足所需状态的等待邻居记录
     *
     * 在 onChunkGenComplete 中调用：遍历 m_waitingNeighbours，移除 completedStatus >= requiredStatus
     * 的条目，返回被移除的邻居列表（调用方已先 removeBlockingNeighbour，这里只清理本端记录）。
     *
     * @param completedStatus 本次完成的状态
     * @return 被移除（已满足）的邻居列表
     */
    std::vector<SingleChunkLifecycleManager*> clearSatisfiedWaitingNeighbours(const ChunkStatus& completedStatus);

    /**
     * @brief 获取我正在等待的邻居数量
     */
    [[nodiscard]] size_t blockingNeighbourCount() const { return m_blockingNeighbours.size(); }

    /**
     * @brief 我是否正在等待任何邻居（阻塞中）
     */
    [[nodiscard]] bool isWaitingForNeighbors() const { return !m_blockingNeighbours.empty(); }

    /**
     * @brief 清空我正在等待的邻居集合（onChunkGenComplete 推进后调用）
     */
    void clearBlockingNeighbours();

    // === 生成任务跟踪（对齐 NewChunkHolder.genTask） ===

    /**
     * @brief 获取当前请求的目标生成状态
     */
    [[nodiscard]] const ChunkStatus& requestedGenStatus() const;

    /**
     * @brief 兼容旧接口
     */
    [[nodiscard]] const ChunkStatus& requestedStatus() const { return requestedGenStatus(); }

    /**
     * @brief 提升目标生成状态（若 newTarget 高于当前目标）
     *
     * @param newTarget 新的目标状态
     * @return true 表示目标被提升（或保持已有目标，已有进行中任务）
     */
    bool upgradeGenTarget(const ChunkStatus& newTarget);

    /**
     * @brief 判断当前是否有进行中的生成任务
     */
    [[nodiscard]] bool hasGenerationTask() const { return m_generationTask != nullptr; }

    /**
     * @brief 设置当前生成任务（断言无进行中任务）
     *
     * @param task 生成任务（由 ChunkTaskScheduler 创建，所有权不转移——任务自持 holder 引用）
     * @param scheduledStatus 本次调度的目标状态
     */
    void setGenerationTask(mc::server::ChunkProgressionTask* task, const ChunkStatus& scheduledStatus);

    /**
     * @brief 获取当前生成任务
     */
    [[nodiscard]] mc::server::ChunkProgressionTask* generationTask() const { return m_generationTask; }

    /**
     * @brief 清除当前生成任务（onChunkGenComplete 或取消时调用）
     */
    void clearGenerationTask();

    /**
     * @brief 获取当前已调度的状态（一次只推进一步）
     */
    [[nodiscard]] const ChunkStatus* scheduledStatus() const { return m_scheduledStatus; }

    // === 失败标记（对齐 Moonrise：失败即不可恢复） ===

    [[nodiscard]] bool hasFailedGeneration() const { return m_hasFailedGeneration; }
    void markFailed() { m_hasFailedGeneration = true; }

    // === 邻居引用计数（防卸载） ===

    /**
     * @brief 增加邻居引用计数（有邻居正在使用我的快照生成）
     */
    void addNeighbourUsingChunk() { m_neighboursUsingThisChunk.fetch_add(1, std::memory_order::acq_rel); }

    /**
     * @brief 减少邻居引用计数
     */
    void removeNeighbourUsingChunk() { m_neighboursUsingThisChunk.fetch_sub(1, std::memory_order::acq_rel); }

    /**
     * @brief 是否安全卸载（无邻居引用、无进行中任务）
     */
    [[nodiscard]] bool isSafeToUnload() const;

    /**
     * @brief 邻居引用计数（诊断用）：有多少邻居正在使用本区块生成
     */
    [[nodiscard]] i32 neighboursUsingThisChunkCount() const
    {
        return m_neighboursUsingThisChunk.load(std::memory_order::acquire);
    }

    /**
     * @brief 请求等待者数量（诊断用）：有多少 pending 的请求 promise/callback 挂在本 holder
     */
    [[nodiscard]] size_t waiterCount() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_waiters.size();
    }

    // === 票据与玩家追踪 ===

    [[nodiscard]] i32 level() const { return m_level.load(std::memory_order::acquire); }
    void setLevel(i32 level) { m_level.store(level, std::memory_order::release); }
    [[nodiscard]] bool shouldLoad() const { return level() <= static_cast<i32>(ChunkLoadLevel::Border); }

    [[nodiscard]] SourceState sourceState() const;
    void addTicket(const ChunkLoadTicket& ticket);
    void removeTicket(const ChunkLoadTicket& ticket);
    [[nodiscard]] size_t ticketCount() const;
    [[nodiscard]] bool hasTickets() const;
    void addTrackingPlayer(PlayerId player);
    void removeTrackingPlayer(PlayerId player);
    [[nodiscard]] size_t trackingPlayerCount() const;
    [[nodiscard]] bool hasTrackingPlayers() const;

    // === 请求聚合 ===

    /**
     * @brief 提交或合并一个区块请求
     *
     * 合并目标状态、优先级和等待者。返回调度器下一步动作决策。
     */
    EnqueueDecision submitRequest(const ChunkStatus& targetStatus,
        i32 priority,
        std::function<void(bool, ChunkData*)> callback,
        std::shared_ptr<std::promise<ChunkData*>> promise);

    /**
     * @brief 记录一次存档来源解析结果
     */
    EnqueueDecision noteStorageResolved(bool foundInStorage);

    /**
     * @brief 取消当前所有活跃工作
     */
    EnqueueDecision cancelActiveWork();

    /**
     * @brief 取出所有"已可立即完成"的等待者
     */
    std::vector<Waiter> takeReadyWaiters();

    /**
     * @brief 取出当前所有等待者（卸载/关闭/失败清理）
     */
    std::vector<Waiter> takeAllWaiters();

    // === 存档恢复 ===

    /**
     * @brief 接管从存档恢复出来的区块数据
     *
     * @param persistedStatus 存档区块的持久化阶段
     */
    void markLoadedFromStorageReady(const ChunkStatus& persistedStatus = ChunkStatuses::FULL);

    // === 旧生成执行接口（已删除，见 ChunkTaskScheduler） ===
    // createGeneratingChunk / completeGeneration / markGenerationReady /
    // noteGenerationQueued / noteGenerationStarted / noteGenerationFinished /
    // noteNeighborProgress / hasGeneratingChunk / isGenerationCurrent /
    // requestPriority / abortSignal 已被 NewChunkHolder 模型取代。

    /**
     * @brief 兼容旧测试接口：获取当前请求的取消令牌
     *
     * 新模型下取消令牌仍用于请求聚合与卸载清理，保留该接口。
     */
    [[nodiscard]] std::shared_ptr<std::atomic<bool>> abortSignal() const;

    /**
     * @brief 获取当前请求优先级（i32，越小越高，INT_MAX 表示未设置/已取消）
     *
     * m_requestPriority 由 submitRequest 单调收敛到更小值（更高优先级），
     * cancelActiveWork/markLoadedFromStorageReady 重置为 INT_MAX。
     * 用于磁盘加载优先级传播：_resolveChunkSourceSync 读取后映射到 TaskPriority，
     * 透传到 ServerIO/ServerCompute 线程池，使玩家附近区块优先加载。
     */
    [[nodiscard]] i32 requestPriority() const;

    /**
     * @brief 复活一个已被取消（abortSignal==true）的 holder，使其可被重新调度生成。
     *
     * checkNeighbour 遇到邻居 holder 处于取消态（cancelActiveWork 置位 abortSignal 但未移除 holder）
     * 时调用：分配新的 false 令牌（与 submitRequest 一致），使后续 schedule 能正常提交任务。
     *
     * 为什么需要复活：cancelActiveWork 置位 abortSignal 后不清空（保留 true），仅 submitRequest 分配
     * 新令牌。但 checkNeighbour→schedule 是邻居驱动的重新生成路径，不经 submitRequest，遇到取消态
     * holder 会因 schedule 的取消守卫返回 nullptr，建立的双向依赖（addBlockingNeighbour/
     * addWaitingNeighbour）永不解除，导致中心 holder 永久阻塞（依赖图泄漏）。
     *
     * 安全性：调用者必须持有调度区域锁（checkNeighbour 持有），cancelActiveWork 也持有同一把锁，
     * 故复活期间 abortSignal 稳定。cancelActiveWork 已清空 m_generationTask（oldTask 身份不冲突），
     * 旧运行任务检测到旧令牌（true）后自取消（execute 返回 false，onCancel→cancelGeneration
     * 因 generationTask!=oldTask 为 no-op），不调用 onChunkGenComplete，不与复活的新任务冲突。
     *
     * @return true 表示发生了复活（此前处于取消态），false 表示未处于取消态（无需复活）。
     */
    bool reviveForScheduling();

private:
    /**
     * @brief 根据当前内部状态构造调度决策（只读，无副作用）
     */
    [[nodiscard]] EnqueueDecision _buildDecisionLocked() const;

    // 对象级内存追踪守卫：绑定本对象地址，ctor 发 alloc、dtor 发 free。本类不可移动
    // （含 mutex 与 atomic），故无需 move 重绑定，ctor 初始化列表绑定 this 即可。
    // 仅 MC_ENABLE_MEMORY && MC_ENABLE_TRACY 时发事件，其余分支空操作。
    ::mc::profiler::TracyObjectTracker<"ChunkHolder"> m_memTrack;

    ChunkCoord m_x;
    ChunkCoord m_z;

    // === 生成状态（对齐 NewChunkHolder currentChunk / currentGenStatus） ===
    // Cubium 简化：ChunkPrimer 累积式（同一对象贯穿所有状态），只需 m_currentChunk 单一指针 + m_currentGenStatus。
    // 无需 Moonrise 的 chunkCompletions[] 每状态数组。
    // SCLM 持有 unique_ptr 所有权：EMPTY 加载时 setCurrentChunk 接管，FULL 完成后不释放（对齐 Moonrise：
    // currentChunk 保留至 holder 卸载，供邻居引用；ChunkData 通过 shared_ptr 与 m_chunks 共享所有权）。
    std::unique_ptr<ChunkPrimer> m_currentChunk;                   ///< 当前可变 ChunkPrimer（累积所有已完成状态数据）
    const ChunkStatus* m_currentGenStatus = &ChunkStatuses::EMPTY; ///< 当前已达到的最高生成状态

    // === 请求聚合 ===
    const ChunkStatus* m_requestedGenStatus = &ChunkStatuses::EMPTY;
    i32 m_requestPriority = std::numeric_limits<i32>::max();
    u64 m_requestGeneration = 0;
    std::shared_ptr<std::atomic<bool>> m_abortSignal;

    // === 生成任务 ===
    mc::server::ChunkProgressionTask* m_generationTask = nullptr;
    const ChunkStatus* m_scheduledStatus = nullptr;
    bool m_hasFailedGeneration = false;

    // === 双向邻居依赖图 ===
    std::unordered_set<SingleChunkLifecycleManager*> m_blockingNeighbours;                    // 我正在等待的邻居
    std::unordered_map<SingleChunkLifecycleManager*, const ChunkStatus*> m_waitingNeighbours; // 等我的邻居及所需状态

    // === 邻居引用计数 ===
    // 原子：worker 线程（scheduleStatusStep add / releaseNeighbourRefCounts remove，持调度锁）
    // 与主线程（_checkChunkUnloading isSafeToUnload 读，不持调度锁）并发访问。
    // 卸载决策的最终一致性由 unloadChunkSync 持调度锁重新检查 isSafeToUnload 保证。
    std::atomic<i32> m_neighboursUsingThisChunk{0};

    // === 来源状态 ===
    SourceState m_sourceState = SourceState::Unknown;

    // === 票据与玩家 ===
    std::atomic<i32> m_level{static_cast<i32>(ChunkLoadLevel::MaxLevel)};
    std::vector<ChunkLoadTicket> m_tickets;
    std::unordered_set<PlayerId> m_trackingPlayers;

    // === 等待者 ===
    std::vector<Waiter> m_waiters;

    // 递归互斥锁：支持 ChunkTaskScheduler 在持锁时重入调用 schedule/checkNeighbour
    mutable std::recursive_mutex m_mutex;
};

} // namespace mc::world::chunk
