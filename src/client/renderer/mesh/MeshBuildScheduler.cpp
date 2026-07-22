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

#include "MeshBuildScheduler.hpp"
#include <algorithm>
#include <chrono>
#include <thread>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

#include "common/util/math/MathConstants.hpp"

namespace mc::client {

MeshBuildScheduler::MeshBuildScheduler(util::UniversalWorkerPool& workerPool,
    std::shared_ptr<MeshDataPool> dataPool,
    std::shared_ptr<MeshResultQueue> resultQueue,
    const MeshSchedulerConfig& config)
    : m_workerPool(workerPool)
    , m_dataPool(std::move(dataPool))
    , m_resultQueue(std::move(resultQueue))
    , m_config(config)
{
    if (m_config.maxDispatchedTaskCount <= 0) {
        spdlog::warn(
            "MeshBuildScheduler: invalid maxDispatchedTaskCount={}, fallback to 1", m_config.maxDispatchedTaskCount);
        m_config.maxDispatchedTaskCount = 1;
    }

    if (m_config.reprioritizeIntervalFrames <= 0) {
        spdlog::warn("MeshBuildScheduler: invalid reprioritizeIntervalFrames={}, fallback to 1",
            m_config.reprioritizeIntervalFrames);
        m_config.reprioritizeIntervalFrames = 1;
    }

    if (m_config.cameraMoveThreshold < 0.0f) {
        spdlog::warn("MeshBuildScheduler: invalid cameraMoveThreshold={}, fallback to 0", m_config.cameraMoveThreshold);
        m_config.cameraMoveThreshold = 0.0f;
    }

    m_config.cameraDirectionDotThreshold = std::clamp(m_config.cameraDirectionDotThreshold, -1.0f, 1.0f);
    m_config.behindCancelDotThreshold = std::clamp(m_config.behindCancelDotThreshold, -1.0f, 1.0f);

    if (m_config.behindCancelDistanceChunks < 0.0f) {
        spdlog::warn("MeshBuildScheduler: invalid behindCancelDistanceChunks={}, fallback to 0",
            m_config.behindCancelDistanceChunks);
        m_config.behindCancelDistanceChunks = 0.0f;
    }
}

void MeshBuildScheduler::setViewState(const MeshSchedulerViewState& viewState)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_viewState = viewState;
    m_hasViewState = true;

    // 更新视锥体
    m_frustum.extractFromMatrix(viewState.viewProjectionMatrix);
    m_frustum.setCameraPosition(viewState.cameraPosition);
}

u64 MeshBuildScheduler::submit(MeshBuildRequest request)
{
    if (!request.chunkData) {
        spdlog::warn("MeshBuildScheduler: submit ignored for chunk ({}, {}), chunk data is null",
            request.chunkId.x,
            request.chunkId.z);
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    const ChunkId chunkId = request.chunkId;
    const u64 taskId = m_nextTaskId++;
    const u64 submitOrder = m_submitOrderCounter++;

    auto latestIt = m_latestTaskByChunk.find(chunkId);
    if (latestIt != m_latestTaskByChunk.end()) {
        auto previousTaskIt = m_tasks.find(latestIt->second);
        if (previousTaskIt != m_tasks.end()) {
            ScheduledTask& previousTask = previousTaskIt->second;
            _requestCancellation(previousTask);
            if (previousTask.state == TaskState::Pending) {
                _removeTaskImmediately(previousTask.taskId);
            }
        }
    }

    ScheduledTask task;
    task.chunkId = chunkId;
    task.taskId = taskId;
    task.request = std::move(request);
    task.abortSignal = std::make_shared<std::atomic<bool>>(false);
    task.state = TaskState::Pending;
    task.submitOrder = submitOrder;

    _updateTaskScore(task);

    m_tasks.emplace(taskId, std::move(task));
    m_latestTaskByChunk[chunkId] = taskId;
    m_pendingOrderDirty = true;
    ++m_submittedTaskCount;

    return taskId;
}

void MeshBuildScheduler::cancelChunk(const ChunkId& chunkId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<u64> removeTaskIds;
    removeTaskIds.reserve(8);

    for (auto& [taskId, task] : m_tasks) {
        if (task.chunkId != chunkId) {
            continue;
        }

        _requestCancellation(task);
        if (task.state == TaskState::Pending) {
            removeTaskIds.push_back(taskId);
        }
    }

    for (u64 taskId : removeTaskIds) {
        _removeTaskImmediately(taskId);
    }

    m_latestTaskByChunk.erase(chunkId);
    m_pendingOrderDirty = true;
}

void MeshBuildScheduler::cancelAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<u64> removeTaskIds;
    removeTaskIds.reserve(m_tasks.size());

    for (auto& [taskId, task] : m_tasks) {
        _requestCancellation(task);
        if (task.state == TaskState::Pending) {
            removeTaskIds.push_back(taskId);
        }
    }

    for (u64 taskId : removeTaskIds) {
        _removeTaskImmediately(taskId);
    }

    m_latestTaskByChunk.clear();
    m_pendingOrderDirty = true;
}

