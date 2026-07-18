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

#include "MeshWorkerPool.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>
#include <string>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

MeshWorkerPool::MeshWorkerPool(i32 threadCount)
    : m_threadCount(threadCount > 0 ? threadCount : _getOptimalThreadCount())
{
    m_recycleBuckets.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_recycleBuckets.emplace_back(std::make_unique<RecycleBucket>());
    }
}

MeshWorkerPool::~MeshWorkerPool()
{
    shutdown();
}

i32 MeshWorkerPool::_getOptimalThreadCount()
{
    const unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    const i32 count = static_cast<i32>(hardwareConcurrency) - 1;
    return std::clamp(count / 2, 1, 32);
}

void MeshWorkerPool::start()
{
    if (m_running.exchange(true, std::memory_order::acq_rel)) {
        return;
    }

    m_runningTaskCount.store(0, std::memory_order::release);
    m_stop.store(false, std::memory_order::release);

    // 支持 shutdown 后重启：回收桶已在 shutdown() 清空，这里按线程数重建。
    m_recycleBuckets.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_recycleBuckets.emplace_back(std::make_unique<RecycleBucket>());
    }

    m_workers.reserve(m_threadCount);
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&MeshWorkerPool::_workerLoop, this, i);
    }

    spdlog::info("MeshWorkerPool started with {} threads", m_threadCount);
}

void MeshWorkerPool::shutdown()
{
    if (!m_running.exchange(false, std::memory_order::acq_rel)) {
        return;
    }

    m_stop.store(true, std::memory_order::release);
    m_condition.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    m_workers.clear();

    // workers 已全部 join，无人再访问回收桶；清空释放桶内残留 MeshData 的 capacity。
    m_recycleBuckets.clear();

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

    m_runningTaskCount.store(0, std::memory_order::release);

    spdlog::info("MeshWorkerPool shutdown complete");
}

bool MeshWorkerPool::isRunning() const
{
    return m_running.load(std::memory_order::acquire);
}

void MeshWorkerPool::submit(MeshWorkerTask task)
{
    if (!isRunning()) {
        spdlog::warn("MeshWorkerPool: submit ignored because pool is not running");
        return;
    }

    if (!task.chunkData) {
        spdlog::warn(
            "MeshWorkerPool: submit ignored for chunk ({}, {}), chunk data is null", task.chunkId.x, task.chunkId.z);
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

void MeshWorkerPool::drainCompleted(const std::function<void(MeshWorkerResult&&)>& callback, u32 maxCount)
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
    return m_runningTaskCount.load(std::memory_order::acquire);
}

size_t MeshWorkerPool::completedTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    return m_completedQueue.size();
}

bool MeshWorkerPool::_isCancelled(const MeshWorkerTask& task)
{
    return task.abortSignal && task.abortSignal->load(std::memory_order::acquire);
}

void MeshWorkerPool::_workerLoop(i32 workerId)
{
    const std::string threadName = "ChunkMeshWorker-" + std::to_string(workerId);
    // sibling_order_rank = 300 + workerId，让 worker-0 排最前（根 track thread_ordering=EXPLICIT 生效）。
    // rankBase=300 排在 ServerCompute(100+)/ServerIO(200+) 之后，组内按 workerId 升序；
    // 每组间隔 100，避免线程数 >10 时跨组相交。
    constexpr int kWorkerRankBase = 300;
    mc::profiler::ProfilerManager::instance().setThreadName(threadName, kWorkerRankBase + workerId);

    while (true) {
        MeshWorkerTask task;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            m_condition.wait(lock, [this] { return !m_taskQueue.empty() || m_stop.load(std::memory_order::acquire); });

            if (m_stop.load(std::memory_order::acquire) && m_taskQueue.empty()) {
                break;
            }

            if (m_taskQueue.empty()) {
                continue;
            }

            InternalTask internalTask = std::move(m_taskQueue.front());
            m_taskQueue.pop();
            task = std::move(internalTask.task);
        }

        m_runningTaskCount.fetch_add(1, std::memory_order::acq_rel);
        _executeTask(workerId, task);
        m_runningTaskCount.fetch_sub(1, std::memory_order::acq_rel);
    }
}

