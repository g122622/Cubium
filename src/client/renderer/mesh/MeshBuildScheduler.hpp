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

#pragma once

#include "MeshBuildTask.hpp"
#include "MeshDataPool.hpp"
#include "MeshResultQueue.hpp"
#include "MeshWorkerTypes.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/chunk/base/ChunkId.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace mc::client {

struct MeshSchedulerConfig {
    i32 maxDispatchedTaskCount;
    i32 reprioritizeIntervalFrames;
    f32 cameraMoveThreshold;
    f32 cameraDirectionDotThreshold;
    f32 behindCancelDotThreshold;
    f32 behindCancelDistanceChunks;
};

struct MeshSchedulerViewState {
    glm::vec3 cameraPosition;
    glm::vec3 cameraForward;
    glm::mat4 viewProjectionMatrix;
    i32 renderDistanceChunks;
    i32 minBuildHeight;
    i32 maxBuildHeight;
};

struct MeshBuildRequest {
    ChunkId chunkId;
    std::shared_ptr<const ChunkData> chunkData;
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors;
};

struct MeshSchedulerStats {
    size_t trackedTaskCount = 0;
    size_t pendingTaskCount = 0;
    size_t dispatchedTaskCount = 0;
    u64 submittedTaskCount = 0;
    u64 cancelledTaskCount = 0;
    u64 completedTaskCount = 0;
    u64 discardedResultCount = 0;
};

class MeshBuildScheduler {
public:
    MeshBuildScheduler(util::UniversalWorkerPool& workerPool,
        std::shared_ptr<MeshDataPool> dataPool,
        std::shared_ptr<MeshResultQueue> resultQueue,
        const MeshSchedulerConfig& config);

    MeshBuildScheduler(const MeshBuildScheduler&) = delete;
    MeshBuildScheduler& operator=(const MeshBuildScheduler&) = delete;

    void setViewState(const MeshSchedulerViewState& viewState);

    [[nodiscard]] u64 submit(MeshBuildRequest request);
    void cancelChunk(const ChunkId& chunkId);
    void cancelAll();

    void tick();

    void drainCompleted(const std::function<void(MeshWorkerResult&&)>& callback, u32 maxCount);

    /**
     * @brief 关停调度器：cancelAll 后等待所有在途任务回调归零。
     *
     * m_inflightTaskCount 在 submit 之前自增、在回调中减一。归零即所有在途任务的回调
     * 已落定，scheduler 析构顺序确定，避免池（生命周期长于 scheduler）晚到的回调访问
     * 已析构的 scheduler（回调只做计数减法，不触碰 scheduler 其它成员，纵深防护在
     * MeshBuildTask 侧用 weak_ptr<MeshResultQueue> 兜底）。
     */
    void shutdown();

    [[nodiscard]] bool isTaskTracked(u64 taskId) const;

    [[nodiscard]] MeshSchedulerStats stats() const;

private:
    enum class TaskState : u8 { Pending, Dispatched };

    struct ScheduledTask {
        ChunkId chunkId;
        u64 taskId = 0;
        MeshBuildRequest request;
        std::shared_ptr<std::atomic<bool>> abortSignal;
        TaskState state = TaskState::Pending;
        f32 distanceChunks = 0.0f;
        f32 forwardDot = 0.0f;
        f32 score = 0.0f;
        bool inFrustum = false;
        u64 submitOrder = 0;
        bool cancellationRequested = false;
    };

    struct PendingSortItem {
        u64 taskId = 0;
        f32 score = 0.0f;
        u64 submitOrder = 0;
    };

    [[nodiscard]] bool _shouldReprioritize() const;
    void _reprioritizePendingTasks();
    void _dispatchPendingTasks();

    void _cancelOutOfDateTasks();
    void _removeTaskImmediately(u64 taskId);
    void _requestCancellation(ScheduledTask& task);

    [[nodiscard]] bool _isOutOfRenderDistance(const ScheduledTask& task) const;
    [[nodiscard]] bool _shouldCancelBehindTask(const ScheduledTask& task) const;

    void _updateTaskScore(ScheduledTask& task);

    [[nodiscard]] static f32 _chunkDistanceInChunks(const MeshSchedulerViewState& viewState, const ChunkId& chunkId);
    [[nodiscard]] static f32 _chunkForwardDot(const MeshSchedulerViewState& viewState, const ChunkId& chunkId);

    util::UniversalWorkerPool& m_workerPool;
    std::shared_ptr<MeshDataPool> m_dataPool;
    std::shared_ptr<MeshResultQueue> m_resultQueue;
    /// 在途任务数（submit 前自增、回调中减一）。shutdown() 等其归零保证析构安全。
    std::atomic<size_t> m_inflightTaskCount{0};
    MeshSchedulerConfig m_config;

    std::unordered_map<u64, ScheduledTask> m_tasks;
    std::unordered_map<ChunkId, u64> m_latestTaskByChunk;
    std::vector<PendingSortItem> m_pendingOrder;

    MeshSchedulerViewState m_viewState{};
    MeshSchedulerViewState m_lastReprioritizedViewState{};

    /// 视锥体，用于视锥剔除
    mc::math::frustum::Frustum m_frustum;

    bool m_hasViewState = false;
    bool m_hasLastReprioritizedView = false;
    bool m_pendingOrderDirty = false;

    i32 m_frameCounter = 0;
    size_t m_dispatchedTaskCount = 0;

    u64 m_nextTaskId = 1;
    u64 m_submitOrderCounter = 1;

    u64 m_submittedTaskCount = 0;
    u64 m_cancelledTaskCount = 0;
    u64 m_completedTaskCount = 0;
    u64 m_discardedResultCount = 0;

    mutable std::mutex m_mutex;
};

} // namespace mc::client
