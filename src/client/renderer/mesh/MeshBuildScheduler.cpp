#include "MeshBuildScheduler.hpp"
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc::client {

namespace {
constexpr f32 FRUSTUM_MARGIN = 1.2f;
constexpr f32 EPSILON = 1e-5f;
} // namespace

MeshBuildScheduler::MeshBuildScheduler(
    MeshWorkerPool& workerPool,
    const MeshSchedulerConfig& config
)
    : m_workerPool(workerPool)
    , m_config(config)
{
    if (m_config.maxDispatchedTaskCount <= 0) {
        spdlog::warn("MeshBuildScheduler: invalid maxDispatchedTaskCount={}, fallback to 1", m_config.maxDispatchedTaskCount);
        m_config.maxDispatchedTaskCount = 1;
    }

    if (m_config.reprioritizeIntervalFrames <= 0) {
        spdlog::warn("MeshBuildScheduler: invalid reprioritizeIntervalFrames={}, fallback to 1", m_config.reprioritizeIntervalFrames);
        m_config.reprioritizeIntervalFrames = 1;
    }

    if (m_config.cameraMoveThreshold < 0.0f) {
        spdlog::warn("MeshBuildScheduler: invalid cameraMoveThreshold={}, fallback to 0", m_config.cameraMoveThreshold);
        m_config.cameraMoveThreshold = 0.0f;
    }

    m_config.cameraDirectionDotThreshold = std::clamp(m_config.cameraDirectionDotThreshold, -1.0f, 1.0f);
    m_config.behindCancelDotThreshold = std::clamp(m_config.behindCancelDotThreshold, -1.0f, 1.0f);

    if (m_config.behindCancelDistanceChunks < 0.0f) {
        spdlog::warn("MeshBuildScheduler: invalid behindCancelDistanceChunks={}, fallback to 0", m_config.behindCancelDistanceChunks);
        m_config.behindCancelDistanceChunks = 0.0f;
    }
}

void MeshBuildScheduler::setViewState(const MeshSchedulerViewState& viewState)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_viewState = viewState;
    m_hasViewState = true;
}

u64 MeshBuildScheduler::submit(MeshBuildRequest request)
{
    if (!request.chunkData) {
        spdlog::warn(
            "MeshBuildScheduler: submit ignored for chunk ({}, {}), chunk data is null",
            request.chunkId.x,
            request.chunkId.z
        );
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
            requestCancellation(previousTask);
            if (previousTask.state == TaskState::Pending) {
                removeTaskImmediately(previousTask.taskId);
            }
        }
    }

    ScheduledTask task;
    task.chunkId = chunkId;
    task.taskId = taskId;
    task.request = std::move(request);
    task.cancelSignal = std::make_shared<std::atomic<bool>>(false);
    task.state = TaskState::Pending;
    task.submitOrder = submitOrder;

    updateTaskScore(task);

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

        requestCancellation(task);
        if (task.state == TaskState::Pending) {
            removeTaskIds.push_back(taskId);
        }
    }

    for (u64 taskId : removeTaskIds) {
        removeTaskImmediately(taskId);
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
        requestCancellation(task);
        if (task.state == TaskState::Pending) {
            removeTaskIds.push_back(taskId);
        }
    }

    for (u64 taskId : removeTaskIds) {
        removeTaskImmediately(taskId);
    }

    m_latestTaskByChunk.clear();
    m_pendingOrderDirty = true;
}

void MeshBuildScheduler::tick()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    ++m_frameCounter;

    cancelOutOfDateTasks();

    if (shouldReprioritize()) {
        reprioritizePendingTasks();
    }

    dispatchPendingTasks();
}

