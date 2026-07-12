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
 */

#pragma once

#include "RuntimeLightingProvider.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <string>

namespace mc::server {

class ServerWorld;

/**
 * @brief 区块加载光照 worker 任务（对齐 Moonrise ThreadedLevelLightEngine）
 *
 * ③-2b 阶段：区块加载后的光照初始化从主线程同步搬到 worker 线程。原
 * LightSyncManager::initializeChunkLighting 在主线程直接调引擎写 nibble，
 * 与 worker RuntimeLightTask 写同一区块 nibble 的 updating 侧竞争（SWMRNibbleArray
 * 非原子单写者语义）。效仿 Moonrise ThreadedLevelLightEngineMixin：所有光照写
 * 操作统一经同一 ServerWorkerPool 区域互斥池（writeRadius=2），重叠 5×5 区域
 * 的 nibble 写必被区域锁串行 → 可安全删 m_mutex。
 *
 * 执行流程（对齐 Moonrise ChunkLightTask.java:154-165）：
 * - if 分支（isLightCorrect && status∈{Generated, Loaded}）：区块已光照，
 *   只需 forceHandleEmptySectionChanges + checkChunkEdges（廉价重载）。
 * - else 分支（需完整光照）：setLightCorrect(false) → updateEmptinessMap +
 *   updateSectionStatus 循环 → light(needsEdgeChecks=true) → setLightCorrect(true)。
 *
 * 引擎经 WorldLightManager TLS 池获取（acquire/release 配对，无引擎级锁）。
 * 完成后：取 provider 收集的 dirty section 入主线程 flush 队列；入区块发送续延
 * 队列（serialize 在主线程 flush 后读已发布 nibble，保证客户端不收全黑区块）。
 *
 * 生命周期：m_provider 作为成员，5×5 shared_ptr 保活与任务绑定——worker 执行
 * 期间区块不被释放。LIGHT 票据保活由 ServerWorld::enqueueChunkLoadLight 在构造
 * 本任务前 add，发送续延 drain 时 remove（见 onCancel 处理取消路径）。
 */
class ChunkLoadLightTask : public util::ITask {
public:
    /**
     * @brief 构造区块加载光照任务
     *
     * 主线程构造（enqueueChunkLoadLight 内）：provider 在此构造并完成 5×5 保活。
     * LIGHT 票据由调用方（enqueueChunkLoadLight）在构造前 add，本任务不负责 add。
     *
     * @param world 服务端世界（_enqueueLightFlush/_enqueueChunkSend 目标、provider 构造参数）
     * @param chunkX 中心区块 X
     * @param chunkZ 中心区块 Z
     */
    ChunkLoadLightTask(ServerWorld& world, ChunkCoord chunkX, ChunkCoord chunkZ);

    // === ITask 实现 ===
    bool execute(const std::atomic<bool>& abortSignal) override;
    void onCancel() override;
    util::TaskType type() const override { return util::TaskType::Custom; }
    std::string description() const override;
    const char* traceCategory() const override { return "lighting_chunk_load"; }

private:
    ServerWorld* m_world;
    ChunkCoord m_chunkX;
    ChunkCoord m_chunkZ;

    /// 运行时 provider（持有 5×5 shared_ptr 保活、收集 dirty section）
    RuntimeLightingProvider m_provider;
};

} // namespace mc::server
