#include "MeshWorkerPool.hpp"
#include "../trident/chunk/ChunkMesher.hpp"
#include "../../../common/perfetto/PerfettoManager.hpp"
#include "../../../common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc::client {

// ============================================================================
// 构造与析构
// ============================================================================

MeshWorkerPool::MeshWorkerPool(i32 threadCount)
    : m_threadCount(threadCount > 0 ? threadCount : getOptimalThreadCount())
{
}

MeshWorkerPool::~MeshWorkerPool()
{
    shutdown();
}

i32 MeshWorkerPool::getOptimalThreadCount()
{
    // 使用硬件并发数 - 1，并除以 2 来给区块生成线程留出足够资源
    // 至少 1 个，最多 114514 个
    const unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    i32 count = static_cast<i32>(hardwareConcurrency) - 1;
    return std::clamp(count / 2, 1, 114514);
}

// ============================================================================
// 生命周期
// ============================================================================

void MeshWorkerPool::start()
{
    if (m_running.exchange(true, std::memory_order_acq_rel)) {
        return; // 已经在运行
    }

    m_stop.store(false, std::memory_order_release);
    m_timestampCounter.store(0, std::memory_order_release);

    // 创建 Worker 线程
    m_workers.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&MeshWorkerPool::workerThread, this, i);
    }

    spdlog::info("MeshWorkerPool started with {} threads", m_threadCount);
}

void MeshWorkerPool::shutdown()
{
    if (!m_running.exchange(false, std::memory_order_acq_rel)) {
        return; // 已经停止
    }

    m_stop.store(true, std::memory_order_release);

    // 唤醒所有等待的线程
    m_condition.notify_all();

    // 等待所有线程结束
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    m_workers.clear();

    // 清空队列
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_taskQueue.empty()) {
            m_taskQueue.pop();
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        while (!m_completedQueue.empty()) {
            m_completedQueue.pop();
        }
    }

    spdlog::info("MeshWorkerPool shutdown complete");
}

// ============================================================================
// 任务提交
// ============================================================================

void MeshWorkerPool::submitTask(
    const ChunkId& chunkId,
    std::shared_ptr<const ChunkData> chunkData,
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors,
    i32 priority,
    u64 taskId)
{
    if (!m_running.load(std::memory_order_acquire)) {
        spdlog::warn("MeshWorkerPool: cannot submit task, pool not running");
        return;
    }

    if (!chunkData) {
        spdlog::warn("MeshWorkerPool: cannot submit task for chunk ({}, {}) with null data",
                     chunkId.x, chunkId.z);
        return;
    }

    ClientMeshTask task;
    task.chunkId = chunkId;
    task.chunkData = std::move(chunkData);
    task.neighbors = std::move(neighbors);
    task.taskId = taskId;
    task.priority = priority;
    task.timestamp = m_timestampCounter.fetch_add(1, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(task));
    }

    m_condition.notify_one();
}

// ============================================================================
// 结果处理
// ============================================================================

void MeshWorkerPool::processCompletedTasks(
    std::function<void(MeshBuildResult)> processor,
    u32 maxPerFrame)
{
    if (!processor) {
        return;
    }

    u32 processed = 0;

    while (processed < maxPerFrame) {
        MeshBuildResult result;

        {
            std::lock_guard<std::mutex> lock(m_completedMutex);
            if (m_completedQueue.empty()) {
                break;
            }
            result = std::move(m_completedQueue.front());
            m_completedQueue.pop();
        }

        processor(std::move(result));
        ++processed;
    }
}

// ============================================================================
// 统计
// ============================================================================

size_t MeshWorkerPool::pendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

size_t MeshWorkerPool::completedTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    return m_completedQueue.size();
}

// ============================================================================
// Worker 线程
// ============================================================================

void MeshWorkerPool::workerThread(i32 workerId)
{
    // 设置线程名称
    std::string threadName = "ChunkMeshWorker-" + std::to_string(workerId);
    mc::perfetto::PerfettoManager::instance().setThreadName(threadName);

    spdlog::debug("MeshWorkerPool worker {} started", workerId);

    while (!m_stop.load(std::memory_order_acquire)) {
        ClientMeshTask task;
        bool hasTask = false;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // 等待任务或停止信号
            m_condition.wait(lock, [this] {
                return !m_taskQueue.empty() || m_stop.load(std::memory_order_acquire);
            });

            if (m_stop.load(std::memory_order_acquire)) {
                break;
            }

            if (!m_taskQueue.empty()) {
                task = std::move(const_cast<ClientMeshTask&>(m_taskQueue.top()));
                m_taskQueue.pop();
                hasTask = true;
            }
        }

        // 执行任务
        if (hasTask) {
            executeTask(task);
        }
    }

    spdlog::debug("MeshWorkerPool worker {} stopped", workerId);
}

void MeshWorkerPool::executeTask(const ClientMeshTask& task)
{
    MC_TRACE_CHUNK_MESH_EVENT("BuildMesh");
    // spdlog::info("MeshWorkerPool: building mesh for chunk ({}, {}) with priority {}",
    //               task.chunkId.x, task.chunkId.z, task.priority);

    MeshBuildResult result;
    result.chunkId = task.chunkId;
    result.taskId = task.taskId;
    result.success = false;

    try {
        // 准备相邻区块指针数组（用于边界面剔除）
        const ChunkData* neighborPtrs[6] = {nullptr};
        for (size_t i = 0; i < 6; ++i) {
            neighborPtrs[i] = task.neighbors[i].get();
        }

        // 生成分层网格（实心 + 半透明）
        {
            MC_TRACE_CHUNK_MESH_EVENT("GenerateSolidMesh");
            ChunkMesher::generateSplitMesh(*task.chunkData, result.solidMesh, result.transparentMesh, neighborPtrs);
        }

        result.success = true;

    } catch (const std::exception& e) {
        spdlog::error("MeshWorkerPool: mesh build failed for chunk ({}, {}): {}",
                      task.chunkId.x, task.chunkId.z, e.what());
        result.success = false;
    } catch (...) {
        spdlog::error("MeshWorkerPool: mesh build failed for chunk ({}, {}): unknown error",
                      task.chunkId.x, task.chunkId.z);
        result.success = false;
    }

    // 将结果推入完成队列
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedQueue.push(std::move(result));
    }
}

} // namespace mc::client
