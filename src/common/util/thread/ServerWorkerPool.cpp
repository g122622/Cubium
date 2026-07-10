/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ServerWorkerPool.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>
#include <chrono>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::util {

// ============================================================================
// 构造与析构
// ============================================================================

ServerWorkerPool::ServerWorkerPool(i32 threadCount, std::string name)
    : m_poolName(std::move(name))
    , m_threadCount(threadCount > 0 ? threadCount : getOptimalThreadCount())
{}

ServerWorkerPool::~ServerWorkerPool()
{
    shutdown();
}

i32 ServerWorkerPool::getOptimalThreadCount()
{
    // 使用硬件并发数的一半，至少 1 个，最多 114514 个
    const unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    return static_cast<i32>(std::clamp(hardwareConcurrency / 2, 1u, 114514u));
}

// ============================================================================
// 生命周期
// ============================================================================

void ServerWorkerPool::start()
{
    if (m_running.exchange(true, std::memory_order::acq_rel)) {
        return; // 已经在运行
    }

    m_stop.store(false, std::memory_order::release);

    // 创建工作线程
    m_workers.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&ServerWorkerPool::workerThread, this, i);
    }

    spdlog::info("[ServerWorkerPool] Started {} worker threads (name: {})", m_threadCount, m_poolName);
}

void ServerWorkerPool::shutdown()
{
    if (!m_running.exchange(false, std::memory_order::acq_rel)) {
        return; // 已经停止
    }

    m_stop.store(true, std::memory_order::release);

    // 唤醒所有等待的线程
    m_condition.notify_all();

    // 等待所有线程结束
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();

    // 清空任务队列
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_taskQueue.empty()) {
            auto task = m_taskQueue.top();
            m_taskQueue.pop();
            if (task && task->callback) {
                task->callback(false, task->task.get()); // 通知失败
            }
        }
    }

    spdlog::info("[ServerWorkerPool] Shutdown complete (name: {})", m_poolName);
}

// ============================================================================
// 任务提交
// ============================================================================

u64 ServerWorkerPool::submit(std::unique_ptr<ITask> task,
    TaskCallback callback,
    TaskPriority priority,
    std::shared_ptr<std::atomic<bool>> abortSignal)
{
    if (!task) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    if (!m_running.load(std::memory_order::acquire)) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    const u64 taskId = m_nextTaskId.fetch_add(1, std::memory_order::relaxed);
    const u64 timestamp = static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count());

    auto internalTask = std::make_unique<InternalTask>();
    internalTask->id = taskId;
    internalTask->priority = priority;
    internalTask->timestamp = timestamp;
    internalTask->task = std::move(task);
    internalTask->callback = std::move(callback);
    internalTask->abortSignal = std::move(abortSignal);

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(internalTask));
    }

    m_condition.notify_one();
    return taskId;
}

u64 ServerWorkerPool::submit(std::unique_ptr<ITask> task,
    TaskCallback callback,
    ChunkCoord centerX,
    ChunkCoord centerZ,
    i32 writeRadius,
    TaskPriority priority,
    std::shared_ptr<std::atomic<bool>> abortSignal)
{
    if (!task) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    if (!m_running.load(std::memory_order::acquire)) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    const u64 taskId = m_nextTaskId.fetch_add(1, std::memory_order::relaxed);
    const u64 timestamp = static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count());

    auto internalTask = std::make_unique<InternalTask>();
    internalTask->id = taskId;
    internalTask->priority = priority;
    internalTask->timestamp = timestamp;
    internalTask->task = std::move(task);
    internalTask->callback = std::move(callback);
    internalTask->abortSignal = std::move(abortSignal);
    internalTask->hasArea = true;
    internalTask->areaCenterX = centerX;
    internalTask->areaCenterZ = centerZ;
    internalTask->areaWriteRadius = writeRadius < 0 ? 0 : writeRadius;

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(internalTask));
    }

    m_condition.notify_one();
    return taskId;
}

