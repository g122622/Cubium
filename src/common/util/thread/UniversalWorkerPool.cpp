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

#include "UniversalWorkerPool.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/thread/ITask.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::util {

// ============================================================================
// 构造与析构
// ============================================================================

UniversalWorkerPool::UniversalWorkerPool(i32 threadCount, std::string name, i32 rankBase)
    : m_poolName(std::move(name))
    , m_threadCount(threadCount > 0 ? threadCount : _getOptimalThreadCount())
    , m_rankBase(rankBase)
{}

UniversalWorkerPool::~UniversalWorkerPool()
{
    try {
        shutdown();
    }
    catch (...) {
        // 线程回收失败时禁止继续销毁线程池，保持析构的终止语义。
        spdlog::critical("[UniversalWorkerPool] Shutdown failed during destruction");
        std::terminate();
    }
}

i32 UniversalWorkerPool::_getOptimalThreadCount()
{
    // 使用硬件并发数 - 1，至少 1 个，最多 114514 个
    const u32 hardwareConcurrency = std::thread::hardware_concurrency();
    return static_cast<i32>(std::clamp(hardwareConcurrency - 1, 1u, 114514u));
}

// ============================================================================
// 生命周期
// ============================================================================

void UniversalWorkerPool::start()
{
    if (m_running.exchange(true, std::memory_order::acq_rel)) {
        return; // 已经在运行
    }

    m_stop.store(false, std::memory_order::release);

    // 创建工作线程
    m_workers.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&UniversalWorkerPool::_workerThread, this, i);
    }

    spdlog::info("[UniversalWorkerPool] Started {} worker threads (name: {})", m_threadCount, m_poolName);
}

void UniversalWorkerPool::shutdown()
{
    if (!m_running.exchange(false, std::memory_order::acq_rel)) {
        return;
    }
    std::vector<std::shared_ptr<InternalTask>> dropped;
    {
        std::lock_guard lock(m_queueMutex);
        m_stop.store(true, std::memory_order::release);
        while (!m_taskQueue.empty()) {
            dropped.push_back(m_taskQueue.top());
            m_taskQueue.pop();
        }
    }
    m_condition.notify_all();
    // 先通知取消，再等待 worker；运行任务可能依赖这些通知才能结束。
    for (const auto& task : dropped) {
        _cancelTask(task);
        _releaseOutstandingTask();
    }
    dropped.clear();
    _notifyIfIdle();
    for (auto& worker : m_workers) {
        worker.join();
    }
    m_workers.clear();
    spdlog::info("[UniversalWorkerPool] Shutdown complete (name: {})", m_poolName);
}

// ============================================================================
// 任务提交
// ============================================================================

u64 UniversalWorkerPool::submit(std::unique_ptr<ITask> task,
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
    auto internal = std::make_shared<InternalTask>();
    internal->task = std::move(task);
    internal->callback = std::move(callback);
    internal->priority = priority;
    internal->abortSignal = std::move(abortSignal);
    {
        std::lock_guard lock(m_queueMutex);
        // 接纳任务与关闭清队列互斥，不能在 shutdown 排空后再提交。
        if (m_running.load(std::memory_order::acquire) && !m_stop.load(std::memory_order::acquire)) {
            internal->id = m_nextTaskId.fetch_add(1, std::memory_order::relaxed);
            m_outstandingTaskCount.fetch_add(1, std::memory_order::relaxed);
            m_taskQueue.push(internal);
            m_condition.notify_one();
            return internal->id;
        }
    }
    spdlog::warn("UniversalWorkerPool[{}]: task submitted while pool is stopped; task dropped", m_poolName);
    if (internal->callback) {
        internal->callback(false, nullptr);
    }
    return 0;
}

