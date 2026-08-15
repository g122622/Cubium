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

#include "RuntimeLightTask.hpp"

#include "ServerWorld.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>
#include <fmt/format.h>

using namespace mc::trace;

namespace mc::server {

RuntimeLightTask::RuntimeLightTask(
    ServerWorld& world, ChunkCoord chunkX, ChunkCoord chunkZ, std::vector<BlockPos> positions)
    : m_world(&world)
    , m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_positions(std::move(positions))
    , m_provider(world, chunkX, chunkZ)
{}

bool RuntimeLightTask::execute(const std::atomic<bool>& abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "RuntimeLightTask::execute",
        "chunk",
        fmt::format("({}, {})", m_chunkX, m_chunkZ),
        "positions",
        m_positions.size(),
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(m_chunkX, m_chunkZ).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    // 任务可能被取消（关服/区块卸载），检查后安全跳过
    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    // worker 线程执行传播：经 TLS 池获取引擎（无引擎级锁），直接调 blocksChangedInChunk。
    // 区域锁 writeRadius=2 串行化重叠 5×5 区域的 nibble 写（SWMRNibbleArray 单写者语义）。
    // provider 的 markLightChanged 收集 dirty section 而非触碰主线程回调。
    // 运行时方块变更不改段空状态，changedSections 留空。
    WorldLightManager* lightManager = m_world->lightManager();
    MC_ASSERT_RELEASE(lightManager != nullptr);

    // 对 provider 保活的 5×5 活体区块逐个取共享锁，覆盖下方 blocksChangedInChunk 全程
    // （setupCaches 缓存 const ChunkSection* 后 performLightIncrease/Decrease 仍读这些
    // 指针指向的 PalettedContainer）。共享锁与主线程 setBlockState 等写方法（持独占锁）
    // 串行，消除并发改写 PalettedContainer 内部缓冲致 worker 读已释放缓冲崩溃。
    // 共享锁之间不互斥，多 worker 可同时读同一区块，无死锁。详见
    // ChunkData::lockForLightRead。
    std::vector<std::shared_lock<std::shared_mutex>> sectionReadLocks;
    const auto& keepalivePtrs = m_provider.keepaliveChunkPtrs();
    sectionReadLocks.reserve(keepalivePtrs.size());
    for (ChunkData* chunkData : keepalivePtrs) {
        if (chunkData != nullptr) {
            sectionReadLocks.push_back(chunkData->lockForLightRead());
        }
    }

    const std::vector<bool> changedSections;

    if (lightManager->hasSkyLight()) {
        auto* skyEngine = WorldLightManager::acquireSkyLightEngine();
        skyEngine->blocksChangedInChunk(&m_provider, m_chunkX, m_chunkZ, m_positions, changedSections);
        WorldLightManager::releaseSkyLightEngine(skyEngine);
    }
    if (lightManager->hasBlockLight()) {
        auto* blockEngine = WorldLightManager::acquireBlockLightEngine();
        blockEngine->blocksChangedInChunk(&m_provider, m_chunkX, m_chunkZ, m_positions, changedSections);
        WorldLightManager::releaseBlockLightEngine(blockEngine);
    }

    // 取出 worker 收集的 dirty section，入主线程 flush 队列。
    // 主线程下一 tick _drainPendingLightFlushes 时调真正的 markLightChanged。
    // 即使任务在传播后被取消，dirty section 仍入队——主线程 markLightChanged 幂等，无副作用。
    auto dirtySections = m_provider.takeDirtySections();
    if (!dirtySections.empty()) {
        m_world->_enqueueLightFlush(std::move(dirtySections));
    }

    return true;
}

std::string RuntimeLightTask::description() const
{
    return fmt::format("RuntimeLightTask(chunk=({},{}), positions={})", m_chunkX, m_chunkZ, m_positions.size());
}

} // namespace mc::server
