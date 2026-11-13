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
#include "common/core/Result.hpp"
#include "common/world/chunk/IChunkManager.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/chunk/base/pipeline/ChunkStatus.hpp"
#include "server/world/chunk/diagnostics/ChunkDiagnosticSnapshot.hpp"
#include "server/world/chunk/lifecycle/demand/base/ChunkLoadTicket.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/spawn/WorldGenSpawner.hpp"
#include <functional>
#include <future>
#include <memory>
#include <vector>

namespace mc::util {
class UniversalWorkerPool;
}

namespace mc::server {
class ServerWorld;
class ChunkLifecycleController;
class ChunkSystemTestAccess;

/** @brief 服务端区块子系统唯一门面；内部状态和调度器不向调用方暴露。 */
class ServerChunkManager : public world::chunk::IChunkManager {
public:
    using ChunkCallback = std::function<void(bool, ChunkData*)>;
    using EntitySpawnCallback = std::function<void(const std::vector<SpawnedEntityData>&)>;
    using ChunkLoadedCallback = std::function<void(ChunkCoord, ChunkCoord)>;
    using TrackingChangeCallback = std::function<void(PlayerId, ChunkCoord, ChunkCoord, bool)>;
    using ChunkCacheCenterCallback = std::function<void(PlayerId, ChunkCoord, ChunkCoord)>;
    /** @brief 创建一个世界的区块运行时。 */
    ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator);
    /** @brief 创建独立生成运行时，执行器和实体接收方由调用者显式装配。 */
    explicit ServerChunkManager(std::unique_ptr<IChunkGenerator> generator);
    ~ServerChunkManager() override;
    ServerChunkManager(const ServerChunkManager&) = delete;
    ServerChunkManager& operator=(const ServerChunkManager&) = delete;
    /** @brief 当前线程借用已发布区块，不发起加载。 */
    ChunkData* tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) override;
    /** @brief 当前线程借用已发布区块，不发起加载。 */
    const ChunkData* tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) const override;
    /** @brief 取得共享所有权以保活，数据并非不可变快照。 */
    std::shared_ptr<ChunkData> tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z);
    /** @brief 取得共享所有权以保活，读取仍须遵守执行同步契约。 */
    std::shared_ptr<const ChunkData> tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z) const;
    /** @brief 查询 FULL 数据是否已经发布。 */
    bool hasChunkInMem(ChunkCoord x, ChunkCoord z) const override;
    /** @brief 关闭并释放区块子系统。 */
    void shutdown();
    /** @brief 停止区块需求推进，供世界关闭任务生产者。 */
    void stopScheduling();
    /** @brief 注入由服务器持有的计算池。 */
    void setWorkerPool(util::UniversalWorkerPool* pool);
    /** @brief 主线程同步取得指定阶段数据，仅推进来源完成。 */
    ChunkData* requestChunkSync(ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& status);
    /** @brief 主线程同步取得 FULL 区块。 */
    ChunkData* requestFullChunkSync(ChunkCoord x, ChunkCoord z) override;
    /** @brief 提交指定阶段需求，完成后交付数据；世界副作用延后至 tick。 */
    std::future<ChunkData*> requestChunkAsync(ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& status);
    /** @brief 提交指定阶段需求，完成后交付数据；世界副作用延后至 tick。 */
    void requestChunkAsync(ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& status, ChunkCallback callback);
    /** @brief 异步发起区块退场，保存后重新复核当前需求。 */
    void requestChunkUnload(ChunkCoord x, ChunkCoord z);
    /** @brief 以世界坐标更新玩家加载和追踪中心。 */
    void updatePlayerPosition(PlayerId player, f64 x, f64 z);
    /** @brief 移除玩家来源与追踪范围。 */
    void removePlayer(PlayerId player);
    /** @brief 添加或撤销指定区块的持久强制加载需求。 */
    void forceChunk(ChunkCoord x, ChunkCoord z, bool force);
    /** @brief 保活区块直至加载光照与发送完成，并推进票据需求。 */
    void addLightTicket(ChunkCoord x, ChunkCoord z);
    /** @brief 释放加载光照保活需求，由后续统一传播。 */
    void removeLightTicket(ChunkCoord x, ChunkCoord z);
    /** @brief 设置服务端视距上限，保留各玩家请求视距。 */
    void setViewDistance(i32 distance);
    /** @brief 设置单玩家请求视距，与服务端上限取较小值。 */
    void setPlayerViewDistance(PlayerId player, i32 distance);
    /** @brief 在基准计时阶段保留无持久票据的显式生成区块。 */
    void setSuppressAutoUnloadForBenchmark(bool suppress);
    /** @brief 按预算传播需求，不接收来源结果或执行世界副作用。 */
    void processTicketUpdates();
    /** @brief 传播需求并接收入口处的一批来源完成结果。 */
    void processPendingChunkTasks();
    /** @brief 依次推进需求、来源交接、世界激活和区块回收。 */
    void tick();
    /** @brief 设置独立生成运行时的实体交付回调。 */
    void setEntitySpawnCallback(EntitySpawnCallback callback);
    /** @brief 设置主线程世界注册完成后的通知。 */
    void setChunkLoadedCallback(ChunkLoadedCallback callback);
    /** @brief 设置确认退场时的主线程通知。 */
    void setChunkUnloadedCallback(ChunkLoadedCallback callback);
    /** @brief 连接玩家追踪范围变更通知。 */
    void setTrackingChangeCallback(TrackingChangeCallback callback);
    /** @brief 连接先于追踪进入事件发生的中心变化通知。 */
    void setChunkCacheCenterCallback(ChunkCacheCenterCallback callback);
    /** @brief 查询当前传播结果是否要求保留区块。 */
    bool shouldChunkLoad(ChunkCoord x, ChunkCoord z) const;
    /** @brief 返回服务端视距上限。 */
    i32 viewDistance() const;
    /** @brief 查询已传播的加载级别。 */
    i32 getChunkLevel(ChunkCoord x, ChunkCoord z) const;
    /** @brief 根据模拟距离与显式强票据判断方块模拟资格。 */
    bool isTickingChunk(ChunkCoord x, ChunkCoord z, i32 distance) const;
    /** @brief 查询玩家是否正在追踪指定区块。 */
    bool isPlayerTracking(PlayerId player, ChunkCoord x, ChunkCoord z) const;
    /** @brief 取得指定区块的追踪玩家列表。 */
    std::vector<PlayerId> getTrackingPlayers(ChunkCoord x, ChunkCoord z) const;
    /** @brief 查询指定区块是否有强制加载票据。 */
    bool isForcedChunk(ChunkCoord x, ChunkCoord z) const;
    /** @brief 取得持久强制加载区块列表。 */
    std::vector<ChunkPos> getForcedChunks() const;
    /** @brief 取得已发布的 FULL 区块数量。 */
    size_t loadedChunkCount() const;
    /** @brief 取得当前生命周期实例数量，包含生成邻域。 */
    size_t lifecycleManagerCount() const;
    /** @brief 取得本世界尚未开始的区域任务数量。 */
    size_t pendingTaskCount() const;
    /** @brief 在保活列表上遍历已发布区块。 */
    void forEachLoadedChunk(const std::function<bool(ChunkData&)>& callback);
    /** @brief 在保活列表上遍历已发布区块。 */
    void forEachLoadedChunk(const std::function<bool(const ChunkData&)>& callback) const;
    /** @brief 仅遍历满足当前模拟资格的已发布区块。 */
    void forEachTickingChunk(const std::function<bool(ChunkData&)>& callback);
    /** @brief 取得区块生成算法的领域入口。 */
    IChunkGenerator* generator();
    /** @brief 取得区块生成算法的领域入口。 */
    const IChunkGenerator* generator() const;
    /** @brief 记录未完成需求和不能安全退场的实例。 */
    void _debugDumpStuckHolders();
    /** @brief 暂停生成写入后收集 primer 状态及内存快照。 */
    std::vector<ChunkDiagnosticSnapshot> debugCollectPrimerStats() const;

    /** @brief 注册一个显式加载需求。 */
    template <typename T>
    void registerTicket(
        const world::chunk::ChunkLoadTicketType<T>& type, ChunkCoord x, ChunkCoord z, i32 level, const T& value)
    {
        _registerTicket(ChunkPos(x, z), world::chunk::ChunkLoadTicket(type, level, value));
    }

private:
    /** @brief 把调用边界构造的基础票据值交给需求模块。 */
    void _registerTicket(ChunkPos pos, world::chunk::ChunkLoadTicket ticket);
    std::unique_ptr<ChunkLifecycleController> m_lifecycle;
    friend class ChunkSystemTestAccess;
};
} // namespace mc::server
