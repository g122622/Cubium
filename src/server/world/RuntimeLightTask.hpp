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
 */

#pragma once

#include "RuntimeLightingProvider.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/block/BlockPos.hpp"
#include <string>
#include <vector>

namespace mc {
class WorldLightManager;
}

namespace mc::server {

class ServerWorld;

/**
 * @brief 运行时方块变更光照传播 worker 任务
 *
 * ③-1 阶段：ServerLightQueue::drainAndProcess(ServerWorld&) 在主线程 tick 时
 * 对每个待处理区块构造本任务，提交到 ServerWorkerPool 区域互斥池
 * （writeRadius=2，与 LIGHT 生成阶段同池）。worker 线程执行：
 *
 * 1. 调 WorldLightManager::checkBlocksWithProvider（持 m_mutex 串行化 nibble 写），
 *    传入任务持有的 RuntimeLightingProvider。propagateBlockChanges/updateVisible
 *    在 worker 完成；updateVisible 内部的 markLightChanged 经 provider 收集到
 *    m_dirtySections 而非触碰主线程独占回调。
 * 2. 取出 provider 的 dirty section 列表，经 ServerWorld::_enqueueLightFlush
 *    入主线程 flush 队列。主线程下一 tick drain 时调真正的 markLightChanged
 *    （_syncLightDataToChunk + m_onLightChanged 网络包）。
 *
 * 生命周期：m_provider 作为成员，5×5 shared_ptr 保活与任务绑定——worker 执行
 * 期间区块不被释放，任务析构自动释放引用。区域锁 writeRadius=2 防止多个光照
 * 任务并发写重叠区域 nibble（第二层保护，第一层是 m_mutex）。
 */
class RuntimeLightTask : public util::ITask {
public:
    /**
     * @brief 构造运行时光照传播任务
     *
     * 主线程构造（drain 入队时）：provider 在此构造并完成 5×5 保活。
     *
     * @param world 服务端世界（_enqueueLightFlush 目标、provider 构造参数）
     * @param manager 光照管理器（checkBlocksWithProvider 执行实际传播）
     * @param chunkX 中心区块 X
     * @param chunkZ 中心区块 Z
     * @param positions 待处理的方块坐标列表（同区块去重后）
     */
    RuntimeLightTask(ServerWorld& world,
        WorldLightManager& manager,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        std::vector<BlockPos> positions);

    // === ITask 实现 ===
    bool execute(const std::atomic<bool>& abortSignal) override;
    util::TaskType type() const override { return util::TaskType::Custom; }
    std::string description() const override;
    const char* traceCategory() const override { return "lighting_runtime"; }

private:
    ServerWorld* m_world;
    WorldLightManager* m_manager;
    ChunkCoord m_chunkX;
    ChunkCoord m_chunkZ;
    std::vector<BlockPos> m_positions;

    /// 运行时 provider（持有 5×5 shared_ptr 保活、收集 dirty section）
    RuntimeLightingProvider m_provider;
};

} // namespace mc::server
