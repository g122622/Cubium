#pragma once

#include "../../../common/core/Types.hpp"
#include "../../../common/world/chunk/ChunkData.hpp"
#include "../MeshTypes.hpp"
#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mc::client {

/**
 * @brief 网格执行任务
 *
 * MeshWorkerPool 只负责执行，不再承担优先级调度。
 * 任务排序、重排和取消策略由独立调度器负责。
 */
struct MeshWorkerTask {
    ChunkId chunkId;
    u64 taskId = 0;
    std::shared_ptr<const ChunkData> chunkData;
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors;
    std::shared_ptr<std::atomic<bool>> cancelSignal;
};

/**
 * @brief 网格执行结果
 */
struct MeshWorkerResult {
    ChunkId chunkId;
    u64 taskId = 0;
    MeshData solidMesh;
    MeshData transparentMesh;
    bool success = false;
    bool cancelled = false;
};

/**
 * @brief 网格执行线程池
 *
 * 该类是纯执行器：
 * - 只按提交顺序消费任务
 * - 不感知视锥、不感知优先级
 * - 通过 cancelSignal 配合 ChunkMesher 实现协作取消
 */
class MeshWorkerPool {
public:
    explicit MeshWorkerPool(i32 threadCount);
    ~MeshWorkerPool();

    MeshWorkerPool(const MeshWorkerPool&) = delete;
    MeshWorkerPool& operator=(const MeshWorkerPool&) = delete;

    void start();
    void shutdown();

    [[nodiscard]] bool isRunning() const;

    void submit(MeshWorkerTask task);

    void drainCompleted(
        const std::function<void(MeshWorkerResult&&)>& callback,
        u32 maxCount
    );

    [[nodiscard]] size_t queuedTaskCount() const;
    [[nodiscard]] size_t runningTaskCount() const;
    [[nodiscard]] size_t completedTaskCount() const;
    [[nodiscard]] i32 threadCount() const { return m_threadCount; }

private:
    struct InternalTask {
        MeshWorkerTask task;
    };

    void workerLoop(i32 workerId);
    void executeTask(const MeshWorkerTask& task);

    [[nodiscard]] static bool isCancelled(const MeshWorkerTask& task);
    [[nodiscard]] static i32 getOptimalThreadCount();

    std::vector<std::thread> m_workers;
    i32 m_threadCount = 0;

    std::queue<InternalTask> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    std::queue<MeshWorkerResult> m_completedQueue;
    mutable std::mutex m_completedMutex;

    std::atomic<size_t> m_runningTaskCount{0};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
};

} // namespace mc::client