void MeshBuildScheduler::tick()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    ++m_frameCounter;

    _cancelOutOfDateTasks();

    if (_shouldReprioritize()) {
        _reprioritizePendingTasks();
    }

    _dispatchPendingTasks();
}

void MeshBuildScheduler::shutdown()
{
    cancelAll();

    // 等待所有在途任务回调归零。回调在 worker 线程只做 m_inflightTaskCount 减法，
    // 归零即所有在途任务已 push 结果（或 lock 结果队列失败丢弃）。cancelAll 已设置
    // abortSignal，worker 会在 abortSignal 检查处短路 execute，归零不会久等。
    while (m_inflightTaskCount.load(std::memory_order::acquire) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void MeshBuildScheduler::drainCompleted(const std::function<void(MeshWorkerResult&&)>& callback, u32 maxCount)
{
    m_resultQueue->drain(
        [this, &callback](MeshWorkerResult&& result) {
            bool shouldDeliver = false;

            {
                std::lock_guard<std::mutex> lock(m_mutex);

                auto taskIt = m_tasks.find(result.taskId);
                if (taskIt == m_tasks.end()) {
                    ++m_discardedResultCount;
                    return;
                }

                ScheduledTask completedTask = std::move(taskIt->second);
                m_tasks.erase(taskIt);

                if (completedTask.state == TaskState::Dispatched && m_dispatchedTaskCount > 0) {
                    --m_dispatchedTaskCount;
                }

                bool isLatest = false;
                auto latestIt = m_latestTaskByChunk.find(completedTask.chunkId);
                if (latestIt != m_latestTaskByChunk.end() && latestIt->second == completedTask.taskId) {
                    isLatest = true;
                    m_latestTaskByChunk.erase(latestIt);
                }

                if (!isLatest) {
                    ++m_discardedResultCount;
                    return;
                }

                if (result.cancelled) {
                    shouldDeliver = true;
                    return;
                }

                if (!result.success) {
                    spdlog::warn("MeshBuildScheduler: mesh build failed for latest task {}, chunk ({}, {})",
                        result.taskId,
                        result.chunkId.x,
                        result.chunkId.z);
                    shouldDeliver = true;
                    return;
                }

                ++m_completedTaskCount;
                shouldDeliver = true;
            }

            if (shouldDeliver && callback) {
                callback(std::move(result));
            }
        },
        maxCount);
}

bool MeshBuildScheduler::isTaskTracked(u64 taskId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.find(taskId) != m_tasks.end();
}

MeshSchedulerStats MeshBuildScheduler::stats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    MeshSchedulerStats result;
    result.trackedTaskCount = m_tasks.size();
    result.dispatchedTaskCount = m_dispatchedTaskCount;
    result.submittedTaskCount = m_submittedTaskCount;
    result.cancelledTaskCount = m_cancelledTaskCount;
    result.completedTaskCount = m_completedTaskCount;
    result.discardedResultCount = m_discardedResultCount;

    size_t pendingCount = 0;
    for (const auto& [taskId, task] : m_tasks) {
        (void)taskId;
        if (task.state == TaskState::Pending) {
            ++pendingCount;
        }
    }
    result.pendingTaskCount = pendingCount;

    return result;
}

bool MeshBuildScheduler::_shouldReprioritize() const
{
    if (!m_pendingOrderDirty) {
        bool hasPendingTask = false;
        for (const auto& [taskId, task] : m_tasks) {
            (void)taskId;
            if (task.state == TaskState::Pending) {
                hasPendingTask = true;
                break;
            }
        }
        if (!hasPendingTask) {
            return false;
        }
    }

    if (m_pendingOrderDirty) {
        return true;
    }

    if (!m_hasViewState) {
        return false;
    }

    if (!m_hasLastReprioritizedView) {
        return true;
    }

    if ((m_frameCounter % m_config.reprioritizeIntervalFrames) == 0) {
        return true;
    }

    const glm::vec2 cameraDelta(m_viewState.cameraPosition.x - m_lastReprioritizedViewState.cameraPosition.x,
        m_viewState.cameraPosition.z - m_lastReprioritizedViewState.cameraPosition.z);

    const f32 cameraMoveDistance = glm::length(cameraDelta);
    if (cameraMoveDistance >= m_config.cameraMoveThreshold) {
        return true;
    }

    const glm::vec2 currentForward(m_viewState.cameraForward.x, m_viewState.cameraForward.z);
    const glm::vec2 lastForward(
        m_lastReprioritizedViewState.cameraForward.x, m_lastReprioritizedViewState.cameraForward.z);

    const f32 currentForwardLen = glm::length(currentForward);
    const f32 lastForwardLen = glm::length(lastForward);
    if (currentForwardLen < mc::math::EPSILON || lastForwardLen < mc::math::EPSILON) {
        return false;
    }

    const f32 forwardDot = glm::dot(currentForward / currentForwardLen, lastForward / lastForwardLen);
    return forwardDot <= m_config.cameraDirectionDotThreshold;
}

void MeshBuildScheduler::_reprioritizePendingTasks()
{
    m_pendingOrder.clear();

    std::vector<u64> removeTaskIds;
    removeTaskIds.reserve(32);

    for (auto& [taskId, task] : m_tasks) {
        if (task.state != TaskState::Pending) {
            continue;
        }

        auto latestIt = m_latestTaskByChunk.find(task.chunkId);
        if (latestIt == m_latestTaskByChunk.end() || latestIt->second != task.taskId) {
            removeTaskIds.push_back(taskId);
            continue;
        }

        _updateTaskScore(task);

        if (_isOutOfRenderDistance(task) || _shouldCancelBehindTask(task)) {
            _requestCancellation(task);
            removeTaskIds.push_back(taskId);
            continue;
        }

        PendingSortItem item;
        item.taskId = task.taskId;
        item.score = task.score;
        item.submitOrder = task.submitOrder;
        m_pendingOrder.push_back(item);
    }

    for (u64 taskId : removeTaskIds) {
        _removeTaskImmediately(taskId);
    }

    std::sort(m_pendingOrder.begin(), m_pendingOrder.end(), [](const PendingSortItem& lhs, const PendingSortItem& rhs) {
        if (lhs.score != rhs.score) {
            return lhs.score < rhs.score;
        }
        return lhs.submitOrder < rhs.submitOrder;
    });

    m_pendingOrderDirty = false;
    if (m_hasViewState) {
        m_lastReprioritizedViewState = m_viewState;
        m_hasLastReprioritizedView = true;
    }
}

void MeshBuildScheduler::_dispatchPendingTasks()
{
    if (!m_workerPool.isRunning()) {
        return;
    }

    if (m_dispatchedTaskCount >= static_cast<size_t>(m_config.maxDispatchedTaskCount)) {
        return;
    }

    if (m_pendingOrder.empty()) {
        return;
    }

    size_t availableSlots = static_cast<size_t>(m_config.maxDispatchedTaskCount) - m_dispatchedTaskCount;
    bool taskDispatched = false;

    for (const PendingSortItem& item : m_pendingOrder) {
        if (availableSlots == 0) {
            break;
        }

        auto taskIt = m_tasks.find(item.taskId);
        if (taskIt == m_tasks.end()) {
            continue;
        }

        ScheduledTask& task = taskIt->second;
        if (task.state != TaskState::Pending) {
            continue;
        }

        auto latestIt = m_latestTaskByChunk.find(task.chunkId);
        if (latestIt == m_latestTaskByChunk.end() || latestIt->second != task.taskId) {
            _removeTaskImmediately(task.taskId);
            continue;
        }

        if (_isOutOfRenderDistance(task) || _shouldCancelBehindTask(task)) {
            _requestCancellation(task);
            _removeTaskImmediately(task.taskId);
            continue;
        }

        // 必须在 submit 之前自增：回调可能在另一线程立即触发（任务被拒/秒完），
        // 回调只做减法，归零责任全在回调侧。submit 拒绝任务时回调仍会被调用
        // （见 UniversalWorkerPool.cpp 拒绝路径），故计数不会泄漏。
        m_inflightTaskCount.fetch_add(1, std::memory_order::acq_rel);

        auto buildTask = std::make_unique<MeshBuildTask>(task.chunkId,
            task.taskId,
            task.request.chunkData,
            task.request.neighbors,
            m_dataPool,
            std::weak_ptr<MeshResultQueue>(m_resultQueue));

        task.state = TaskState::Dispatched;
        ++m_dispatchedTaskCount;
        --availableSlots;

        // 取消令牌经 pool.submit 的 abortSignal 参数传入；execute 收到的即该令牌，
        // 与原 MeshWorkerPool 检查 task.abortSignal 语义一致。
        m_workerPool.submit(
            std::move(buildTask),
            [this](bool, ::mc::util::ITask*) { m_inflightTaskCount.fetch_sub(1, std::memory_order::acq_rel); },
            ::mc::util::TaskPriority::Normal,
            task.abortSignal);
        taskDispatched = true;
    }

    if (taskDispatched) {
        m_pendingOrderDirty = true;
    }
}

void MeshBuildScheduler::_cancelOutOfDateTasks()
{
    if (!m_hasViewState) {
        return;
    }

    std::vector<u64> removeTaskIds;
    removeTaskIds.reserve(32);

    for (auto& [taskId, task] : m_tasks) {
        const bool shouldCancel = _isOutOfRenderDistance(task) || _shouldCancelBehindTask(task);
        if (!shouldCancel) {
            continue;
        }

        _requestCancellation(task);

        if (task.state == TaskState::Pending) {
            removeTaskIds.push_back(taskId);
        }
    }

    for (u64 taskId : removeTaskIds) {
        _removeTaskImmediately(taskId);
    }
}

void MeshBuildScheduler::_removeTaskImmediately(u64 taskId)
{
    auto taskIt = m_tasks.find(taskId);
    if (taskIt == m_tasks.end()) {
        return;
    }

    if (taskIt->second.state == TaskState::Dispatched) {
        return;
    }

    const ChunkId chunkId = taskIt->second.chunkId;
    m_tasks.erase(taskIt);

    auto latestIt = m_latestTaskByChunk.find(chunkId);
    if (latestIt != m_latestTaskByChunk.end() && latestIt->second == taskId) {
        m_latestTaskByChunk.erase(latestIt);
    }

    m_pendingOrderDirty = true;
}

void MeshBuildScheduler::_requestCancellation(ScheduledTask& task)
{
    if (task.abortSignal) {
        task.abortSignal->store(true, std::memory_order::release);
    }

    if (!task.cancellationRequested) {
        task.cancellationRequested = true;
        ++m_cancelledTaskCount;
    }
}

bool MeshBuildScheduler::_isOutOfRenderDistance(const ScheduledTask& task) const
{
    if (!m_hasViewState) {
        return false;
    }

    const f32 distanceChunks = _chunkDistanceInChunks(m_viewState, task.chunkId);
    const f32 maxDistanceChunks = static_cast<f32>(m_viewState.renderDistanceChunks) + 1.0f;
    return distanceChunks > maxDistanceChunks;
}

bool MeshBuildScheduler::_shouldCancelBehindTask(const ScheduledTask& task) const
{
    if (!m_hasViewState) {
        return false;
    }

    const f32 distanceChunks = _chunkDistanceInChunks(m_viewState, task.chunkId);
    if (distanceChunks < m_config.behindCancelDistanceChunks) {
        return false;
    }

    const f32 forwardDot = _chunkForwardDot(m_viewState, task.chunkId);
    return forwardDot < m_config.behindCancelDotThreshold;
}

void MeshBuildScheduler::_updateTaskScore(ScheduledTask& task)
{
    if (!m_hasViewState) {
        task.distanceChunks = 0.0f;
        task.forwardDot = 0.0f;
        task.inFrustum = true;
        task.score = static_cast<f32>(task.submitOrder);
        return;
    }

    task.distanceChunks = _chunkDistanceInChunks(m_viewState, task.chunkId);
    task.forwardDot = _chunkForwardDot(m_viewState, task.chunkId);

    // 使用 Frustum 类进行视锥剔除
    task.inFrustum = m_frustum.isChunkVisible(
        task.chunkId.x, task.chunkId.z, m_viewState.minBuildHeight, m_viewState.maxBuildHeight);

    f32 score = task.distanceChunks;

    if (task.inFrustum) {
        score -= 100.0f;
    }

    score -= task.forwardDot * 8.0f;

    if (task.forwardDot < m_config.behindCancelDotThreshold) {
        score += 20.0f;
    }

    task.score = score;
}

f32 MeshBuildScheduler::_chunkDistanceInChunks(const MeshSchedulerViewState& viewState, const ChunkId& chunkId)
{
    const f32 chunkCenterOffset = static_cast<f32>(world::CHUNK_WIDTH) * 0.5f;
    const f32 centerX = static_cast<f32>(chunkId.x * world::CHUNK_WIDTH) + chunkCenterOffset;
    const f32 centerZ = static_cast<f32>(chunkId.z * world::CHUNK_WIDTH) + chunkCenterOffset;

    const glm::vec2 toChunk(centerX - viewState.cameraPosition.x, centerZ - viewState.cameraPosition.z);
    const f32 distanceBlocks = glm::length(toChunk);
    return distanceBlocks / static_cast<f32>(world::CHUNK_WIDTH);
}

f32 MeshBuildScheduler::_chunkForwardDot(const MeshSchedulerViewState& viewState, const ChunkId& chunkId)
{
    const f32 chunkCenterOffset = static_cast<f32>(world::CHUNK_WIDTH) * 0.5f;
    const f32 centerX = static_cast<f32>(chunkId.x * world::CHUNK_WIDTH) + chunkCenterOffset;
    const f32 centerZ = static_cast<f32>(chunkId.z * world::CHUNK_WIDTH) + chunkCenterOffset;

    glm::vec2 cameraForward(viewState.cameraForward.x, viewState.cameraForward.z);
    const f32 cameraForwardLen = glm::length(cameraForward);
    if (cameraForwardLen < mc::math::EPSILON) {
        return 0.0f;
    }
    cameraForward /= cameraForwardLen;

    glm::vec2 toChunk(centerX - viewState.cameraPosition.x, centerZ - viewState.cameraPosition.z);
    const f32 toChunkLen = glm::length(toChunk);
    if (toChunkLen < mc::math::EPSILON) {
        return 1.0f;
    }
    toChunk /= toChunkLen;

    return glm::dot(cameraForward, toChunk);
}

} // namespace mc::client
