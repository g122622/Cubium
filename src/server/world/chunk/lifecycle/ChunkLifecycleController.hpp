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
#include "server/world/chunk/diagnostics/ChunkDiagnostics.hpp"
#include "server/world/chunk/integration/WorldChunkLifecycle.hpp"
#include "server/world/chunk/lifecycle/unloading/ChunkUnloadController.hpp"
#include "server/world/scheduling/AreaTaskScheduler.hpp"

namespace mc::server {

class ServerWorld;

/** @brief 区块生命周期用例编排；状态和执行细节归各能力模块所有。 */
class ChunkLifecycleController {
public:
    using Holder = world::chunk::ChunkHolder;
    using ChunkCallback = std::function<void(bool, ChunkData*)>;
    /** @brief 每世界复用区域执行器；独立生成显式持有自己的执行器。 */
    ChunkLifecycleController(ServerWorld* world, std::unique_ptr<IChunkGenerator> generator);
    ~ChunkLifecycleController();
    /** @brief 停止需求和重调度，供世界关闭各任务生产者时使用。 */
    void stopScheduling();
    /** @brief 排空执行器、交付失败请求并释放全部实例。 */
    void shutdown();
    /** @brief 装配外部计算池，保持所有内部服务地址稳定。 */
    void setWorkerPool(util::UniversalWorkerPool* pool);
    /** @brief 主线程同步请求仅泵送来源完成，不执行世界副作用。 */
    ChunkData* requestChunkSync(ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& targetStatus);
    /** @brief 提交带回调或 promise 的阶段需求。 */
    void submit(ChunkCoord x,
        ChunkCoord z,
        const world::chunk::ChunkStatus& targetStatus,
        ChunkCallback callback,
        std::shared_ptr<std::promise<ChunkData*>> promise);
    /** @brief 距离图传播与延迟需求协调。 */
    void processTicketUpdates();
    /** @brief 交接来源完成，保留入口快照与时间预算。 */
    void processPendingChunkTasks();
    /** @brief 主线程按固定阶段推进需求、激活、退场和回收。 */
    void tick();
    /** @brief 处理一个票据级别变化。 */
    void onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);
    /** @brief 无世界副作用的已发布区块遍历。 */
    void forEachLoadedChunk(const std::function<bool(ChunkData&)>& callback) const;
    /** @brief 世界模拟资格过滤后的区块遍历。 */
    void forEachTickingChunk(const std::function<bool(ChunkData&)>& callback) const;
    world::chunk::ChunkDemandController demand;
    ChunkRegistry registry;
    WorldChunkLifecycle integration;
    ChunkStepExecutor steps;
    std::unique_ptr<AreaTaskScheduler> standaloneArea;
    AreaTaskScheduler& area;
    ChunkGenerationScheduler scheduler;
    ChunkSourceLoader loader;
    ChunkUnloadController unloader;

private:
    void _advanceChunkState(Holder& holder, const Holder::EnqueueDecision& decision);
    void _resolveStorageForScheduling(Holder& holder);
    void _scheduleGeneration(Holder& holder);
    void _notifyStorageReady(Holder& holder);
    void _drainDeferredTicketLevels();
    void _publishFull(Holder& holder, world::chunk::ChunkPrimer& primer);

    util::UniversalWorkerPool* computeExecutor() const
    {
        return m_workerPool.load(std::memory_order::acquire);
    }

    static u64 posToKey(ChunkCoord x, ChunkCoord z)
    {
        return ChunkPos(x, z).toId();
    }

    ServerWorld* m_world;
    std::atomic<util::UniversalWorkerPool*> m_workerPool{nullptr};
    std::atomic<bool> m_shuttingDown{false};
    std::unordered_map<u64, i32> m_deferredTicketLevels;
};
} // namespace mc::server
