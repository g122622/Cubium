#pragma once

#include "ChunkStatus.hpp"
#include "ChunkPrimer.hpp"
#include "ChunkData.hpp"
#include "ChunkLoadTicket.hpp"
#include "../../core/Types.hpp"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

namespace mc {

// 导入票据类型到当前命名空间
using ChunkLoadTicket = world::ChunkLoadTicket;

/**
 * @brief 区块持有者
 *
 * 参考 MC SingleChunkLifecycleManager，管理单个区块的加载状态和生成进度。
 * 每个区块对应一个 SingleChunkLifecycleManager，跟踪其生成阶段和 Future 链。
 *
 * 使用方法：
 * 1. SingleChunkLifecycleManager 在区块首次被请求时创建
 * 2. 通过 scheduleGeneration() 启动生成流程
 * 3. 通过 getChunkFuture() 获取指定阶段的区块 Future
 * 4. 当区块不再需要时，标记为卸载
 */
class SingleChunkLifecycleManager {
public:
    /**
     * @brief 区块加载错误
     */
    enum class Error {
        None,
        Unloaded,
        GenerationFailed,
        Timeout
    };

    /**
     * @brief 区块结果类型
     */
    using ChunkResult = std::variant<ChunkPrimer*, Error>;

    /**
     * @brief 区块 Future 类型
     */
    using ChunkFuture = std::shared_future<ChunkResult>;

    /**
     * @brief 生成任务回调类型
     */
    using GenerationCallback = std::function<void(SingleChunkLifecycleManager&)>;

    // ============================================================================
    // 构造函数
    // ============================================================================

    /**
     * @brief 创建区块持有者
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    SingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z);

    ~SingleChunkLifecycleManager() = default;

    // 禁止拷贝
    SingleChunkLifecycleManager(const SingleChunkLifecycleManager&) = delete;
    SingleChunkLifecycleManager& operator=(const SingleChunkLifecycleManager&) = delete;

    // 允许移动
    SingleChunkLifecycleManager(SingleChunkLifecycleManager&&) noexcept = default;
    SingleChunkLifecycleManager& operator=(SingleChunkLifecycleManager&&) noexcept = default;

    // ============================================================================
    // 位置信息
    // ============================================================================

    [[nodiscard]] ChunkCoord x() const { return m_x; }
    [[nodiscard]] ChunkCoord z() const { return m_z; }
    [[nodiscard]] ChunkPos pos() const { return ChunkPos(m_x, m_z); }
    [[nodiscard]] u64 id() const { return ChunkId(m_x, m_z, 0).toId(); }

    // ============================================================================
    // 状态管理
    // ============================================================================

    /**
     * @brief 获取当前区块状态
     */
    [[nodiscard]] const ChunkStatus& getStatus() const { return *m_status; }

    /**
     * @brief 设置当前区块状态
     */
    void setStatus(const ChunkStatus& status);

    /**
     * @brief 检查是否已完成指定阶段
     */
    [[nodiscard]] bool hasCompletedStatus(const ChunkStatus& status) const {
        return m_status->isAtLeast(status);
    }

    /**
     * @brief 获取加载级别
     *
     * 级别越小优先级越高。
     * 级别 <= 33 的区块应该被加载。
     */
    [[nodiscard]] i32 getLevel() const { return m_level.load(std::memory_order_acquire); }

    /**
     * @brief 设置加载级别
     */
    void setLevel(i32 level);

    /**
     * @brief 检查区块是否应该加载
     *
     * @note 级别 <= 33 的区块应该被加载（参考 ChunkLoadTicketManager::MAX_LOADED_LEVEL）
     */
    [[nodiscard]] bool shouldLoad() const { return getLevel() <= 33; }

    // ============================================================================
    // 区块数据访问
    // ============================================================================