bool UniversalWorkerPool::cancel(u64 taskId)
{
    std::shared_ptr<InternalTask> cancelled;
    {
        std::lock_guard lock(m_queueMutex);
        std::vector<std::shared_ptr<InternalTask>> retained;
        while (!m_taskQueue.empty()) {
            auto task = m_taskQueue.top();
            m_taskQueue.pop();
            if (task->id == taskId) {
                cancelled = task;
                if (task->abortSignal) {
                    task->abortSignal->store(true, std::memory_order::release);
                }
            } else {
                retained.push_back(std::move(task));
            }
        }
        for (auto& task : retained) {
            m_taskQueue.push(std::move(task));
        }
    }
    if (!cancelled) {
        return false;
    }
    _cancelTask(cancelled);
    _releaseOutstandingTask();
    _notifyIfIdle();
    return true;
}

void UniversalWorkerPool::pruneCancelledTasks()
{
    std::vector<std::shared_ptr<InternalTask>> cancelled;
    {
        std::lock_guard lock(m_queueMutex);
        std::vector<std::shared_ptr<InternalTask>> retained;
        while (!m_taskQueue.empty()) {
            auto task = m_taskQueue.top();
            m_taskQueue.pop();
            if (_isTaskCancelled(*task)) {
                cancelled.push_back(std::move(task));
            } else {
                retained.push_back(std::move(task));
            }
        }
        for (auto& task : retained) {
            m_taskQueue.push(std::move(task));
        }
    }
    for (const auto& task : cancelled) {
        _cancelTask(task);
        _releaseOutstandingTask();
    }
    _notifyIfIdle();
}

void UniversalWorkerPool::_cancelTask(const std::shared_ptr<InternalTask>& task)
{
    try {
        task->task->onCancel();
    }
    catch (const std::exception& e) {
        spdlog::error("[UniversalWorkerPool] Cancellation threw: {}", e.what());
    }
    catch (...) {
        spdlog::error("[UniversalWorkerPool] Cancellation threw an unknown exception");
    }
    _callback(task, false);
}

void UniversalWorkerPool::_callback(const std::shared_ptr<InternalTask>& task, bool success)
{
    if (!task->callback) {
        return;
    }
    try {
        MC_TRACE_SCOPED_EVENT(TraceEvents.WorkerPool.Generic, "TaskCallback", "description", task->task->description());
        task->callback(success, task->task.get());
    }
    catch (const std::exception& e) {
        spdlog::error("[UniversalWorkerPool] Callback threw: {}", e.what());
    }
    catch (...) {
        spdlog::error("[UniversalWorkerPool] Callback threw an unknown exception");
    }
}

void UniversalWorkerPool::waitForCompletion()
{
    // 谓词用 m_outstandingTaskCount（覆盖"已入队 / 执行中 / 回调执行中"三种状态）。
    // 不能用 pendingTaskCount()==0 && runningTaskCount()==0：存在两个提前返回窗口——
    //   1) 回调在 m_runningTaskCount 递减之后才执行；
    //   2) worker 出队与标记运行之间，队列已空且运行数为 0，任务却尚未开始执行。
    std::unique_lock<std::mutex> lock(m_completionMutex);
    m_completionCondition.wait(lock, [this] {
        return m_outstandingTaskCount.load(std::memory_order::acquire) == 0;
    });
}

void UniversalWorkerPool::_releaseOutstandingTask()
{
    const size_t before = m_outstandingTaskCount.fetch_sub(1, std::memory_order::release);
    // 下溢即"递减次数多于入队次数"，会让计数器回绕到 SIZE_MAX 并使 waitForCompletion() 永久挂起。
    // 这种失衡是代码缺陷而非运行时输入问题，直接断言暴露，避免退化成难以定位的挂起。
    MC_ASSERT_RELEASE_MSG(before > 0, "UniversalWorkerPool: outstanding task count underflow");
}

void UniversalWorkerPool::_notifyIfIdle()
{
    // fast-path:仍有未完成任务则必非空闲,直接返回不抢锁(任务执行期间 worker 高频调用此点,
    // 避免无谓的 m_completionMutex 竞争)。
    if (m_outstandingTaskCount.load(std::memory_order::acquire) != 0) {
        return;
    }
    // 持 m_completionMutex 重检谓词并 notify,与 waitForCompletion 的"谓词检查+wait"串行化,
    // 杜绝 worker 在等待方 pred()=false 与 release+block 之间发 notify 致丢失唤醒。
    std::lock_guard<std::mutex> lock(m_completionMutex);
    if (m_outstandingTaskCount.load(std::memory_order::acquire) == 0) {
        m_completionCondition.notify_all();
    }
}

