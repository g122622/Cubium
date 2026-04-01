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
    submitGenerate(x, z, targetStatus, std::move(callback), nullptr, priority);
}

void ChunkWorkerPool::submitGenerate(ChunkCoord x, ChunkCoord z,
                                       const ChunkStatus& targetStatus,
                                       CompletionCallback callback,
                                       std::shared_ptr<std::atomic<bool>> cancelToken,
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

    auto internalTask = std::make_shared<InternalTask>();
    internalTask->task = std::move(chunkTask);
    internalTask->generator = m_generator;
    internalTask->callback = std::move(callback);
    internalTask->cancelToken = std::move(cancelToken);

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(internalTask));
    }

    m_condition.notify_one();
}

void ChunkWorkerPool::submitTask(ChunkTask task,
                                   GeneratorFunc generator,
                                   CompletionCallback callback)
{
    submitTask(std::move(task), std::move(generator), std::move(callback), nullptr);
}

void ChunkWorkerPool::submitTask(ChunkTask task,
                                   GeneratorFunc generator,
                                   CompletionCallback callback,
                                   std::shared_ptr<std::atomic<bool>> cancelToken)
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
    internalTask->cancelToken = std::move(cancelToken);

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

void ChunkWorkerPool::pruneCancelledTasks()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    pruneCancelledTasksLocked();
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
    if (isTaskCancelled(task)) {
        if (task.callback) {
            task.callback(false, nullptr);
        }
        return;
    }

    MC_TRACE_CHUNK_GEN_EVENT("GenerateChunk");

    // 创建区块生成器
    auto primer = std::make_unique<ChunkPrimer>(task.task.x, task.task.z);

    bool success = true;

    try {
        // 执行生成
        if (task.generator && task.task.targetStatus) {
            static const std::atomic<bool> neverCancel{false};
            const std::atomic<bool>& cancelSignal = task.cancelToken
                ? *task.cancelToken
                : neverCancel;
            task.generator(*primer, *task.task.targetStatus, cancelSignal);
        }
    } catch (const std::exception&) {
        success = false;
    } catch (...) {
        success = false;
    }

    if (isTaskCancelled(task)) {
        success = false;
    }

    ChunkPrimer* callbackChunk = nullptr;

    if (success) {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedChunks.push_back(std::move(primer));
        callbackChunk = m_completedChunks.back().get();
    }

    if (task.callback) {
        task.callback(success, callbackChunk);
    }
}

bool ChunkWorkerPool::isTaskCancelled(const InternalTask& task)
{
    if (!task.cancelToken) {
        return false;
    }
    return task.cancelToken->load(std::memory_order_acquire);
}

void ChunkWorkerPool::pruneCancelledTasksLocked()
{
    if (m_taskQueue.empty()) {
        return;
    }

    std::vector<std::shared_ptr<InternalTask>> retained;
    retained.reserve(m_taskQueue.size());

    while (!m_taskQueue.empty()) {
        auto task = m_taskQueue.top();
        m_taskQueue.pop();
        if (task && !isTaskCancelled(*task)) {
            retained.push_back(std::move(task));
        }
    }

    for (auto& task : retained) {
        m_taskQueue.push(std::move(task));
    }
}

} // namespace mc::server
