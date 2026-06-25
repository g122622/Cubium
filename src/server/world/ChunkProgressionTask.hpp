/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING THE LIABILITY OF ANY KIND, EXPRESS OR
 * IMPLIED, BUT NOT LIMITED TO THE KIND OF EXPRESS OR LIABILITY,
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "server/world/StaticChunkCache2D.hpp"
#include <atomic>

namespace mc::world::chunk {
class ChunkPrimer;
}

namespace mc {
class WorldGenRegion;
}

namespace mc::server {

class ChunkTaskScheduler;
class ServerChunkManager;

/**
 * @brief 单状态推进任务（对齐 Moonrise ChunkProgressionTask + ChunkUpgradeGenericStatusTask）
 *
 * 每个 ChunkProgressionTask 实例负责把一个区块从 currentGenStatus 推进**一步**到
 * scheduledStatus（= currentGenStatus.next()）。任务在执行器线程中：
 *   1. 从 holder 取出当前可变 ChunkPrimer（getCurrentChunk，EMPTY 加载时新建空 Primer）
 *   2. 从 m_neighbours 构建可变 WorldGenRegion（StaticChunkCache2D<ChunkPrimer*>，邻居为可变 ChunkPrimer）
 *   3. 调用 ServerChunkManager._executeStepTask(primer, scheduledStatus, region)（修改同一 primer）
 *   4. primer.setPersistedStatus(scheduledStatus); primer.setChunkStatus(scheduledStatus)
 *   5. 回调 ChunkTaskScheduler.onChunkGenComplete(holder, scheduledStatus)（推进 currentGenStatus，primer 同一对象）
 *
 * 关键不变量（对齐 Moonrise）：m_neighbours 中的每个 ChunkPrimer 都由 ChunkTaskScheduler.checkNeighbour
 * 保证已达到该步所需的 requiredStatus，因此 WorldGenRegion 的访问窗口绝不会出现 nullptr 或低状态区块。
 * 邻居是可变 ChunkPrimer（与中心同类型），FEATURES/LIGHT 写邻居合法，并发安全由 ReentrantAreaLock 保证。
 */
class ChunkProgressionTask : public util::ITask {
public:
    /**
     * @brief 构造单状态推进任务
     *
     * @param scheduler 调度器（用于 onChunkGenComplete 回调）
     * @param manager 所属 ServerChunkManager（用于 _executeStepTask / _finalizeGeneratedChunkSync）
     * @param x 区块 X
     * @param z 区块 Z
     * @param toStatus 目标状态（= currentGenStatus.next()，本任务只推进一步）
     * @param neighbours 已确认就绪的邻居可变 ChunkPrimer 缓存（中心为 (x, z)，半径 = step.neighbourReadRadius()）
     * @param writeRadius 区域互斥写入半径（ChunkStep::blockStateWriteRadius()，负值视为 0）
     */
    ChunkProgressionTask(ChunkTaskScheduler& scheduler,
        ServerChunkManager& manager,
        ChunkCoord x,
        ChunkCoord z,
        const ChunkStatus& toStatus,
        StaticChunkCache2D<mc::world::chunk::ChunkPrimer*> neighbours,
        i32 writeRadius);

    ~ChunkProgressionTask() override = default;

    // ITask 接口
    bool execute(const std::atomic<bool>& abortSignal) override;
    void onCancel() override;
    util::TaskType type() const override { return util::TaskType::ChunkGenerate; }
    std::string description() const override;
    const char* traceCategory() const override { return "world.chunk_gen"; }

    [[nodiscard]] ChunkCoord x() const noexcept { return m_x; }
    [[nodiscard]] ChunkCoord z() const noexcept { return m_z; }
    [[nodiscard]] const ChunkStatus& toStatus() const noexcept { return *m_toStatus; }
    [[nodiscard]] i32 writeRadius() const noexcept { return m_writeRadius; }

private:
    /**
     * @brief 执行存档加载任务（EMPTY 状态：从存档加载或新建空 Primer）
     *
     * EMPTY 推进不调用 IChunkGenerator，只解析存档来源。完成后调用 onChunkGenComplete(EMPTY)。
     */
    bool executeEmptyLoad(const std::atomic<bool>& abortSignal);

    /**
     * @brief 执行普通状态推进任务
     *
     * 调用 IChunkGenerator 的对应阶段方法，推进 primer 状态。
     */
    bool executeStatusStep(const std::atomic<bool>& abortSignal);

    ChunkTaskScheduler& m_scheduler;
    ServerChunkManager& m_manager;
    ChunkCoord m_x;
    ChunkCoord m_z;
    const ChunkStatus* m_toStatus;
    StaticChunkCache2D<mc::world::chunk::ChunkPrimer*> m_neighbours;
    i32 m_writeRadius;
};

} // namespace mc::server
