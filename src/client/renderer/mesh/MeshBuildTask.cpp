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

#include "MeshBuildTask.hpp"
#include "client/renderer/mesh/MeshDataPool.hpp"
#include "client/renderer/mesh/MeshResultQueue.hpp"
#include "client/renderer/mesh/MeshWorkerTypes.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/chunk/base/ChunkId.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

MeshBuildTask::MeshBuildTask(ChunkId chunkId,
    u64 taskId,
    std::shared_ptr<const ChunkData> chunkData,
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors,
    std::shared_ptr<MeshDataPool> dataPool,
    std::weak_ptr<MeshResultQueue> resultQueue)
    : m_chunkId(chunkId)
    , m_taskId(taskId)
    , m_chunkData(std::move(chunkData))
    , m_neighbors(std::move(neighbors))
    , m_dataPool(std::move(dataPool))
    , m_resultQueue(std::move(resultQueue))
{}

std::string MeshBuildTask::description() const
{
    return fmt::format("MeshBuildTask({}, {})", m_chunkId.x, m_chunkId.z);
}

bool MeshBuildTask::execute(const std::atomic<bool>& abortSignal)
{
    ChunkPos chunkPos(m_chunkId.x, m_chunkId.z);
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh,
        "BuildMesh",
        "pos",
        fmt::format("({}, {})", m_chunkId.x, m_chunkId.z),
        [flow = ::perfetto::Flow::ProcessScoped(chunkPos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 从回收池取出带历史 capacity 的 MeshData，避免每次从 0 重新 reserve。
    // generateSplitMesh 内部的 clear()(只清 size)+ reserve()(capacity 足够时 no-op)会自动复用。
    m_solidMesh = m_dataPool->acquire(false);
    m_transparentMesh = m_dataPool->acquire(true);

    if (abortSignal.load(std::memory_order::acquire)) {
        m_cancelled = true;
        _pushResult();
        return false;
    }

    try {
        const ChunkData* neighborPtrs[6] = {nullptr};
        for (size_t i = 0; i < 6; ++i) {
            neighborPtrs[i] = m_neighbors[i].get();
        }

        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "GenerateSolidMesh");
        ChunkMesher::generateSplitMesh(*m_chunkData, m_solidMesh, m_transparentMesh, neighborPtrs, &abortSignal);

        if (abortSignal.load(std::memory_order::acquire)) {
            m_cancelled = true;
            m_success = false;
        } else {
            m_success = true;
        }
    }
    catch (const std::exception& e) {
        spdlog::error("MeshBuildTask: build failed for chunk ({}, {}): {}", m_chunkId.x, m_chunkId.z, e.what());
        m_success = false;
    }
    catch (...) {
        spdlog::error("MeshBuildTask: build failed for chunk ({}, {}): unknown error", m_chunkId.x, m_chunkId.z);
        m_success = false;
    }

    _pushResult();
    return m_success;
}

void MeshBuildTask::onCancel()
{
    // 两处调用点（见头文件契约）：预取消时 execute 从未运行，须在此补推 cancelled 结果，
    // 否则 scheduler 收不到结果会泄漏 m_tasks 条目与 dispatched 插槽；execute 后取消时
    // _pushResult 幂等跳过。标记 cancelled 兼顾两条路径——execute 已成功完成的任务不会
    // 进入 onCancel（pool 仅在 !success && cancelled 时调用）。
    m_cancelled = true;
    m_success = false;
    _pushResult();
}

void MeshBuildTask::_pushResult()
{
    // 幂等：execute（含异常出口）与 onCancel 都可能调用，全生命周期只推一次。
    if (m_resultPushed) {
        return;
    }
    m_resultPushed = true;

    auto queue = m_resultQueue.lock();
    if (!queue) {
        // scheduler 已析构（结果队列已释放），晚到的回调直接丢弃。
        return;
    }

    MeshWorkerResult result;
    result.chunkId = m_chunkId;
    result.taskId = m_taskId;
    result.solidMesh = std::move(m_solidMesh);
    result.transparentMesh = std::move(m_transparentMesh);
    result.success = m_success;
    result.cancelled = m_cancelled;
    queue->push(std::move(result));
}

} // namespace mc::client