// ============================================================================
// 统计
// ============================================================================

size_t UniversalWorkerPool::pendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

size_t UniversalWorkerPool::runningTaskCount() const
{
    return m_runningTaskCount.load(std::memory_order::acquire);
}

void UniversalWorkerPool::debugDumpState()
{
    spdlog::info("[workerPool-debug] pending={} running={}", pendingTaskCount(), runningTaskCount());
}

void UniversalWorkerPool::debugDumpRunningTasks()
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
        spdlog::info("[workerPool-running] worker={} elapsedMs={} desc={}", wid, elapsedMs, info.description);
    }
}

// ============================================================================
// 工作线程
// ============================================================================

void UniversalWorkerPool::_workerThread(i32 workerId)
{
    const std::string threadName = m_poolName + "-" + std::to_string(workerId);
    mc::profiler::ProfilerManager::instance().setThreadName(threadName, m_rankBase + workerId);
    while (true) {
        std::shared_ptr<InternalTask> task;
        {
            std::unique_lock lock(m_queueMutex);
            m_condition.wait(lock, [this] {
                return !m_taskQueue.empty() || m_stop.load(std::memory_order::acquire);
            });
            if (m_stop.load(std::memory_order::acquire)) {
                return;
            }
            task = m_taskQueue.top();
            m_taskQueue.pop();
        }
        {
            std::lock_guard lock(m_runningTaskMutex);
            m_runningTaskInfo[workerId] = {task->task->description(), std::chrono::steady_clock::now()};
        }
        _executeTask(task);
        {
            std::lock_guard lock(m_runningTaskMutex);
            m_runningTaskInfo.erase(workerId);
        }
        _releaseOutstandingTask();
        _notifyIfIdle();
    }
}

void UniversalWorkerPool::_executeTask(const std::shared_ptr<InternalTask>& task)
{
    if (_isTaskCancelled(*task)) {
        _cancelTask(task);
        return;
    }

    m_runningTaskCount.fetch_add(1, std::memory_order::relaxed);

    bool success = false;

    try {
        // 追踪事件须置于 try 块内：Perfetto/Tracy 的 RAII zone 在构造/析构时可能抛 C++ 异常
        // （例如未初始化 backend 下访问全局注册表），若在 try 外抛出会逃逸 catch(...) 致
        // std::terminate → 进程崩溃。整段任务执行（含追踪）包进 try/catch 是 worker 健壮性兜底。
        static const std::atomic<bool> neverAbort{false};
        const std::atomic<bool>& abortSignal = task->abortSignal ? *task->abortSignal : neverAbort;

        MC_TRACE_SCOPED_EVENT(TraceEvents.WorkerPool.Generic,
            "ExecuteTask",
            "type",
            static_cast<u8>(task->task->type()),
            "description",
            task->task->description(),
            "priority",
            static_cast<i8>(task->priority));

        MC_TRACE_SCOPED_EVENT(
            TraceEvents.WorkerPool.Generic, "TaskExecution", "description", task->task->description());
        success = task->task->execute(abortSignal);
    }
    catch (const std::exception& e) {
        spdlog::error("[UniversalWorkerPool] Task {} threw exception: {}", task->task->description(), e.what());
        success = false;
    }
    catch (...) {
        spdlog::error("[UniversalWorkerPool] Task {} threw unknown exception", task->task->description());
        success = false;
    }

    // 成功完成的任务只通知成功，执行期间置位的令牌不反转已完成结果。
    if (!success && _isTaskCancelled(*task)) {
        _cancelTask(task);
    } else {
        _callback(task, success);
    }
    m_runningTaskCount.fetch_sub(1, std::memory_order::relaxed);
}

bool UniversalWorkerPool::_isTaskCancelled(const InternalTask& task)
{
    if (!task.abortSignal) {
        return false;
    }
    return task.abortSignal->load(std::memory_order::acquire);
}

} // namespace mc::util
