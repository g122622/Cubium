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
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <chrono>
#include <spdlog/spdlog.h>

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
    if (m_running.exchange(true, std::memory_order_acq_rel)) {
        return; // 已经在运行
    }

    m_stop.store(false, std::memory_order_release);

    // 创建工作线程
    m_workers.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&ServerWorkerPool::workerThread, this, i);
    }

    spdlog::info("[ServerWorkerPool] Started {} worker threads (name: {})", m_threadCount, m_poolName);
}

void ServerWorkerPool::shutdown()
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
    std::shared_ptr<std::atomic<bool>> cancelToken)
{
    if (!task) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    if (!m_running.load(std::memory_order_acquire)) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    const u64 taskId = m_nextTaskId.fetch_add(1, std::memory_order_relaxed);
    const u64 timestamp = static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count());

    auto internalTask = std::make_unique<InternalTask>();
    internalTask->id = taskId;
    internalTask->priority = priority;
    internalTask->timestamp = timestamp;
    internalTask->task = std::move(task);
    internalTask->callback = std::move(callback);
    internalTask->cancelToken = std::move(cancelToken);

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(internalTask));
    }

    m_condition.notify_one();
    return taskId;
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
            if (task->cancelToken) {
                task->cancelToken->store(true, std::memory_order_release);
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
    return m_runningTaskCount.load(std::memory_order_acquire);
}

// ============================================================================
// 工作线程
// ============================================================================

void ServerWorkerPool::workerThread(i32 workerId)
{
    // 设置线程名称
    std::string threadName = m_poolName + "-" + std::to_string(workerId);
    mc::perfetto::PerfettoManager::instance().setThreadName(threadName);

    while (true) {
        std::shared_ptr<InternalTask> taskCopy;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // 等待任务或停止信号
            m_condition.wait(lock, [this] { return !m_taskQueue.empty() || m_stop.load(std::memory_order_acquire); });

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
            executeTask(std::move(taskCopy));
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

    m_runningTaskCount.fetch_add(1, std::memory_order_relaxed);

    // 保存原始任务指针用于回调
    ITask* taskPtr = task->task.get();

    // 追踪事件
    MC_TRACE_EVENT("worker_pool",
        "ExecuteTask",
        "type",
        static_cast<u8>(task->task->type()),
        "description",
        task->task->description(),
        "priority",
        static_cast<i8>(task->priority));

    bool success = false;

    try {
        static const std::atomic<bool> neverCancel{false};
        const std::atomic<bool>& cancelSignal = task->cancelToken ? *task->cancelToken : neverCancel;

        success = task->task->execute(cancelSignal);
    }
    catch (const std::exception& e) {
        spdlog::error("[ServerWorkerPool] Task {} threw exception: {}", task->task->description(), e.what());
        success = false;
    }
    catch (...) {
        spdlog::error("[ServerWorkerPool] Task {} threw unknown exception", task->task->description());
        success = false;
    }

    // 再次检查取消
    if (isTaskCancelled(*task)) {
        task->task->onCancel();
        success = false;
    }

    m_runningTaskCount.fetch_sub(1, std::memory_order_relaxed);

    // 回调
    if (task->callback) {
        try {
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
    if (!task.cancelToken) {
        return false;
    }
    return task.cancelToken->load(std::memory_order_acquire);
}

} // namespace mc::util
