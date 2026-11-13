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
#include "server/world/chunk/lifecycle/generation/ChunkGenerationScheduler.hpp"
#include "server/world/chunk/lifecycle/generation/ChunkStepExecutor.hpp"
#include "server/world/chunk/lifecycle/loading/ChunkSourceLoader.hpp"
#include <unordered_set>

namespace mc::server {
/** @brief 事件驱动的候选回收、快照保存、身份复核与退避。 */
class ChunkUnloadController {
public:
    using Holder = world::chunk::ChunkHolder;
    /** @brief 生命周期依赖由装配者显式连接，不访问区块门面。 */
    ChunkUnloadController(ChunkRegistry& registry,
        world::chunk::ChunkDemandController& demand,
        ChunkGenerationScheduler& scheduler,
        ChunkStepExecutor& steps,
        ChunkSourceLoader& loader,
        IChunkWorldLifecycle& integration,
        std::function<world::storage::SingleLevelStorageManager*()> storage,
        DimensionId dimension,
        std::function<bool(u64)> deferred);
    /** @brief 发起一次退场；异步保存完成后由 drain 收尾。 */
    void request(ChunkCoord x, ChunkCoord z);
    /** @brief 主线程按预算接收保存完成消息。 */
    void drain();
    /** @brief 状态安全性变化时更新候选资格，可在 worker 调用。 */
    void updateCandidate(Holder& holder);
    /** @brief 主线程按批量与墙钟预算开始回收。 */
    void processCandidates();
    /** @brief 主线程处理已到期的回收冷却。 */
    void expireCooldowns();
    /** @brief 票据回调同步冷却镜像，供 worker 查询。 */
    void noteCooldown(ChunkCoord x, ChunkCoord z);
    /** @brief 停止候选更新并拒绝后台完成交接。 */
    void stop();
    /** @brief 执行器排空后释放队列。 */
    void clear();

    /** @brief 基准测量期间保留显式生成的区块。 */
    void suppressAutomatic(bool suppress)
    {
        m_suppressAutoUnload = suppress;
    }

private:
    void _finalizeUnloadAfterSave(
        ChunkCoord x, ChunkCoord z, DimensionId dimension, std::shared_ptr<Holder> holder, bool saveSucceeded);
    bool _isGenerationIdle();
    void _enqueueIdleHaloReclaim();
    bool _isUnloadSuppressed(const Holder& holder) const;
    bool _isUnloadCandidate(const Holder& holder);
    bool _tryPopUnloadCandidate(u64& key);

    static u64 posToKey(ChunkCoord x, ChunkCoord z)
    {
        return ChunkPos(x, z).toId();
    }

    struct PendingUnloadFinish {
        ChunkCoord x = 0, z = 0;
        DimensionId dimension{};
        std::shared_ptr<Holder> lifecycleHolder;
        bool saveSucceeded = true;
    };

    ChunkRegistry& m_registry;
    world::chunk::ChunkDemandController& m_ticketManager;
    ChunkGenerationScheduler& m_scheduler;
    ChunkStepExecutor& m_steps;
    ChunkSourceLoader& m_loader;
    IChunkWorldLifecycle& m_integration;
    std::function<world::storage::SingleLevelStorageManager*()> m_storage;
    DimensionId m_dimension;
    std::function<bool(u64)> m_deferred;
    mutable std::mutex m_unloadStateMutex;
    std::shared_ptr<ChunkCompletionInbox<PendingUnloadFinish>> m_finished =
        std::make_shared<ChunkCompletionInbox<PendingUnloadFinish>>();
    std::unordered_set<u64> m_unloadSaveInProgress, m_unloadCooldownChunks;
    std::deque<u64> m_unloadQueue;
    std::unordered_set<u64> m_inUnloadQueue;
    mutable std::mutex m_unloadQueueMutex;
    std::atomic<bool> m_shuttingDown{false}, m_idleHaloReclaimArmed{false}, m_suppressAutoUnload{false};
    static constexpr size_t MIN_CHUNK_UNLOAD_COUNT = 50;
    static constexpr f64 MIN_CHUNK_UNLOAD_FRACTION = 0.05;
    static constexpr u64 UNLOAD_IDLE_DEBOUNCE_MS = 1000;
};
} // namespace mc::server
