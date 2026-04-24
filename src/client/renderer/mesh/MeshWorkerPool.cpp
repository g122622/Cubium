#include "MeshWorkerPool.hpp"
#include "../trident/chunk/ChunkMesher.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <string>

namespace mc::client {

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
    const unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    const i32 count = static_cast<i32>(hardwareConcurrency) - 1;
    return std::clamp(count / 2, 1, 32);
}

void MeshWorkerPool::start()
{
    if (m_running.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    m_runningTaskCount.store(0, std::memory_order_release);
    m_stop.store(false, std::memory_order_release);

    m_workers.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&MeshWorkerPool::workerLoop, this, i);
    }

    spdlog::info("MeshWorkerPool started with {} threads", m_threadCount);
}

void MeshWorkerPool::shutdown()
{
    if (!m_running.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    m_stop.store(true, std::memory_order_release);
    m_condition.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    m_workers.clear();

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

    m_runningTaskCount.store(0, std::memory_order_release);

    spdlog::info("MeshWorkerPool shutdown complete");
}

bool MeshWorkerPool::isRunning() const
{
    return m_running.load(std::memory_order_acquire);
}

void MeshWorkerPool::submit(MeshWorkerTask task)
{
    if (!isRunning()) {
        spdlog::warn("MeshWorkerPool: submit ignored because pool is not running");
        return;
    }

    if (!task.chunkData) {
        spdlog::warn(
            "MeshWorkerPool: submit ignored for chunk ({}, {}), chunk data is null",
            task.chunkId.x,
            task.chunkId.z
        );
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        InternalTask internalTask;
        internalTask.task = std::move(task);
        m_taskQueue.push(std::move(internalTask));
    }

    m_condition.notify_one();
}

void MeshWorkerPool::drainCompleted(
    const std::function<void(MeshWorkerResult&&)>& callback,
    u32 maxCount
)
{
    if (!callback || maxCount == 0) {
        return;
    }

    u32 drained = 0;

    while (drained < maxCount) {
        MeshWorkerResult result;

        {
            std::lock_guard<std::mutex> lock(m_completedMutex);
            if (m_completedQueue.empty()) {
                break;
            }
            result = std::move(m_completedQueue.front());
            m_completedQueue.pop();
        }

        callback(std::move(result));
        ++drained;
    }
}

size_t MeshWorkerPool::queuedTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

size_t MeshWorkerPool::runningTaskCount() const
{
    return m_runningTaskCount.load(std::memory_order_acquire);
}

size_t MeshWorkerPool::completedTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    return m_completedQueue.size();
}

bool MeshWorkerPool::isCancelled(const MeshWorkerTask& task)
{
    return task.cancelSignal && task.cancelSignal->load(std::memory_order_acquire);
}

void MeshWorkerPool::workerLoop(i32 workerId)
{
    const std::string threadName = "ChunkMeshWorker-" + std::to_string(workerId);
    mc::perfetto::PerfettoManager::instance().setThreadName(threadName);

    while (true) {
        MeshWorkerTask task;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            m_condition.wait(lock, [this] {
                return !m_taskQueue.empty() || m_stop.load(std::memory_order_acquire);
            });

            if (m_stop.load(std::memory_order_acquire) && m_taskQueue.empty()) {
                break;
            }

            if (m_taskQueue.empty()) {
                continue;
            }

            InternalTask internalTask = std::move(m_taskQueue.front());
            m_taskQueue.pop();
            task = std::move(internalTask.task);
        }

        m_runningTaskCount.fetch_add(1, std::memory_order_acq_rel);
        executeTask(task);
        m_runningTaskCount.fetch_sub(1, std::memory_order_acq_rel);
    }
}

void MeshWorkerPool::executeTask(const MeshWorkerTask& task)
{
    ChunkPos chunkPos(task.chunkId.x, task.chunkId.z);
    MC_TRACE_CHUNK_MESH_EVENT(
        "BuildMesh",
        "pos", fmt::format("({}, {})", task.chunkId.x, task.chunkId.z),
        [flow = ::perfetto::Flow::ProcessScoped(chunkPos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });

    MeshWorkerResult result;
    result.chunkId = task.chunkId;
    result.taskId = task.taskId;
    result.success = false;
    result.cancelled = false;

    if (isCancelled(task)) {
        result.cancelled = true;
        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedQueue.push(std::move(result));
        return;
    }

    try {
        const ChunkData* neighborPtrs[6] = {nullptr};
        for (size_t i = 0; i < 6; ++i) {
            neighborPtrs[i] = task.neighbors[i].get();
        }

        MC_TRACE_CHUNK_MESH_EVENT("GenerateSolidMesh");
        ChunkMesher::generateSplitMesh(
            *task.chunkData,
            result.solidMesh,
            result.transparentMesh,
            neighborPtrs,
            task.cancelSignal.get()
        );

        if (isCancelled(task)) {
            result.cancelled = true;
            result.success = false;
        } else {
            result.success = true;
        }

    } catch (const std::exception& e) {
        spdlog::error(
            "MeshWorkerPool: build failed for chunk ({}, {}): {}",
            task.chunkId.x,
            task.chunkId.z,
            e.what()
        );
        result.success = false;
    } catch (...) {
        spdlog::error(
            "MeshWorkerPool: build failed for chunk ({}, {}): unknown error",
            task.chunkId.x,
            task.chunkId.z
        );
        result.success = false;
    }

    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedQueue.push(std::move(result));
    }
}

} // namespace mc::client