    /**
     * @brief 获取正在生成的区块
     */
    [[nodiscard]] ChunkPrimer* getGeneratingChunk() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_generatingChunk.get();
    }
    [[nodiscard]] const ChunkPrimer* getGeneratingChunk() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_generatingChunk.get();
    }

    [[nodiscard]] bool hasGeneratingChunk() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_generatingChunk != nullptr;
    }

    /**
     * @brief 获取已完成的区块数据
     */
    [[nodiscard]] ChunkData* getChunkData() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_chunkData.get();
    }
    [[nodiscard]] const ChunkData* getChunkData() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_chunkData.get();
    }

    /**
     * @brief 设置区块数据（加载完成时）
     */
    void setChunkData(std::unique_ptr<ChunkData> data) {
        m_chunkData = std::move(data);
    }

    /**
     * @brief 创建新的生成区块
     */
    ChunkPrimer* createGeneratingChunk();

    /**
     * @brief 完成生成，转换为 ChunkData
     */
    std::unique_ptr<ChunkData> completeGeneration();

    // ============================================================================
    // Future 管理
    // ============================================================================

    /**
     * @brief 获取指定阶段的 Future
     *
     * 如果区块已经达到该阶段，返回已完成的 Future。
     * 否则返回等待中的 Future。
     */
    [[nodiscard]] ChunkFuture getChunkFuture(const ChunkStatus& status);

    /**
     * @brief 设置 Future 完成
     */
    void completeFuture(const ChunkStatus& status, ChunkPrimer* chunk);

    /**
     * @brief 设置 Future 错误
     */
    void failFuture(const ChunkStatus& status, Error error);

    // ============================================================================
    // 票据管理
    // ============================================================================

    /**
     * @brief 添加票据
     */
    void addTicket(const ChunkLoadTicket& ticket);

    /**
     * @brief 移除票据
     */
    void removeTicket(const ChunkLoadTicket& ticket);

    /**
     * @brief 获取票据数量
     */
    [[nodiscard]] size_t ticketCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tickets.size();
    }

    /**
     * @brief 检查是否有票据
     */
    [[nodiscard]] bool hasTickets() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_tickets.empty();
    }

    // ============================================================================
    // 玩家追踪
    // ============================================================================

    /**
     * @brief 添加追踪玩家
     */
    void addTrackingPlayer(PlayerId player);

    /**
     * @brief 移除追踪玩家
     */
    void removeTrackingPlayer(PlayerId player);

    /**
     * @brief 获取追踪玩家数量
     */
    [[nodiscard]] size_t trackingPlayerCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_trackingPlayers.size();
    }

    /**
     * @brief 检查是否有追踪玩家
     */
    [[nodiscard]] bool hasTrackingPlayers() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_trackingPlayers.empty();
    }

    // ============================================================================
    // 回调
    // ============================================================================

    /**
     * @brief 设置级别变化回调
     */
    void setLevelChangeCallback(GenerationCallback callback) {
        m_levelChangeCallback = std::move(callback);
    }

    /**
     * @brief 设置状态变化回调
     */
    void setStatusChangeCallback(GenerationCallback callback) {
        m_statusChangeCallback = std::move(callback);
    }

    // ============================================================================
    // 标记
    // ============================================================================

    [[nodiscard]] bool isDirty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_dirty;
    }
    void setDirty(bool dirty) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_dirty = dirty;
    }

    [[nodiscard]] bool isQueuedForUnload() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queuedForUnload;
    }
    void setQueuedForUnload(bool queued) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queuedForUnload = queued;
    }

private:
    ChunkCoord m_x;
    ChunkCoord m_z;

    // 区块状态
    const ChunkStatus* m_status = &ChunkStatuses::EMPTY;

    // 加载级别（原子操作）
    std::atomic<i32> m_level{33};

    // 正在生成的区块
    std::unique_ptr<ChunkPrimer> m_generatingChunk;

    // 已完成的区块数据
    std::unique_ptr<ChunkData> m_chunkData;

    // Future 缓存（每个阶段一个）
    std::array<std::promise<ChunkResult>, ChunkStatuses::COUNT> m_promises;
    std::array<ChunkFuture, ChunkStatuses::COUNT> m_futures;
    std::array<bool, ChunkStatuses::COUNT> m_futureInitialized{};

    // 票据
    std::vector<ChunkLoadTicket> m_tickets;

    // 追踪玩家
    std::unordered_set<PlayerId> m_trackingPlayers;

    // 回调
    GenerationCallback m_levelChangeCallback;
    GenerationCallback m_statusChangeCallback;

    // 标记
    bool m_dirty = false;
    bool m_queuedForUnload = false;

    // 互斥锁
    mutable std::mutex m_mutex;
};

// ============================================================================
// 区块任务
// ============================================================================

/**
 * @brief 区块生成任务
 */
struct ChunkTask {
    enum class Type {
        Generate,       // 生成区块
        Load,           // 加载区块
        Unload,         // 卸载区块
        Save            // 保存区块
    };

    Type type = Type::Generate;
    ChunkCoord x = 0;
    ChunkCoord z = 0;
    const ChunkStatus* targetStatus = nullptr;
    i32 priority = 0;  // 越小优先级越高
    u64 timestamp = 0; // 创建时间

    ChunkTask() = default;

    ChunkTask(Type t, ChunkCoord x_, ChunkCoord z_, const ChunkStatus* status = nullptr, i32 prio = 0)
        : type(t), x(x_), z(z_), targetStatus(status), priority(prio), timestamp(0) {}

    /**
     * @brief 比较函数（用于优先队列）
     */
    bool operator<(const ChunkTask& other) const {
        if (priority != other.priority) {
            return priority > other.priority;  // 优先级小的在前
        }
        return timestamp > other.timestamp;  // 时间早的在前
    }
};

} // namespace mc