void MeshBuildScheduler::drainCompleted(
    const std::function<void(MeshWorkerResult&&)>& callback,
    u32 maxCount
)
{
    m_workerPool.drainCompleted(
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
                    spdlog::warn(
                        "MeshBuildScheduler: mesh build failed for latest task {}, chunk ({}, {})",
                        result.taskId,
                        result.chunkId.x,
                        result.chunkId.z
                    );
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
        maxCount
    );
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

bool MeshBuildScheduler::shouldReprioritize() const
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

    const glm::vec2 cameraDelta(
        m_viewState.cameraPosition.x - m_lastReprioritizedViewState.cameraPosition.x,
        m_viewState.cameraPosition.z - m_lastReprioritizedViewState.cameraPosition.z
    );

    const f32 cameraMoveDistance = glm::length(cameraDelta);
    if (cameraMoveDistance >= m_config.cameraMoveThreshold) {
        return true;
    }

    const glm::vec2 currentForward(m_viewState.cameraForward.x, m_viewState.cameraForward.z);
    const glm::vec2 lastForward(
        m_lastReprioritizedViewState.cameraForward.x,
        m_lastReprioritizedViewState.cameraForward.z
    );

    const f32 currentForwardLen = glm::length(currentForward);
    const f32 lastForwardLen = glm::length(lastForward);
    if (currentForwardLen < EPSILON || lastForwardLen < EPSILON) {
        return false;
    }

    const f32 forwardDot = glm::dot(currentForward / currentForwardLen, lastForward / lastForwardLen);
    return forwardDot <= m_config.cameraDirectionDotThreshold;
}

void MeshBuildScheduler::reprioritizePendingTasks()
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

        updateTaskScore(task);

        if (isOutOfRenderDistance(task) || shouldCancelBehindTask(task)) {
            requestCancellation(task);
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
        removeTaskImmediately(taskId);
    }

    std::sort(
        m_pendingOrder.begin(),
        m_pendingOrder.end(),
        [](const PendingSortItem& lhs, const PendingSortItem& rhs) {
            if (lhs.score != rhs.score) {
                return lhs.score < rhs.score;
            }
            return lhs.submitOrder < rhs.submitOrder;
        }
    );

    m_pendingOrderDirty = false;
    if (m_hasViewState) {
        m_lastReprioritizedViewState = m_viewState;
        m_hasLastReprioritizedView = true;
    }
}

void MeshBuildScheduler::dispatchPendingTasks()
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
            removeTaskImmediately(task.taskId);
            continue;
        }

        if (isOutOfRenderDistance(task) || shouldCancelBehindTask(task)) {
            requestCancellation(task);
            removeTaskImmediately(task.taskId);
            continue;
        }

        MeshWorkerTask workerTask;
        workerTask.chunkId = task.chunkId;
        workerTask.taskId = task.taskId;
        workerTask.chunkData = task.request.chunkData;
        workerTask.neighbors = task.request.neighbors;
        workerTask.cancelSignal = task.cancelSignal;

        task.state = TaskState::Dispatched;
        ++m_dispatchedTaskCount;
        --availableSlots;

        m_workerPool.submit(std::move(workerTask));
        taskDispatched = true;
    }

    if (taskDispatched) {
        m_pendingOrderDirty = true;
    }
}

void MeshBuildScheduler::cancelOutOfDateTasks()
{
    if (!m_hasViewState) {
        return;
    }

    std::vector<u64> removeTaskIds;
    removeTaskIds.reserve(32);

    for (auto& [taskId, task] : m_tasks) {
        const bool shouldCancel = isOutOfRenderDistance(task) || shouldCancelBehindTask(task);
        if (!shouldCancel) {
            continue;
        }

        requestCancellation(task);

        if (task.state == TaskState::Pending) {
            removeTaskIds.push_back(taskId);
        }
    }

    for (u64 taskId : removeTaskIds) {
        removeTaskImmediately(taskId);
    }
}

void MeshBuildScheduler::removeTaskImmediately(u64 taskId)
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

void MeshBuildScheduler::requestCancellation(ScheduledTask& task)
{
    if (task.cancelSignal) {
        task.cancelSignal->store(true, std::memory_order_release);
    }

    if (!task.cancellationRequested) {
        task.cancellationRequested = true;
        ++m_cancelledTaskCount;
    }
}

bool MeshBuildScheduler::isOutOfRenderDistance(const ScheduledTask& task) const
{
    if (!m_hasViewState) {
        return false;
    }

    const f32 distanceChunks = chunkDistanceInChunks(m_viewState, task.chunkId);
    const f32 maxDistanceChunks = static_cast<f32>(m_viewState.renderDistanceChunks) + 1.0f;
    return distanceChunks > maxDistanceChunks;
}