bool ServerWorkerPool::canExecuteNow(ChunkCoord centerX, ChunkCoord centerZ, i32 writeRadius) const
{
    if (writeRadius < 0) {
        writeRadius = 0;
    }
    std::lock_guard<std::mutex> lock(m_runningRegionsMutex);
    const i32 r = writeRadius;
    for (i32 dx = -r; dx <= r; ++dx) {
        for (i32 dz = -r; dz <= r; ++dz) {
            if (m_runningRegions.count(packChunkKey(centerX + dx, centerZ + dz)) > 0) {
                return false;
            }
        }
    }
    return true;
}

// ============================================================================
// 任务管理
// ============================================================================

bool ServerWorkerPool::cancel(u64 taskId)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    // 遍历队列查找任务
    std::vector<std::shared_ptr<InternalTask>> temp;
    bool found = false;

    while (!m_taskQueue.empty()) {
        auto task = m_taskQueue.top();
        m_taskQueue.pop();

        if (task && task->id == taskId) {
            // 找到任务，设置取消标志
            if (task->abortSignal) {
                task->abortSignal->store(true, std::memory_order::release);
            }
            if (task->callback) {
                task->callback(false, task->task.get());
            }
            found = true;
            // 不放回队列
        } else {
            temp.push_back(std::move(task));
        }
    }

    // 放回队列
    for (auto& task : temp) {
        m_taskQueue.push(std::move(task));
    }

    return found;
}

void ServerWorkerPool::pruneCancelledTasks()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

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
        } else if (task && task->callback) {
            task->callback(false, task->task.get()); // 通知取消
        }
    }

    for (auto& task : retained) {
        m_taskQueue.push(std::move(task));
    }
}

void ServerWorkerPool::waitForCompletion()
{
    std::unique_lock<std::mutex> lock(m_completionMutex);
    m_completionCondition.wait(lock, [this] { return pendingTaskCount() == 0 && runningTaskCount() == 0; });
}

// ============================================================================
// 统计
// ============================================================================

size_t ServerWorkerPool::pendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

size_t ServerWorkerPool::runningTaskCount() const
{
    return m_runningTaskCount.load(std::memory_order::acquire);
}

void ServerWorkerPool::debugDumpState()
{
    size_t queueSize = 0;
    size_t areaTaskCount = 0;
    size_t cancelledCount = 0;
    size_t nonAreaCount = 0;
    std::unordered_map<i32, size_t> writeRadiusHistogram;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        // 优先队列不支持遍历，用临时 vector 取出再放回
        std::vector<std::shared_ptr<InternalTask>> tmp;
        tmp.reserve(m_taskQueue.size());
        while (!m_taskQueue.empty()) {
            auto t = m_taskQueue.top();
            m_taskQueue.pop();
            tmp.push_back(t);
            ++queueSize;
            if (t->hasArea) {
                ++areaTaskCount;
                writeRadiusHistogram[t->areaWriteRadius]++;
            } else {
                ++nonAreaCount;
            }
            if (t->abortSignal && t->abortSignal->load(std::memory_order::acquire)) {
                ++cancelledCount;
            }
        }
        for (auto& t : tmp) {
            m_taskQueue.push(t);
        }
    }
    size_t runningRegionsCount = 0;
    {
        std::lock_guard<std::mutex> lock(m_runningRegionsMutex);
        runningRegionsCount = m_runningRegions.size();
    }
    spdlog::info("[workerPool-debug] queueSize={}, areaTaskCount={}, nonAreaCount={}, cancelledInQueue={}, "
                 "runningRegions={}, runningTaskCount={}",
        queueSize,
        areaTaskCount,
        nonAreaCount,
        cancelledCount,
        runningRegionsCount,
        m_runningTaskCount.load(std::memory_order::acquire));
    for (const auto& [wr, cnt] : writeRadiusHistogram) {
        spdlog::info("[workerPool-debug]   writeRadius={} count={}", wr, cnt);
    }
}

