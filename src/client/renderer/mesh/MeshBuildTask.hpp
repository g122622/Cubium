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

#include "MeshDataPool.hpp"
#include "MeshResultQueue.hpp"
#include "MeshWorkerTypes.hpp"
#include "common/core/Types.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>
#include <atomic>
#include <memory>

namespace mc::client {

/**
 * @brief 网格构建任务（ITask 子类）
 *
 * 迁移到 UniversalWorkerPool 后取代原 MeshWorkerPool 的内部任务体。
 * 携带 chunkData/neighbors 状态，execute 内调 ChunkMesher::generateSplitMesh，
 * 结果通过 weak_ptr<MeshResultQueue> 推回主线程。
 *
 * 取消令牌由调度器经 pool.submit 的 abortSignal 参数传入（ITask 契约：execute 收到的
 * abortSignal 即该令牌），task 自身不持有副本——与原 MeshWorkerPool 检查 task.abortSignal
 * 的语义一致，且避免双份令牌不同步。
 *
 * 纵深防护：持有 weak_ptr<MeshResultQueue> + shared_ptr<MeshDataPool>。
 * scheduler 析构后晚到的回调（池晚于 scheduler shutdown）lock 结果队列失败即丢弃，安全；
 * MeshDataPool 由 shared_ptr 延长生命周期，acquire/recycle 永不悬垂。
 *
 * 结果推送契约：UniversalWorkerPool 在两处可能调用 onCancel——
 *   1. 预检查发现任务已取消（UniversalWorkerPool.cpp:522），此时 execute 从未运行；
 *   2. execute 返回 false 且任务已取消（UniversalWorkerPool.cpp:570），此时 execute 已推过结果。
 * 故 execute（成功/失败/取消/异常各出口）与 onCancel 都调 _pushResult()，由 m_resultPushed
 * 保证全生命周期恰好推一次。预取消路径由 onCancel 负责推 cancelled 结果，否则 scheduler 的
 * m_tasks 表会因收不到结果而泄漏条目（cancelChunk/cancelAll 在 submit 后才置 abortSignal，
 * 若任务尚在队列则被预检查短路，必须靠 onCancel 兜底推结果）。execute 与 onCancel 由同一
 * worker 线程串行调用（预取消路径只调 onCancel，执行后路径先 execute 后 onCancel），故
 * m_resultPushed 为普通 bool 即可，无需原子。
 */
class MeshBuildTask : public ::mc::util::ITask {
public:
    MeshBuildTask(ChunkId chunkId,
        u64 taskId,
        std::shared_ptr<const ChunkData> chunkData,
        std::array<std::shared_ptr<const ChunkData>, 6> neighbors,
        std::shared_ptr<MeshDataPool> dataPool,
        std::weak_ptr<MeshResultQueue> resultQueue);

    bool execute(const std::atomic<bool>& abortSignal) override;
    void onCancel() override;

    [[nodiscard]] ::mc::util::TaskType type() const override { return ::mc::util::TaskType::Custom; }
    [[nodiscard]] std::string description() const override;

private:
    /// 组装 MeshWorkerResult 并推入结果队列（lock 失败则丢弃）。幂等：全生命周期只推一次。
    void _pushResult();

    ChunkId m_chunkId;
    u64 m_taskId = 0;
    std::shared_ptr<const ChunkData> m_chunkData;
    std::array<std::shared_ptr<const ChunkData>, 6> m_neighbors;
    std::shared_ptr<MeshDataPool> m_dataPool;
    std::weak_ptr<MeshResultQueue> m_resultQueue;

    MeshData m_solidMesh;
    MeshData m_transparentMesh;
    bool m_success = false;
    bool m_cancelled = false;
    bool m_resultPushed = false;
};

} // namespace mc::client