bool MeshBuildScheduler::shouldCancelBehindTask(const ScheduledTask& task) const
{
    if (!m_hasViewState) {
        return false;
    }

    const f32 distanceChunks = chunkDistanceInChunks(m_viewState, task.chunkId);
    if (distanceChunks < m_config.behindCancelDistanceChunks) {
        return false;
    }

    const f32 forwardDot = chunkForwardDot(m_viewState, task.chunkId);
    return forwardDot < m_config.behindCancelDotThreshold;
}

void MeshBuildScheduler::updateTaskScore(ScheduledTask& task)
{
    if (!m_hasViewState) {
        task.distanceChunks = 0.0f;
        task.forwardDot = 0.0f;
        task.inFrustum = true;
        task.score = static_cast<f32>(task.submitOrder);
        return;
    }

    task.distanceChunks = chunkDistanceInChunks(m_viewState, task.chunkId);
    task.forwardDot = chunkForwardDot(m_viewState, task.chunkId);
    task.inFrustum = chunkInFrustum(m_viewState, task.chunkId);

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

f32 MeshBuildScheduler::chunkDistanceInChunks(
    const MeshSchedulerViewState& viewState,
    const ChunkId& chunkId
)
{
    const f32 chunkCenterOffset = static_cast<f32>(ChunkData::WIDTH) * 0.5f;
    const f32 centerX = static_cast<f32>(chunkId.x * ChunkData::WIDTH) + chunkCenterOffset;
    const f32 centerZ = static_cast<f32>(chunkId.z * ChunkData::WIDTH) + chunkCenterOffset;

    const glm::vec2 toChunk(centerX - viewState.cameraPosition.x, centerZ - viewState.cameraPosition.z);
    const f32 distanceBlocks = glm::length(toChunk);
    return distanceBlocks / static_cast<f32>(ChunkData::WIDTH);
}

f32 MeshBuildScheduler::chunkForwardDot(
    const MeshSchedulerViewState& viewState,
    const ChunkId& chunkId
)
{
    const f32 chunkCenterOffset = static_cast<f32>(ChunkData::WIDTH) * 0.5f;
    const f32 centerX = static_cast<f32>(chunkId.x * ChunkData::WIDTH) + chunkCenterOffset;
    const f32 centerZ = static_cast<f32>(chunkId.z * ChunkData::WIDTH) + chunkCenterOffset;

    glm::vec2 cameraForward(viewState.cameraForward.x, viewState.cameraForward.z);
    const f32 cameraForwardLen = glm::length(cameraForward);
    if (cameraForwardLen < EPSILON) {
        return 0.0f;
    }
    cameraForward /= cameraForwardLen;

    glm::vec2 toChunk(centerX - viewState.cameraPosition.x, centerZ - viewState.cameraPosition.z);
    const f32 toChunkLen = glm::length(toChunk);
    if (toChunkLen < EPSILON) {
        return 1.0f;
    }
    toChunk /= toChunkLen;

    return glm::dot(cameraForward, toChunk);
}

bool MeshBuildScheduler::chunkInFrustum(
    const MeshSchedulerViewState& viewState,
    const ChunkId& chunkId
)
{
    const f32 chunkCenterOffset = static_cast<f32>(ChunkData::WIDTH) * 0.5f;
    const f32 centerX = static_cast<f32>(chunkId.x * ChunkData::WIDTH) + chunkCenterOffset;
    const f32 centerY = static_cast<f32>(viewState.minBuildHeight + viewState.maxBuildHeight) * 0.5f;
    const f32 centerZ = static_cast<f32>(chunkId.z * ChunkData::WIDTH) + chunkCenterOffset;

    const glm::vec4 clip = viewState.viewProjectionMatrix * glm::vec4(centerX, centerY, centerZ, 1.0f);
    if (clip.w <= 0.0f) {
        return false;
    }

    const f32 clipLimit = clip.w * FRUSTUM_MARGIN;
    return clip.x >= -clipLimit && clip.x <= clipLimit &&
           clip.y >= -clipLimit && clip.y <= clipLimit &&
           clip.z >= -clipLimit && clip.z <= clipLimit;
}

} // namespace mc::client