void ServerWorkerPool::debugDumpRunningTasks()
{
    std::vector<std::pair<i32, RunningTaskInfo>> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_runningTaskMutex);
        for (const auto& [wid, info] : m_runningTaskInfo) {
            snapshot.emplace_back(wid, info);
        }
    }
    const auto now = std::chrono::steady_clock::now();
    if (snapshot.empty()) {
        spdlog::info("[workerPool-running] no task currently executing");
        return;
    }
    for (const auto& [wid, info] : snapshot) {
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.startTime).count();
        spdlog::info("[workerPool-running] worker={} elapsedMs={} hasArea={} area=({},{},r={}) desc={}",
            wid,
            elapsedMs,
            info.hasArea,
            info.areaCenterX,
            info.areaCenterZ,
            info.areaWriteRadius,
            info.description);
    }
}

// ============================================================================
// 工作线程
// ============================================================================

void ServerWorkerPool::workerThread(i32 workerId)
{
    // 设置线程名称
    std::string threadName = m_poolName + "-" + std::to_string(workerId);
    // sibling_order_rank = rankBase + workerId，让 worker-0 排最前（根 track thread_ordering=EXPLICIT 生效）。
    // 按 pool 名区分 rankBase，使 UI 中三组 worker 分块排列、组内按 workerId 升序：
    //   ServerCompute(100+) -> ServerIO(200+) -> ChunkMeshWorker(300+，见 MeshWorkerPool)。
    // 每组间隔 100，避免线程数 >10 时跨组相交。
    const int rankBase = (m_poolName == "ServerCompute") ? 100 : 200;
    mc::profiler::ProfilerManager::instance().setThreadName(threadName, rankBase + workerId);

    while (true) {
        std::shared_ptr<InternalTask> taskCopy;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // 等待任务或停止信号
            m_condition.wait(lock, [this] { return !m_taskQueue.empty() || m_stop.load(std::memory_order::acquire); });

            if (m_taskQueue.empty() && m_stop.load(std::memory_order::acquire)) {
                return;
            }

            if (!m_taskQueue.empty()) {
                taskCopy = m_taskQueue.top();
                m_taskQueue.pop();
            }
        }

        // 执行任务
        if (taskCopy) {
            // 取消检查：被取消的任务（abortSignal=true）在入队期间被 cancelActiveWork 标记，
            // 必须尽快走取消路径（onCancel → callback(false)）清理依赖图（cancelGeneration/clearGenerationTask/
            // releaseNeighbourRefCounts），绝不能进入冲突重试循环（否则永远无法到达 executeTask 的取消检查，
            // m_generationTask 永不清除 → isSafeToUnload 永假 → holder 永久卡住 → 测试挂起）。
            if (isTaskCancelled(*taskCopy)) {
                executeTask(std::move(taskCopy)); // executeTask 开头检测取消 → onCancel + callback(false)
                if (pendingTaskCount() == 0 && runningTaskCount() == 0) {
                    m_completionCondition.notify_all();
                }
                continue;
            }

            // 区域互斥任务：执行前检查写入区域是否与正在执行的区域任务冲突。
            // 冲突时把任务放回队列，等待某个区域任务释放后重试。
            if (taskCopy->hasArea) {
                bool conflict = false;
                {
                    std::lock_guard<std::mutex> areaLock(m_runningRegionsMutex);
                    conflict = hasAreaConflictLocked(*taskCopy);
                    if (!conflict) {
                        // 无冲突：立即标记区域为正在执行
                        markAreaRunningLocked(*taskCopy);
                    }
                }

                if (conflict) {
                    // 放回队列，等待区域释放通知或短暂超时后重试
                    {
                        std::lock_guard<std::mutex> lock(m_queueMutex);
                        m_taskQueue.push(taskCopy); // 保留 shared_ptr，不 move
                    }
                    // 通知一个工作线程重试（可能是自己，也可能是其他线程）
                    m_condition.notify_one();
                    // 等待区域释放（带 1ms 超时避免永久阻塞：防止通知丢失导致活锁）
                    {
                        std::unique_lock<std::mutex> areaLock(m_runningRegionsMutex);
                        m_areaReleasedCondition.wait_for(areaLock, std::chrono::milliseconds(1));
                    }
                    continue;
                }

                // 无冲突且已标记区域：执行任务
                // 保存区域信息（executeTask 会 move taskCopy，之后无法访问）
                const ChunkCoord areaX = taskCopy->areaCenterX;
                const ChunkCoord areaZ = taskCopy->areaCenterZ;
                const i32 areaR = taskCopy->areaWriteRadius;

                {
                    std::lock_guard<std::mutex> rlock(m_runningTaskMutex);
                    auto& info = m_runningTaskInfo[workerId];
                    info.description = taskCopy->task ? taskCopy->task->description() : std::string();
                    info.startTime = std::chrono::steady_clock::now();
                    info.hasArea = true;
                    info.areaCenterX = areaX;
                    info.areaCenterZ = areaZ;
                    info.areaWriteRadius = areaR;
                }
                executeTask(std::move(taskCopy));
                {
                    std::lock_guard<std::mutex> rlock(m_runningTaskMutex);
                    m_runningTaskInfo.erase(workerId);
                }

                // 执行完成：清除区域标记并通知等待冲突的工作线程
                {
                    std::lock_guard<std::mutex> areaLock(m_runningRegionsMutex);
                    InternalTask tmp;
                    tmp.areaCenterX = areaX;
                    tmp.areaCenterZ = areaZ;
                    tmp.areaWriteRadius = areaR;
                    unmarkAreaRunningLocked(tmp);
                }
                m_areaReleasedCondition.notify_all();
            } else {
                // 无区域互斥任务：直接执行
                {
                    std::lock_guard<std::mutex> rlock(m_runningTaskMutex);
                    auto& info = m_runningTaskInfo[workerId];
                    info.description = taskCopy->task ? taskCopy->task->description() : std::string();
                    info.startTime = std::chrono::steady_clock::now();
                    info.hasArea = false;
                    info.areaCenterX = 0;
                    info.areaCenterZ = 0;
                    info.areaWriteRadius = 0;
                }
                executeTask(std::move(taskCopy));
                {
                    std::lock_guard<std::mutex> rlock(m_runningTaskMutex);
                    m_runningTaskInfo.erase(workerId);
                }
            }
        }

        // 检查是否所有任务都完成了
        if (pendingTaskCount() == 0 && runningTaskCount() == 0) {
            m_completionCondition.notify_all();
        }
    }
}