void MeshWorkerPool::_executeTask(i32 workerId, const MeshWorkerTask& task)
{
    ChunkPos chunkPos(task.chunkId.x, task.chunkId.z);
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh,
        "BuildMesh",
        "pos",
        fmt::format("({}, {})", task.chunkId.x, task.chunkId.z),
        [flow = ::perfetto::Flow::ProcessScoped(chunkPos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    MeshWorkerResult result;
    result.chunkId = task.chunkId;
    result.taskId = task.taskId;
    result.workerId = workerId;
    result.success = false;
    result.cancelled = false;

    // 从本 worker 的回收桶取出带历史 capacity 的 MeshData，避免每次从 0 重新 reserve。
    // generateSplitMesh 内部的 clear()(只清 size)+ reserve()(capacity 足够时 no-op)会自动复用。
    result.solidMesh = _acquireRecycled(workerId, false);
    result.transparentMesh = _acquireRecycled(workerId, true);

    if (_isCancelled(task)) {
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

        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "GenerateSolidMesh");
        ChunkMesher::generateSplitMesh(
            *task.chunkData, result.solidMesh, result.transparentMesh, neighborPtrs, task.abortSignal.get());

        if (_isCancelled(task)) {
            result.cancelled = true;
            result.success = false;
        } else {
            result.success = true;
        }
    }
    catch (const std::exception& e) {
        spdlog::error("MeshWorkerPool: build failed for chunk ({}, {}): {}", task.chunkId.x, task.chunkId.z, e.what());
        result.success = false;
    }
    catch (...) {
        spdlog::error("MeshWorkerPool: build failed for chunk ({}, {}): unknown error", task.chunkId.x, task.chunkId.z);
        result.success = false;
    }

    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedQueue.push(std::move(result));
    }
}

// ============================================================================
// MeshData 回收池（free-list）
// ============================================================================

void MeshWorkerPool::recycle(i32 workerId, MeshData&& solid, MeshData&& transparent)
{
    // pool 未运行或 workerId 越界（老数据/边界）：直接析构释放，不进池。
    // 右值参在函数返回时析构，Tracy 对其 vector 缓冲区发对应 free，正确。
    if (!isRunning() || workerId < 0 || workerId >= static_cast<i32>(m_recycleBuckets.size())) {
        return;
    }

    _recycleOne(workerId, false, std::move(solid));
    _recycleOne(workerId, true, std::move(transparent));
}

MeshData MeshWorkerPool::_acquireRecycled(i32 workerId, bool transparent)
{
    if (workerId < 0 || workerId >= static_cast<i32>(m_recycleBuckets.size())) {
        return {};
    }

    RecycleBucket& bucket = *m_recycleBuckets[workerId];
    std::lock_guard<std::mutex> lock(bucket.mutex);
    std::vector<MeshData>& slots = transparent ? bucket.transparentSlots : bucket.solidSlots;
    if (slots.empty()) {
        return {};
    }
    MeshData out = std::move(slots.back());
    slots.pop_back();
    return out;
}

void MeshWorkerPool::_recycleOne(i32 workerId, bool transparent, MeshData&& data)
{
    _shrinkIfBloated(data);

    RecycleBucket& bucket = *m_recycleBuckets[workerId];
    std::lock_guard<std::mutex> lock(bucket.mutex);
    std::vector<MeshData>& slots = transparent ? bucket.transparentSlots : bucket.solidSlots;
    if (slots.size() >= kRecycleSlotsPerBucket) {
        // 桶满：不进池，data 在函数返回时析构释放。
        return;
    }
    slots.push_back(std::move(data));
}

void MeshWorkerPool::_shrinkIfBloated(MeshData& data)
{
    // 两条件同时满足才 shrink：capacity 超阈值 且 远大于实际 size(4 倍以上)。
    // 归还时 data 已 clear()(size=0)，条件退化为「超阈值即 shrink」——空壳不该带超大 capacity 入池。
    // shrink_to_fit 会触发 Tracy 成对 free(旧大块)+alloc(新小块)，严格一对一，不变量保持。
    if (data.vertices.capacity() > kMaxReuseVertexCapacity && data.vertices.capacity() > data.vertices.size() * 4) {
        data.vertices.shrink_to_fit();
    }
    if (data.indices.capacity() > kMaxReuseIndexCapacity && data.indices.capacity() > data.indices.size() * 4) {
        data.indices.shrink_to_fit();
    }
}

} // namespace mc::client
