#pragma once

#include "../../../common/core/Types.hpp"
#include "../../../common/world/chunk/ChunkData.hpp"
#include "../MeshTypes.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <functional>
#include <array>

namespace mc::client {

// ============================================================================
// 网格构建结果
// ============================================================================

/**
 * @brief 网格构建结果
 *
 * Worker 线程完成网格构建后返回的数据结构。
 * 包含区块 ID、实心网格和透明网格。
 */
struct MeshBuildResult {
    ChunkId chunkId;              ///< 区块 ID
    u64 taskId = 0;               ///< 任务代际 ID（用于丢弃过期结果）
    MeshData solidMesh;           ///< 实心方块网格
    MeshData transparentMesh;     ///< 透明方块网格
    bool success = false;         ///< 是否成功构建
};

// ============================================================================
// 客户端网格构建任务
// ============================================================================

/**
 * @brief 客户端网格构建任务
 *
 * 提交到 MeshWorkerPool 的任务描述。
 * 包含区块数据指针和相邻区块数据指针（用于边界面剔除）。
 *
 * 注意：使用 shared_ptr 共享所有权，确保 Worker 线程访问期间数据有效。
 */
struct ClientMeshTask {
    ChunkId chunkId;                                           ///< 区块 ID
    std::shared_ptr<const ChunkData> chunkData;               ///< 区块数据
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors; ///< 相邻区块 (-X, +X, -Z, +Z, -Y, +Y)
    u64 taskId = 0;                                            ///< 任务代际 ID
    i32 priority = 0;                                          ///< 优先级（越小越高）
    u64 timestamp = 0;                                         ///< 提交时间戳

    /**
     * @brief 优先队列比较器
     *
     * priority 越小优先级越高
     * timestamp 越小优先级越高（先提交的先处理）
     */
    bool operator<(const ClientMeshTask& other) const {
        if (priority != other.priority) return priority > other.priority;
        return timestamp > other.timestamp;
    }
};

// ============================================================================
// 网格构建线程池
// ============================================================================

/**
 * @brief 网格构建线程池
 *
 * 管理多个 Worker 线程，异步执行区块网格构建任务。
 * 解决区块数据接收时的主线程卡顿问题。
 *
 * 使用方法：
 * @code
 * MeshWorkerPool pool(4);  // 4 个 Worker 线程
 * pool.start();
 *
 * // 提交任务（非阻塞）
 * pool.submitTask(chunkId, chunkData, neighbors, priority);
 *
 * // 每帧处理完成的结果（主线程调用）
 * pool.processCompletedTasks([](MeshBuildResult result) {
 *     // 更新 GPU 缓冲区
 * }, 4);  // 每帧最多处理 4 个
 *
 * // 关闭
 * pool.shutdown();
 * @endcode
 *
 * 线程安全：
 * - 任务队列和完成队列使用 mutex 保护
 * - ChunkData 通过 shared_ptr 共享，创建后不可变
 * - MeshData 所有权从 Worker 转移到主线程
 */
class MeshWorkerPool {
public:
    /**
     * @brief 完成结果处理器类型
     */
    using CompletionCallback = std::function<void(MeshBuildResult)>;

    // ============================================================================
    // 构造与析构
    // ============================================================================

    /**
     * @brief 创建网格构建线程池
     * @param threadCount 线程数量（-1 表示自动检测，默认使用 CPU 核心数 - 1）
     */
    explicit MeshWorkerPool(i32 threadCount = -1);

    ~MeshWorkerPool();

    // 禁止拷贝
    MeshWorkerPool(const MeshWorkerPool&) = delete;
    MeshWorkerPool& operator=(const MeshWorkerPool&) = delete;

    // ============================================================================
    // 生命周期
    // ============================================================================

    /**
     * @brief 启动 Worker 线程
     *
     * 必须在提交任务前调用。
     * 线程启动后立即开始等待任务。
     */
    void start();

    /**
     * @brief 关闭 Worker 线程
     *
     * 会等待当前任务完成后退出。
     * 未完成的任务会被丢弃。
     */
    void shutdown();

    /**
     * @brief 检查是否正在运行
     */
    [[nodiscard]] bool isRunning() const { return m_running.load(std::memory_order_acquire); }

    // ============================================================================
    // 任务提交
    // ============================================================================

    /**
     * @brief 提交网格构建任务
     *
     * 非阻塞调用，任务被添加到优先队列等待 Worker 处理。
     *
     * @param chunkId 区块 ID
     * @param chunkData 区块数据（shared_ptr 共享所有权）
     * @param neighbors 相邻区块数据（可为 nullptr）
     * @param priority 优先级（越小越高，默认 0）
     * @param taskId 任务代际 ID（同一 chunk 的旧任务结果会被丢弃）
     */
    void submitTask(
        const ChunkId& chunkId,
        std::shared_ptr<const ChunkData> chunkData,
        std::array<std::shared_ptr<const ChunkData>, 6> neighbors,
        i32 priority = 0,
        u64 taskId = 0
    );

    // ============================================================================
    // 结果处理
    // ============================================================================

    /**
     * @brief 处理已完成的网格构建结果
     *
     * 在主线程每帧调用一次，处理 Worker 线程完成的结果。
     * 使用 maxPerFrame 限制每帧处理数量，避免帧卡顿。
     *
     * @param processor 处理函数，接收 MeshBuildResult
     * @param maxPerFrame 每帧最多处理数量（默认 4）
     */
    void processCompletedTasks(
        std::function<void(MeshBuildResult)> processor,
        u32 maxPerFrame = 4
    );

    // ============================================================================
    // 统计
    // ============================================================================

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] size_t pendingTaskCount() const;

    /**
     * @brief 获取已完成但未处理的结果数量
     */
    [[nodiscard]] size_t completedTaskCount() const;

    /**
     * @brief 获取线程数量
     */
    [[nodiscard]] i32 threadCount() const { return m_threadCount; }

private:
    /**
     * @brief Worker 线程函数
     */
    void workerThread(i32 workerId);

    /**
     * @brief 执行单个网格构建任务
     */
    void executeTask(const ClientMeshTask& task);

    /**
     * @brief 获取最优线程数
     */
    static i32 getOptimalThreadCount();

    // 线程
    std::vector<std::thread> m_workers;
    i32 m_threadCount;

    // 任务队列
    std::priority_queue<ClientMeshTask> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    // 完成队列
    std::queue<MeshBuildResult> m_completedQueue;
    mutable std::mutex m_completedMutex;

    // 时间戳计数器
    std::atomic<u64> m_timestampCounter{0};

    // 状态
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
};

} // namespace mc::client
