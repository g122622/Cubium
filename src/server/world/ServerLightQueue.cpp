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

#include "ServerLightQueue.hpp"
#include "RuntimeLightTask.hpp"
#include "RuntimeLightingProvider.hpp"
#include "ServerWorld.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

namespace mc::server {

namespace {

/// 将世界坐标打包为区块键，与 ChunkPos::toId 一致（X/Z 各 32 位拼接）
[[nodiscard]] u64 _chunkKey(i32 chunkX, i32 chunkZ) noexcept
{
    ChunkPos pos{chunkX, chunkZ};
    return pos.toId();
}

/// 从去重后的 packed long 集合还原 BlockPos 列表
[[nodiscard]] std::vector<BlockPos> _positionsFromLongs(const std::unordered_set<i64>& longs)
{
    std::vector<BlockPos> positions;
    positions.reserve(longs.size());
    for (const i64 posLong : longs) {
        positions.push_back(BlockPos::fromLong(posLong));
    }
    return positions;
}

} // namespace

void ServerLightQueue::queueBlockChange(i32 x, i32 y, i32 z)
{
    const i32 chunkX = x >> world::CHUNK_SHIFT;
    const i32 chunkZ = z >> world::CHUNK_SHIFT;

    // 用 asLong 去重，12 位 Y 可覆盖 ±2048，远超 1.21.11 建筑高度范围，
    // 避免 toId() 8 位 Y 掩码在高建筑维度碰撞
    const i64 posLong = BlockPos::asLong(x, y, z);

    m_chunkTasks[_chunkKey(chunkX, chunkZ)].changedPositionLongs.insert(posLong);
}

void ServerLightQueue::drainAndProcess(ServerWorld& world)
{
    if (m_chunkTasks.empty()) {
        return;
    }

    // 主线程调用：swap 出任务表后清空
    std::unordered_map<u64, _ChunkLightTasks> tasks;
    tasks.swap(m_chunkTasks);

    ServerChunkManager* cm = world.chunkManager();
    WorldLightManager* lm = world.lightManager();

    // 生产路径下 ServerWorld::tick 的 if (m_lightManager) 守卫已保证 lm 非空；
    // lm 为空时无光照管理器可传播，直接丢弃任务（无方块变更能产生光照效果）
    if (lm == nullptr) {
        return;
    }

    // executor 未注入（启动早期/测试环境）：fallback 主线程同步经 TLS 引擎传播。
    // 构造 RuntimeLightingProvider 拿 5×5 keepalive，与 worker 路径同构。
    util::ServerWorkerPool* executor = (cm != nullptr) ? cm->radiusAwareExecutor() : nullptr;
    if (executor == nullptr) {
        const std::vector<bool> changedSections; // 运行时方块变更不改段空状态
        for (auto& [chunkKey, task] : tasks) {
            if (task.changedPositionLongs.empty()) {
                continue;
            }
            const ChunkPos chunkPos = ChunkPos::fromId(chunkKey);
            const i32 chunkX = chunkPos.x;
            const i32 chunkZ = chunkPos.z;
            const auto positions = _positionsFromLongs(task.changedPositionLongs);

            RuntimeLightingProvider provider(world, chunkX, chunkZ);
            if (lm->hasSkyLight()) {
                auto* skyEngine = WorldLightManager::acquireSkyLightEngine();
                skyEngine->blocksChangedInChunk(&provider, chunkX, chunkZ, positions, changedSections);
                WorldLightManager::releaseSkyLightEngine(skyEngine);
            }
            if (lm->hasBlockLight()) {
                auto* blockEngine = WorldLightManager::acquireBlockLightEngine();
                blockEngine->blocksChangedInChunk(&provider, chunkX, chunkZ, positions, changedSections);
                WorldLightManager::releaseBlockLightEngine(blockEngine);
            }

            // fallback 主线程路径：dirty section 直接调 markLightChanged（无需 flush 队列）
            auto dirtySections = provider.takeDirtySections();
            for (const auto& [type, pos] : dirtySections) {
                world.markLightChanged(type, pos);
            }
        }
        return;
    }

    // 生产路径：逐区块构造 RuntimeLightTask，提交到区域互斥池（writeRadius=2）
    // callback 为空：dirty section 在 execute 末尾经 _enqueueLightFlush 入主线程 flush 队列
    for (auto& [chunkKey, task] : tasks) {
        if (task.changedPositionLongs.empty()) {
            continue;
        }
        const ChunkPos chunkPos = ChunkPos::fromId(chunkKey);
        const i32 chunkX = chunkPos.x;
        const i32 chunkZ = chunkPos.z;

        auto taskPtr =
            std::make_unique<RuntimeLightTask>(world, chunkX, chunkZ, _positionsFromLongs(task.changedPositionLongs));
        executor->submit(std::move(taskPtr), /*callback=*/nullptr, chunkX, chunkZ, /*writeRadius=*/2);
    }
}

} // namespace mc::server
