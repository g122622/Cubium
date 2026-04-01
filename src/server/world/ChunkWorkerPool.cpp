#include "ChunkWorkerPool.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <chrono>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::server {

namespace {

[[nodiscard]] u64 makeChunkKey(ChunkCoord x, ChunkCoord z)
{
    const u32 ux = static_cast<u32>(x);
    const u32 uz = static_cast<u32>(z);
    return (static_cast<u64>(ux) << 32) | static_cast<u64>(uz);
}

} // namespace

// ============================================================================
// 构造与析构
// ============================================================================

ChunkWorkerPool::ChunkWorkerPool(i32 threadCount)
    : m_threadCount(threadCount > 0 ? threadCount : getOptimalThreadCount())
{
}

ChunkWorkerPool::~ChunkWorkerPool()
{
    shutdown();
}

i32 ChunkWorkerPool::getOptimalThreadCount()
{
    // 使用硬件并发数，至少 1 个，最多 114514 个
    const unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    return static_cast<i32>(std::clamp(hardwareConcurrency / 2, 1u, 114514u));
}

// ============================================================================
// 生命周期
// ============================================================================

void ChunkWorkerPool::start()
{
    if (m_running.exchange(true, std::memory_order_acq_rel)) {
        return; // 已经在运行
    }

    m_stop.store(false, std::memory_order_release);

    // 创建 Worker 线程
    m_workers.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&ChunkWorkerPool::workerThread, this, i);
    }
}

void ChunkWorkerPool::shutdown()
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

    {
        std::lock_guard<std::mutex> queueLock(m_queueMutex);
        while (!m_taskQueue.empty()) {
            m_taskQueue.pop();
        }
        m_pendingGenerateTasks.clear();
    }

    {
        std::lock_guard<std::mutex> completedLock(m_completedMutex);
        m_completedChunks.clear();
    }
}

// ============================================================================
// 任务提交
// ============================================================================

void ChunkWorkerPool::submitGenerate(ChunkCoord x, ChunkCoord z,
                                       const ChunkStatus& targetStatus,
                                       CompletionCallback callback,
                                       i32 priority)
{
    if (!m_running.load(std::memory_order_acquire)) {
        if (callback) {
            callback(false, nullptr);
        }
        return;
    }

    ChunkTask chunkTask(ChunkTask::Type::Generate, x, z, &targetStatus, priority);
    chunkTask.timestamp = static_cast<u64>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    const u64 chunkKey = makeChunkKey(x, z);
    auto coalescedCallbacks = std::make_shared<CoalescedCallbacks>();
    std::shared_ptr<CoalescedCallbacks> existingCallbacks;
    bool shouldNotifyWorker = false;

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        auto pendingIt = m_pendingGenerateTasks.find(chunkKey);
        if (pendingIt != m_pendingGenerateTasks.end()) {
            existingCallbacks = pendingIt->second;
        } else {
            if (callback) {
                std::lock_guard<std::mutex> callbackLock(coalescedCallbacks->mutex);
                coalescedCallbacks->callbacks.push_back(std::move(callback));
            }

            auto task = std::make_shared<InternalTask>();
            task->task = std::move(chunkTask);
            task->generator = m_generator;
            task->coalesceKey = chunkKey;
            task->coalescedCallbacks = coalescedCallbacks;

            m_pendingGenerateTasks.emplace(chunkKey, coalescedCallbacks);
            m_taskQueue.push(std::move(task));
            shouldNotifyWorker = true;
        }
    }

    if (existingCallbacks) {
        if (callback) {
            std::lock_guard<std::mutex> callbackLock(existingCallbacks->mutex);
            existingCallbacks->callbacks.push_back(std::move(callback));
        }
        return;
    }

    if (shouldNotifyWorker) {
        m_condition.notify_one();
    }
}

void ChunkWorkerPool::submitTask(ChunkTask task,
                                   GeneratorFunc generator,
                                   CompletionCallback callback)
{
    if (!m_running.load(std::memory_order_acquire)) {
        if (callback) {
            callback(false, nullptr);
        }
        return;
    }

    task.timestamp = static_cast<u64>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    auto internalTask = std::make_shared<InternalTask>();
    internalTask->task = std::move(task);
    internalTask->generator = std::move(generator);
    internalTask->callback = std::move(callback);

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(internalTask));
    }

    m_condition.notify_one();
}

// ============================================================================
// 统计
// ============================================================================

size_t ChunkWorkerPool::pendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

// ============================================================================
// Worker 线程
// ============================================================================

void ChunkWorkerPool::workerThread(i32 workerId)
{
    // 设置线程名称
    std::string threadName = "ChunkGenWorker-" + std::to_string(workerId);
    mc::perfetto::PerfettoManager::instance().setThreadName(threadName);

    while (true) {
        std::shared_ptr<InternalTask> taskCopy;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // 等待任务或停止信号
            m_condition.wait(lock, [this] {
                return !m_taskQueue.empty() || m_stop.load(std::memory_order_acquire);
            });

            if (m_taskQueue.empty() && m_stop.load(std::memory_order_acquire)) {
                return;
            }

            if (!m_taskQueue.empty()) {
                taskCopy = m_taskQueue.top();
                m_taskQueue.pop();
            }
        }

        // 执行任务
        if (taskCopy) {
            executeTask(*taskCopy);
        }
    }
}

void ChunkWorkerPool::executeTask(InternalTask& task)
{
    MC_TRACE_CHUNK_GEN_EVENT("GenerateChunk");

    // 创建区块生成器
    auto primer = std::make_unique<ChunkPrimer>(task.task.x, task.task.z);

    bool success = true;

    try {
        // 执行生成
        if (task.generator && task.task.targetStatus) {
            task.generator(*primer, *task.task.targetStatus);
        }
    } catch (const std::exception&) {
        success = false;
    } catch (...) {
        success = false;
    }

    ChunkPrimer* callbackChunk = nullptr;

    if (success) {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedChunks.push_back(std::move(primer));
        callbackChunk = m_completedChunks.back().get();
    }

    std::vector<CompletionCallback> callbacks;

    if (task.coalescedCallbacks) {
        {
            std::lock_guard<std::mutex> lock(task.coalescedCallbacks->mutex);
            callbacks = task.coalescedCallbacks->callbacks;
        }

        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_pendingGenerateTasks.erase(task.coalesceKey);
    } else if (task.callback) {
        callbacks.push_back(task.callback);
    }

    if (!callbacks.empty()) {
        for (const auto& callback : callbacks) {
            if (callback) {
                callback(success, callbackChunk);
            }
        }
    }
}

} // namespace mc::server
