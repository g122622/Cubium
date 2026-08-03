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

#include "RuntimeLightingProvider.hpp"

#include "ServerChunkManager.hpp"
#include "ServerWorld.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/lighting/LightType.hpp"
#include <utility>
#include <vector>

namespace mc::server {

RuntimeLightingProvider::RuntimeLightingProvider(ServerWorld& world, ChunkCoord centerX, ChunkCoord centerZ)
    : m_world(&world)
    , m_centerX(centerX)
    , m_centerZ(centerZ)
{
    // 主线程构造：对 5×5 范围拿 shared_ptr 保活，防止 worker 在途期间区块被卸载释放。
    // chunkManager 在 drain 入队时必然已就位（setBlockState 路径要求世界已 initialize）
    ServerChunkManager* cm = m_world->chunkManager();
    MC_ASSERT_RELEASE(cm != nullptr);

    for (i32 dz = -KEEPALIVE_RADIUS; dz <= KEEPALIVE_RADIUS; ++dz) {
        for (i32 dx = -KEEPALIVE_RADIUS; dx <= KEEPALIVE_RADIUS; ++dx) {
            const i32 idx = _keepaliveIndex(dx, dz);
            m_keepaliveChunks[idx] = cm->tryToGetChunkSharedInMem(m_centerX + dx, m_centerZ + dz);
            // shared_ptr 可能为空（区块未加载/已卸载），对应槽位 nullptr，引擎 relaxed 模式安全跳过
            m_chunkPtrs[idx] = m_keepaliveChunks[idx].get();
        }
    }
}

IChunk* RuntimeLightingProvider::getChunkForLight(ChunkCoord x, ChunkCoord z)
{
    const i32 dx = x - m_centerX;
    const i32 dz = z - m_centerZ;
    if (dx < -KEEPALIVE_RADIUS || dx > KEEPALIVE_RADIUS || dz < -KEEPALIVE_RADIUS || dz > KEEPALIVE_RADIUS) {
        // 5×5 外：运行时传播不应访问更远区块（setupCaches loadTwoRadius=true 仅触达半径2）
        return nullptr;
    }
    return m_chunkPtrs[_keepaliveIndex(dx, dz)];
}

const IChunk* RuntimeLightingProvider::getChunkForLight(ChunkCoord x, ChunkCoord z) const
{
    const i32 dx = x - m_centerX;
    const i32 dz = z - m_centerZ;
    if (dx < -KEEPALIVE_RADIUS || dx > KEEPALIVE_RADIUS || dz < -KEEPALIVE_RADIUS || dz > KEEPALIVE_RADIUS) {
        return nullptr;
    }
    return m_chunkPtrs[_keepaliveIndex(dx, dz)];
}

const BlockState* RuntimeLightingProvider::getBlockStateForLight(const BlockPos& pos) const
{
    // 光照引擎传播期零调用此接口（grep 确认 lighting 目录内不调），委托 ServerWorld 安全
    return m_world->getBlockStateForLight(pos);
}

IWorld* RuntimeLightingProvider::getWorld()
{
    return m_world->getWorld();
}

const IWorld* RuntimeLightingProvider::getWorld() const
{
    return m_world->getWorld();
}

void RuntimeLightingProvider::markLightChanged(LightType type, const SectionPos& pos)
{
    // 收集 dirty section，不调主线程独占的 ServerWorld::markLightChanged。
    // worker 任务完成后由 RuntimeLightTask 入主线程 flush 队列，主线程统一 flush。
    m_dirtySections.emplace_back(type, pos);
}

bool RuntimeLightingProvider::hasSkyLight() const
{
    return m_world->hasSkyLight();
}

i32 RuntimeLightingProvider::getMinBuildHeight() const
{
    return m_world->getMinBuildHeight();
}

i32 RuntimeLightingProvider::getMaxBuildHeight() const
{
    return m_world->getMaxBuildHeight();
}

i32 RuntimeLightingProvider::getSectionCount() const
{
    return m_world->getSectionCount();
}

std::vector<std::pair<LightType, SectionPos>> RuntimeLightingProvider::takeDirtySections()
{
    return std::move(m_dirtySections);
}

} // namespace mc::server
