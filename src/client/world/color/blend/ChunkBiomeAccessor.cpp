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

#include "ChunkBiomeAccessor.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>

namespace mc::client {

using namespace mc::world;

ChunkBiomeAccessor::ChunkBiomeAccessor(const ChunkData& chunk,
    const std::array<const ChunkData*, 4>& neighbors,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    i32 minBuildHeight,
    i32 maxBuildHeight)
    : m_chunk(chunk)
    , m_neighbors(neighbors)
    , m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_minBuildHeight(minBuildHeight)
    , m_maxBuildHeight(maxBuildHeight)
{}

ChunkBiomeAccessor::ChunkBiomeAccessor(
    const ChunkData& chunk, ChunkCoord chunkX, ChunkCoord chunkZ, i32 minBuildHeight, i32 maxBuildHeight)
    : m_chunk(chunk)
    , m_neighbors{{nullptr, nullptr, nullptr, nullptr}}
    , m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_minBuildHeight(minBuildHeight)
    , m_maxBuildHeight(maxBuildHeight)
{}

const Biome* ChunkBiomeAccessor::getBiome(i32 x, i32 y, i32 z) const
{
    // 检查Y范围
    if (y < m_minBuildHeight || y >= m_maxBuildHeight) {
        return nullptr;
    }

    i32 localX, localZ;
    const ChunkData* targetChunk = _resolveChunk(x, z, localX, localZ);

    if (!targetChunk) {
        return nullptr;
    }

    const BiomeId biomeId = targetChunk->getBiomeAtBlock(localX, y, localZ);
    return &BiomeRegistry::instance().get(biomeId);
}

bool ChunkBiomeAccessor::isChunkLoaded(ChunkCoord x, ChunkCoord z) const
{
    if (x == m_chunkX && z == m_chunkZ) {
        return true;
    }

    // 检查是否是邻居区块
    if (x == m_chunkX - 1 && z == m_chunkZ) return m_neighbors[0] != nullptr; // 西
    if (x == m_chunkX + 1 && z == m_chunkZ) return m_neighbors[1] != nullptr; // 东
    if (x == m_chunkX && z == m_chunkZ - 1) return m_neighbors[2] != nullptr; // 北
    if (x == m_chunkX && z == m_chunkZ + 1) return m_neighbors[3] != nullptr; // 南

    return false;
}

const Biome* ChunkBiomeAccessor::getBiomeLocal(i32 localX, i32 y, i32 localZ) const
{
    if (localX < 0 || localX >= CHUNK_WIDTH || localZ < 0 || localZ >= CHUNK_WIDTH || y < m_minBuildHeight ||
        y >= m_maxBuildHeight) {
        return nullptr;
    }

    const BiomeId biomeId = m_chunk.getBiomeAtBlock(localX, y, localZ);
    return &BiomeRegistry::instance().get(biomeId);
}

const ChunkData* ChunkBiomeAccessor::_resolveChunk(i32 worldX, i32 worldZ, i32& outLocalX, i32& outLocalZ) const
{
    // 计算区块坐标
    const ChunkCoord targetChunkX = worldX >> 4;
    const ChunkCoord targetChunkZ = worldZ >> 4;

    // 计算区块内坐标
    outLocalX = worldX & 15;
    outLocalZ = worldZ & 15;

    // 检查是否在当前区块内
    if (targetChunkX == m_chunkX && targetChunkZ == m_chunkZ) {
        return &m_chunk;
    }

    // 检查邻居区块
    // 索引: 0=西(-X), 1=东(+X), 2=北(-Z), 3=南(+Z)

    // 西边区块 (-X)
    if (targetChunkX == m_chunkX - 1 && targetChunkZ == m_chunkZ) {
        return m_neighbors[0];
    }

    // 东边区块 (+X)
    if (targetChunkX == m_chunkX + 1 && targetChunkZ == m_chunkZ) {
        return m_neighbors[1];
    }

    // 北边区块 (-Z)
    if (targetChunkX == m_chunkX && targetChunkZ == m_chunkZ - 1) {
        return m_neighbors[2];
    }

    // 南边区块 (+Z)
    if (targetChunkX == m_chunkX && targetChunkZ == m_chunkZ + 1) {
        return m_neighbors[3];
    }

    // 对角线或更远的区块 - 不支持
    return nullptr;
}

} // namespace mc::client
