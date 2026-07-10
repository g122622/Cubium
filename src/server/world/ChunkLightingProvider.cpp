/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ChunkLightingProvider.hpp"

#include "ServerWorld.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc::server {

ChunkLightingProvider::ChunkLightingProvider(
    ServerWorld& world, WorldGenRegion& region, ChunkCoord centerX, ChunkCoord centerZ)
    : m_world(&world)
    , m_region(&region)
    , m_centerX(centerX)
    , m_centerZ(centerZ)
{}

// 获取 ChunkData 指针（若 IChunk 是 ChunkPrimer 则取底层 ChunkData，否则原样返回）
namespace {
ChunkData* _chunkDataFromIChunk(IChunk* chunk)
{
    if (chunk == nullptr) {
        return nullptr;
    }
    // WorldGenRegion 内的区块都是 ChunkPrimer（ChunkProgressionTask 从 StaticChunkCache2D<shared_ptr<SCLM>> 提取各
    // holder 的 ChunkPrimer 填充）
    auto* primer = dynamic_cast<world::chunk::ChunkPrimer*>(chunk);
    return primer != nullptr ? primer->getChunkData() : nullptr;
}

const ChunkData* _chunkDataFromIChunkConst(const IChunk* chunk)
{
    if (chunk == nullptr) {
        return nullptr;
    }
    const auto* primer = dynamic_cast<const world::chunk::ChunkPrimer*>(chunk);
    return primer != nullptr ? primer->getChunkData() : nullptr;
}
} // namespace

IChunk* ChunkLightingProvider::getChunkForLight(ChunkCoord x, ChunkCoord z)
{
    // 中心+半径1（region 覆盖范围）：经 WorldGenRegion 取 ChunkPrimer 底层 ChunkData
    const i32 relX = x - m_region->mainX();
    const i32 relZ = z - m_region->mainZ();
    const i32 radius = m_region->chunkRadius();
    if (relX >= -radius && relX <= radius && relZ >= -radius && relZ <= radius) {
        IChunk* chunk = m_region->getChunkAt(relX, relZ);
        ChunkData* data = _chunkDataFromIChunk(chunk);
        return data;
    }

    // 半径2（region 外）：从已发布区块缓存取 ChunkData*
    return m_world->getChunkForLight(x, z);
}

const IChunk* ChunkLightingProvider::getChunkForLight(ChunkCoord x, ChunkCoord z) const
{
    const i32 relX = x - m_region->mainX();
    const i32 relZ = z - m_region->mainZ();
    const i32 radius = m_region->chunkRadius();
    if (relX >= -radius && relX <= radius && relZ >= -radius && relZ <= radius) {
        const IChunk* chunk = m_region->getChunkAt(relX, relZ);
        const ChunkData* data = _chunkDataFromIChunkConst(chunk);
        return data;
    }

    return m_world->getChunkForLight(x, z);
}

const BlockState* ChunkLightingProvider::getBlockStateForLight(const BlockPos& pos) const
{
    return m_world->getBlockStateForLight(pos);
}

IWorld* ChunkLightingProvider::getWorld()
{
    return m_world->getWorld();
}

const IWorld* ChunkLightingProvider::getWorld() const
{
    return m_world->getWorld();
}

void ChunkLightingProvider::markLightChanged(LightType /*type*/, const SectionPos& /*pos*/)
{
    // no-op：见类注释。light() 已通过 setNibbles 将结果写入 ChunkData，无需回写。
}

bool ChunkLightingProvider::hasSkyLight() const
{
    return m_world->hasSkyLight();
}

i32 ChunkLightingProvider::getMinBuildHeight() const
{
    return m_world->getMinBuildHeight();
}

i32 ChunkLightingProvider::getMaxBuildHeight() const
{
    return m_world->getMaxBuildHeight();
}

i32 ChunkLightingProvider::getSectionCount() const
{
    return m_world->getSectionCount();
}

} // namespace mc::server
