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
#include "server/world/chunk/base/completion/IChunkWorldLifecycle.hpp"
#include "server/world/chunk/lifecycle/demand/ChunkDemandController.hpp"
#include "server/world/chunk/lifecycle/state/ChunkRegistry.hpp"
#include "server/world/gen/spawn/WorldGenSpawner.hpp"
#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace mc::server {
class ServerWorld;

} // namespace mc::server

namespace mc {
class WorldGenRegion;
}

namespace mc::server {
/** @brief 世界侧区块激活、后处理和退场。世界副作用只在主线程执行。 */
class WorldChunkLifecycle final : public IChunkWorldLifecycle {
public:
    using LoadedCallback = std::function<void(ChunkCoord, ChunkCoord)>;
    using SpawnCallback = std::function<void(const std::vector<SpawnedEntityData>&)>;
    /** @brief 绑定世界与状态查询；world 为空时仅派发独立生成回调。 */
    WorldChunkLifecycle(ServerWorld* world, ChunkRegistry& registry, world::chunk::ChunkDemandController& demand);
    /** @brief worker 只提交完成消息，主线程 drain 后执行激活。 */
    void enqueue(ChunkCoord x, ChunkCoord z, std::vector<SpawnedEntityData>&& entities, bool needsPostProcess) override;
    /** @brief 按入口快照和预算执行世界注册。 */
    void drain();
    /** @brief 进入模拟范围后才注册生成留下的方块刻和流体刻。 */
    void postProcessTicking();
    /** @brief 确认退场后执行唯一一次世界和网络通知。 */
    void notifyUnload(ChunkCoord x, ChunkCoord z) override;
    /** @brief 清理一个生命周期的激活标记，旧完成消息会被身份校验丢弃。 */
    void forget(ChunkCoord x, ChunkCoord z) override;
    /** @brief 排空执行器之后清理未执行通知。 */
    void clear();
    /** @brief 是否仍有等待主线程接收的激活消息。 */
    bool pending() const override;
    /** @brief 推进有玩家追踪区块的居住时间。 */
    void incrementInhabitedTime();
    /** @brief 给单次生成区域显式填入世界运行时信息。 */
    void configureRegion(WorldGenRegion& region) const;
    /** @brief 在区域执行保护下执行生成 LIGHT 步骤。 */
    void lightGeneration(world::chunk::ChunkPrimer& chunk, WorldGenRegion& region);
    /** @brief 当前维度；独立生成使用显式构造的主世界上下文。 */
    DimensionId dimension() const;

    /** @brief 连接主线程激活通知。 */
    void setLoadedCallback(LoadedCallback callback)
    {
        m_chunkLoadedCallback = std::move(callback);
    }

    /** @brief 连接退场通知。 */
    void setUnloadedCallback(LoadedCallback callback)
    {
        m_chunkUnloadedCallback = std::move(callback);
    }

    /** @brief 连接独立生成实体接收方。 */
    void setSpawnCallback(SpawnCallback callback)
    {
        m_entitySpawnCallback = std::move(callback);
    }

    /** @brief 向独立生成接收方交付实体；生产路径经主线程激活消息。 */
    void deliverStandaloneEntities(const std::vector<SpawnedEntityData>& entities);

private:
    void _postProcessChunk(ChunkData& chunk);
    void _forEachTicking(const std::function<bool(ChunkData&)>& callback);

    static u64 posToKey(ChunkCoord x, ChunkCoord z)
    {
        return ChunkPos(x, z).toId();
    }

    struct PendingPostProcess {
        ChunkCoord x = 0;
        ChunkCoord z = 0;
        std::vector<SpawnedEntityData> spawnedEntities;
        bool needsPostProcess = false;
        std::shared_ptr<world::chunk::ChunkHolder> holder;
    };

    ServerWorld* m_world;
    ChunkRegistry& m_registry;
    world::chunk::ChunkDemandController& m_ticketManager;
    LoadedCallback m_chunkLoadedCallback, m_chunkUnloadedCallback;
    SpawnCallback m_entitySpawnCallback;
    mutable std::mutex m_pendingPostProcessMutex;
    std::deque<PendingPostProcess> m_pendingPostProcess;
    std::unordered_set<u64> m_postProcessedChunks, m_pendingPostProcessChunks;
    static constexpr size_t PENDING_POST_PROCESS_WARN_THRESHOLD = 256;
};
} // namespace mc::server
