#pragma once

#include "ChunkData.hpp"
#include "ChunkLoadTicket.hpp"
#include "ChunkPrimer.hpp"
#include "ChunkStatus.hpp"
#include "../../core/Types.hpp"
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace mc {

using ChunkLoadTicket = world::ChunkLoadTicket;

class SingleChunkLifecycleManager {
public:
    /**
     * @brief 区块来源解析状态
     *
     * 该状态只描述“这个区块的数据最终从哪里来”，
     * 用于把“存档恢复”与“生成调度”从架构上明确拆开。
     */
    enum class SourceState {
        Unknown,            ///< 尚未判定该区块是否存在于存档中
        ResolvingStorage,   ///< 正在执行一次性的存档来源解析
        StorageMissing,     ///< 已确认存档中不存在该区块，后续只能走生成链路
        LoadedFromStorage,  ///< 已从存档恢复出区块数据，但尚未发布到最终 Ready
        Ready               ///< 区块已经在内存中可用，可直接满足请求
    };

    /**
     * @brief 区块执行状态
     *
     * 该状态只描述“当前请求推进到了哪一步”，
     * 不负责表达区块来自存档还是生成。
     */
    enum class ExecutionState {
        Idle,               ///< 当前没有待推进的执行动作
        WaitingForNeighbors,///< 已知需要走生成，但被邻居依赖阻塞
        Queued,             ///< 邻居条件满足，等待提交或已提交给 worker
        Generating          ///< worker 已开始执行生成逻辑
    };

    /**
     * @brief 生成任务完成状态
     *
     * 用于 worker 完成回调把最终结果回灌给生命周期状态机。
     */
    enum class CompletionState {
        InProgress, ///< 仅作为占位值，正常回调不应以此结束
        Succeeded,  ///< 生成成功，区块结果可发布
        Cancelled,  ///< 生成被取消，结果必须丢弃
        Failed      ///< 生成失败，状态机回到可重试状态
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
     */
    struct EnqueueDecision {
        bool shouldResolveStorage = false;     ///< 是否需要执行一次存档来源解析
        bool shouldQueueGeneration = false;    ///< 是否需要把当前请求提交给生成 worker
        bool shouldWakeReadyWaiters = false;   ///< 是否应立即完成所有等待者
        u64 generation = 0;                    ///< 当前请求代际，用于丢弃过期 worker 结果
        i32 priority = 0;                      ///< 当前收敛后的最高优先级
        const ChunkStatus* targetStatus = nullptr; ///< 当前请求目标状态
        std::shared_ptr<std::atomic<bool>> cancelToken; ///< 当前代际对应的取消令牌
    };

    /**
     * @brief 存档解析结果
     *
     * 该结构预留给来源解析步骤返回完整结果，
     * 便于后续继续扩展为更丰富的来源决策。
     */
    struct StorageResolution {
        bool loadedFromStorage = false;
        std::unique_ptr<ChunkData> loadedChunk;
    };

    /**
     * @brief 创建单区块生命周期管理器
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    SingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 析构函数
     */
    ~SingleChunkLifecycleManager() = default;

    SingleChunkLifecycleManager(const SingleChunkLifecycleManager&) = delete;
    SingleChunkLifecycleManager& operator=(const SingleChunkLifecycleManager&) = delete;
    SingleChunkLifecycleManager(SingleChunkLifecycleManager&&) noexcept = default;
    SingleChunkLifecycleManager& operator=(SingleChunkLifecycleManager&&) noexcept = default;

    /**
     * @brief 获取区块 X 坐标
     */
    [[nodiscard]] ChunkCoord x() const { return m_x; }

    /**
     * @brief 获取区块 Z 坐标
     */
    [[nodiscard]] ChunkCoord z() const { return m_z; }

    /**
     * @brief 获取区块位置对象
     */
    [[nodiscard]] ChunkPos pos() const { return ChunkPos(m_x, m_z); }

    /**
     * @brief 获取区块唯一 ID
     */
    [[nodiscard]] u64 id() const { return ChunkId(m_x, m_z, 0).toId(); }

    /**
     * @brief 获取当前已完成的区块状态
     *
     * 返回值表示该区块在“内容生成阶段”上的实际完成进度，
     * 与请求目标状态不是同一个概念。
     */
    [[nodiscard]] const ChunkStatus& status() const;

    /**
     * @brief 获取当前已完成的区块状态
     *
     * 这是只读访问器的兼容命名别名，语义与 `status()` 完全一致。
     */
    [[nodiscard]] const ChunkStatus& getStatus() const { return status(); }

    /**
     * @brief 检查当前区块是否至少完成到指定阶段
     *
     * @param status 目标阶段
     * @return 如果当前完成阶段不低于目标阶段，返回 true
     */
    [[nodiscard]] bool hasCompletedStatus(const ChunkStatus& status) const;

    /**
     * @brief 获取当前加载级别
     */
    [[nodiscard]] i32 level() const { return m_level.load(std::memory_order_acquire); }

    /**
     * @brief 获取当前加载级别
     *
     * 这是只读访问器的兼容命名别名，语义与 `level()` 完全一致。
     */
    [[nodiscard]] i32 getLevel() const { return level(); }

    /**
     * @brief 设置当前加载级别
     *
     * @param level 新的加载级别
     */
    void setLevel(i32 level) { m_level.store(level, std::memory_order_release); }

    /**
     * @brief 判断该区块当前是否应当保持加载
     *
     * @return 当 level <= 33 时返回 true
     */
    [[nodiscard]] bool shouldLoad() const { return level() <= 33; }

    /**
     * @brief 获取区块来源解析状态
     */
    [[nodiscard]] SourceState sourceState() const;

    /**
     * @brief 获取区块执行状态
     */
    [[nodiscard]] ExecutionState executionState() const;

    /**
     * @brief 判断区块当前是否因邻居依赖而阻塞
     */
    [[nodiscard]] bool isWaitingForNeighbors() const;

    /**
     * @brief 判断是否已创建生成中的 ChunkPrimer
     */
    [[nodiscard]] bool hasGeneratingChunk() const;

    /**
     * @brief 获取已就绪的区块数据
     *
     * @return 内存中的 ChunkData 指针；若尚未就绪则返回 nullptr
     */
    [[nodiscard]] ChunkData* chunkData();

    /**
     * @brief 获取已就绪的区块数据（const 版本）
     *
     * @return 内存中的 ChunkData 指针；若尚未就绪则返回 nullptr
     */
    [[nodiscard]] const ChunkData* chunkData() const;

    /**
     * @brief 添加一个加载票据
     *
     * @param ticket 要添加的票据
     */
    void addTicket(const ChunkLoadTicket& ticket);

    /**
     * @brief 移除一个加载票据
     *
     * @param ticket 要移除的票据
     */
    void removeTicket(const ChunkLoadTicket& ticket);

    /**
     * @brief 获取当前票据数量
     */
    [[nodiscard]] size_t ticketCount() const;

    /**
     * @brief 判断当前是否仍持有任意加载票据
     */
    [[nodiscard]] bool hasTickets() const;

    /**
     * @brief 添加正在追踪该区块的玩家
     *
     * @param player 玩家 ID
     */
    void addTrackingPlayer(PlayerId player);

    /**
     * @brief 移除正在追踪该区块的玩家
     *
     * @param player 玩家 ID
     */
    void removeTrackingPlayer(PlayerId player);

    /**
     * @brief 获取追踪该区块的玩家数量
     */
    [[nodiscard]] size_t trackingPlayerCount() const;

    /**
     * @brief 判断当前是否有玩家正在追踪该区块
     */
    [[nodiscard]] bool hasTrackingPlayers() const;

    /**
     * @brief 提交或合并一个区块请求
     *
     * 这是新的统一请求入口。所有同步/异步请求都应先进入这里，
     * 由生命周期状态机负责合并目标阶段、优先级和等待者集合。
     *
     * @param targetStatus 请求目标阶段
     * @param priority 请求优先级，数值越小优先级越高
     * @param callback 异步回调，可为空
     * @param promise future 对应的 promise，可为空
     * @return 调度器下一步应执行的动作决策
     */
    EnqueueDecision submitRequest(
        const ChunkStatus& targetStatus,
        i32 priority,
        std::function<void(bool, ChunkData*)> callback,
        std::shared_ptr<std::promise<ChunkData*>> promise);

    /**
     * @brief 记录一次邻居推进事件
     *
     * 当周围区块状态前进后，ServerChunkManager 会调用此函数，
     * 让当前区块重新评估是否可以从 WaitingForNeighbors 进入可排队状态。
     *
     * @param neighborsReady 当前邻居条件是否已满足
     * @return 调度器下一步应执行的动作决策
     */
    EnqueueDecision noteNeighborProgress(bool neighborsReady);

    /**
     * @brief 记录一次存档来源解析结果
     *
     * @param foundInStorage 是否在存档中找到了区块数据
     * @return 调度器下一步应执行的动作决策
     */
    EnqueueDecision noteStorageResolved(bool foundInStorage);

    /**
     * @brief 记录当前 generation 已进入排队阶段
     *
     * @param generation 本次排队对应的请求代际
     * @return 调度器下一步应执行的动作决策
     */
    EnqueueDecision noteGenerationQueued(u64 generation);

    /**
     * @brief 记录当前 generation 已开始执行
     *
     * @param generation 本次执行对应的请求代际
     * @return 调度器下一步应执行的动作决策
     */
    EnqueueDecision noteGenerationStarted(u64 generation);

    /**
     * @brief 记录一次生成任务完成事件
     *
     * @param generation 完成事件对应的请求代际
     * @param completionState 完成结果
     * @return 调度器下一步应执行的动作决策
     */
    EnqueueDecision noteGenerationFinished(u64 generation, CompletionState completionState);

    /**
     * @brief 取消当前所有活跃工作
     *
     * 该函数会使当前代际失效，并触发取消令牌。
     * 取消后不会自动完成等待者，调用方需要自行决定是失败返回还是继续保留请求。
     *
     * @return 调度器下一步应执行的动作决策
     */
    EnqueueDecision cancelActiveWork();

    /**
     * @brief 取出所有“已可立即完成”的等待者
     *
     * 该函数用于区块已经 Ready 的场景。
     * 调用后内部等待者列表会被清空。
     *
     * @return 当前挂起的所有等待者
     */
    std::vector<Waiter> takeReadyWaiters();

    /**
     * @brief 取出当前所有等待者
     *
     * 该函数通常用于卸载、关闭或失败清理路径。
     * 调用后内部等待者列表会被清空。
     *
     * @return 当前挂起的所有等待者
     */
    std::vector<Waiter> takeAllWaiters();

    /**
     * @brief 获取或创建生成中的 ChunkPrimer
     *
     * 仅当状态机已经决定进入生成执行阶段时才应调用。
     *
     * @return 当前区块对应的 ChunkPrimer
     */
    ChunkPrimer* createGeneratingChunk();

    /**
     * @brief 完成生成并提取最终 ChunkData
     *
     * 该函数会把内部 ChunkPrimer 转换为 ChunkData，
     * 同时把来源状态推进到 Ready。
     *
     * @return 新生成的 ChunkData 所有权
     */
    std::unique_ptr<ChunkData> completeGeneration();

    /**
     * @brief 标记异步生成已成功完成
     *
     * 当生成工作在线程池内使用外部 `ChunkPrimer` 完成时，
     * 生命周期管理器内部并不一定持有 `m_generatingChunk`。
     * 这种情况下使用该接口只推进状态，不再尝试提取内部生成结果。
     */
    void markGenerationReady();

    /**
     * @brief 接管从存档恢复出来的区块数据
     *
     * @param data 已成功恢复的区块数据
     */
    void markLoadedFromStorageReady();

    /**
     * @brief 直接设置当前完成阶段
     *
     * 该函数仅用于区块已经就绪后同步修正内部状态，
     * 不承担调度或回调语义。
     *
     * @param status 新的完成阶段
     */
    void setStatus(const ChunkStatus& status);

    /**
     * @brief 判断指定 generation 是否仍是当前有效请求
     *
     * @param generation 待检查的请求代际
     * @return 若与当前 active generation 一致则返回 true
     */
    [[nodiscard]] bool isGenerationCurrent(u64 generation) const;

    /**
     * @brief 获取当前收敛后的请求优先级
     */
    [[nodiscard]] i32 requestPriority() const;

    /**
     * @brief 获取当前收敛后的请求目标状态
     */
    [[nodiscard]] const ChunkStatus& requestedStatus() const;

    /**
     * @brief 获取当前请求的取消令牌
     *
     * @return 当前 generation 对应的取消令牌；若当前没有活跃请求则返回空指针
     */
    [[nodiscard]] std::shared_ptr<std::atomic<bool>> cancelToken() const;

private:
    /**
     * @brief 根据当前内部状态构造一份调度决策
     *
     * 该函数只读内部状态，不产生副作用。
     * 所有真正的 IO、worker 提交和等待者完成都由上层调度器执行。
     */
    [[nodiscard]] EnqueueDecision buildDecisionLocked() const;

    /**
     * @brief 在持锁状态下清理当前 active generation
     *
     * 该函数会推进 generation、触发取消令牌并丢弃生成中间态，
     * 用于取消、失败恢复和卸载清理等路径。
     */
    void clearActiveGenerationLocked();

    ChunkCoord m_x;
    ChunkCoord m_z;

    const ChunkStatus* m_status = &ChunkStatuses::EMPTY;
    std::atomic<i32> m_level{33};

    SourceState m_sourceState = SourceState::Unknown;
    ExecutionState m_executionState = ExecutionState::Idle;

    const ChunkStatus* m_requestedStatus = &ChunkStatuses::EMPTY;
    i32 m_requestPriority = std::numeric_limits<i32>::max();
    u64 m_requestGeneration = 0;
    u64 m_submittedGeneration = 0;
    std::shared_ptr<std::atomic<bool>> m_cancelToken;

    std::unique_ptr<ChunkPrimer> m_generatingChunk;
    std::vector<Waiter> m_waiters;
    std::vector<ChunkLoadTicket> m_tickets;
    std::unordered_set<PlayerId> m_trackingPlayers;

    mutable std::mutex m_mutex;
};

} // namespace mc
