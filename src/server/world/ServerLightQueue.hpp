/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {
class WorldLightManager;
}

namespace mc::server {

class ServerWorld;

/**
 * @brief 运行时方块变更光照延迟队列
 *
 * 主线程单线程版的光照批处理队列。setBlockState 时仅入队标记脏，
 * tick 时按区块批量调 WorldLightManager::checkBlocks 一次性传播，
 * 避免每次方块变更都触发一次完整的 setupCaches/destroyCaches。
 *
 * 本阶段（③-0）仍在主线程执行，不引入并发；后续阶段才会把传播
 * 计算搬到 worker 线程。
 */
class ServerLightQueue {
public:
    ServerLightQueue() = default;

    /**
     * @brief 入队一个方块变更
     *
     * 仅记录变更坐标，不执行任何光照传播。同一坐标多次入队自动去重，
     * 同一区块内多个坐标在 drain 时合并为一次 blocksChangedInChunk 调用。
     *
     * @param x 方块世界 X 坐标
     * @param y 方块世界 Y 坐标
     * @param z 方块世界 Z 坐标
     */
    void queueBlockChange(i32 x, i32 y, i32 z);

    /**
     * @brief 排空队列并批量处理
     *
     * 按区块分组，对每个区块调用一次 WorldLightManager::checkBlocks，
     * 一次 setupCaches/destroyCaches 处理该区块内全部待处理坐标。
     * 处理完毕后清空队列。
     *
     * @param manager 光照管理器（执行实际传播）
     */
    void drainAndProcess(WorldLightManager& manager);

    /**
     * @brief 排空队列并提交 worker 异步传播（生产路径，③-1）
     *
     * 主线程 tick 调用：对每个待处理区块构造 RuntimeLightTask，经
     * ServerChunkManager::radiusAwareExecutor() 提交到区域互斥池（writeRadius=2），
     * worker 线程执行传播，完成后 dirty section 入 ServerWorld flush 队列，
     * 主线程下一 tick flush visible。实现 Moonrise "compute on worker, flush on main"。
     *
     * 若 executor 为 nullptr（启动早期/测试环境未注入 worker 池），fallback 调
     * 同步 drainAndProcess(manager)，保证正确性不依赖 worker 池可用性。
     *
     * @param world 服务端世界（取 chunkManager/lightManager、worker 任务 flush 目标）
     */
    void drainAndProcess(ServerWorld& world);

    /**
     * @brief 队列是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return m_chunkTasks.empty(); }

    /**
     * @brief 获取待处理区块数
     */
    [[nodiscard]] size_t pendingChunkCount() const noexcept { return m_chunkTasks.size(); }

private:
    /**
     * @brief 单个区块的待处理方块变更集合
     *
     * 坐标去重使用 BlockPos::asLong() 作为键，避免 toId() 的 8 位 Y
     * 掩码在 Y=±320 高度区间产生碰撞。
     */
    struct _ChunkLightTasks {
        std::unordered_set<i64> changedPositionLongs;
    };

    /// 按 ChunkPos::toId() 分组的待处理任务
    std::unordered_map<u64, _ChunkLightTasks> m_chunkTasks;
};

} // namespace mc::server