void ServerWorkerPool::executeTask(std::shared_ptr<InternalTask> task)
{
    if (!task || !task->task) {
        return;
    }

    // 检查取消
    if (isTaskCancelled(*task)) {
        task->task->onCancel();
        if (task->callback) {
            task->callback(false, task->task.get());
        }
        return;
    }

    m_runningTaskCount.fetch_add(1, std::memory_order::relaxed);

    // 保存原始任务指针用于回调
    ITask* taskPtr = task->task.get();

    // 追踪事件
    MC_TRACE_SCOPED_EVENT(TraceEvents.WorkerPool.Generic,
        "ExecuteTask",
        "type",
        static_cast<u8>(task->task->type()),
        "description",
        task->task->description(),
        "priority",
        static_cast<i8>(task->priority));

    bool success = false;

    try {
        static const std::atomic<bool> neverAbort{false};
        const std::atomic<bool>& abortSignal = task->abortSignal ? *task->abortSignal : neverAbort;

        MC_TRACE_SCOPED_EVENT(
            TraceEvents.WorkerPool.Generic, "TaskExecution", "description", task->task->description());
        success = task->task->execute(abortSignal);
    }
    catch (const std::exception& e) {
        spdlog::error("[ServerWorkerPool] Task {} threw exception: {}", task->task->description(), e.what());
        success = false;
    }
    catch (...) {
        spdlog::error("[ServerWorkerPool] Task {} threw unknown exception", task->task->description());
        success = false;
    }

    // 执行后取消检查：仅在任务未成功完成时才走取消路径。
    // 若 execute 返回 true（成功），任务已在 execute 内调用 onChunkGenComplete 完成状态推进、
    // 清除 m_generationTask 并可能自重调度新任务。此时即使 abortSignal 在执行期间被 cancelActiveWork
    // 置位，也不应调用 onCancel——否则会取消自重调度的新任务（m_generationTask 已被新任务覆盖），
    // 导致状态机错乱（新任务被误取消、依赖图被错误清理）。取消语义只在任务尚未完成时生效。
    if (!success && isTaskCancelled(*task)) {
        task->task->onCancel();
        success = false;
    }

    m_runningTaskCount.fetch_sub(1, std::memory_order::relaxed);

    // 回调
    if (task->callback) {
        try {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.WorkerPool.Generic, "TaskCallback", "description", task->task->description());
            task->callback(success, taskPtr);
        }
        catch (const std::exception& e) {
            spdlog::error("[ServerWorkerPool] Callback threw exception: {}", e.what());
        }
        catch (...) {
            spdlog::error("[ServerWorkerPool] Callback threw unknown exception");
        }
    }
}

bool ServerWorkerPool::isTaskCancelled(const InternalTask& task)
{
    if (!task.abortSignal) {
        return false;
    }
    return task.abortSignal->load(std::memory_order::acquire);
}

// ============================================================================
// 区域互斥（对齐 Moonrise 区域锁执行器）
// ============================================================================

u64 ServerWorkerPool::packChunkKey(ChunkCoord x, ChunkCoord z) noexcept
{
    // 高 32 位 X，低 32 位 Z（都转为 u32 以保留位模式）
    return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u64>(static_cast<u32>(z));
}

bool ServerWorkerPool::hasAreaConflictLocked(const InternalTask& task) const
{
    // 调用者必须持有 m_runningRegionsMutex
    const i32 r = task.areaWriteRadius;
    for (i32 dx = -r; dx <= r; ++dx) {
        for (i32 dz = -r; dz <= r; ++dz) {
            if (m_runningRegions.count(packChunkKey(task.areaCenterX + dx, task.areaCenterZ + dz)) > 0) {
                return true;
            }
        }
    }
    return false;
}

void ServerWorkerPool::markAreaRunningLocked(const InternalTask& task)
{
    // 调用者必须持有 m_runningRegionsMutex
    const i32 r = task.areaWriteRadius;
    for (i32 dx = -r; dx <= r; ++dx) {
        for (i32 dz = -r; dz <= r; ++dz) {
            m_runningRegions.insert(packChunkKey(task.areaCenterX + dx, task.areaCenterZ + dz));
        }
    }
}

void ServerWorkerPool::unmarkAreaRunningLocked(const InternalTask& task)
{
    // 调用者必须持有 m_runningRegionsMutex
    const i32 r = task.areaWriteRadius;
    for (i32 dx = -r; dx <= r; ++dx) {
        for (i32 dz = -r; dz <= r; ++dz) {
            m_runningRegions.erase(packChunkKey(task.areaCenterX + dx, task.areaCenterZ + dz));
        }
    }
}

std::vector<u64> ServerWorkerPool::computeAreaKeys(const InternalTask& task)
{
    const i32 r = task.areaWriteRadius;
    std::vector<u64> keys;
    keys.reserve(static_cast<size_t>((2 * r + 1) * (2 * r + 1)));
    for (i32 dx = -r; dx <= r; ++dx) {
        for (i32 dz = -r; dz <= r; ++dz) {
            keys.push_back(packChunkKey(task.areaCenterX + dx, task.areaCenterZ + dz));
        }
    }
    return keys;
}

} // namespace mc::util
