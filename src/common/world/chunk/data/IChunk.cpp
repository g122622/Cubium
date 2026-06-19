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

#include "common/world/chunk/data/IChunk.hpp"
#include "common/core/Constants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <algorithm>

namespace mc::world::chunk {

// ============================================================================
// Heightmap 实现
// ============================================================================

Heightmap::Heightmap(HeightmapType type)
    : m_type(type)
{
    m_heights.fill(0);
}

bool Heightmap::update(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return false;
    }

    const i32 index = z * mc::world::CHUNK_WIDTH + x;
    const BlockCoord currentHeight = m_heights[static_cast<size_t>(index)];

    // 只有当新方块高于当前高度且是阻挡方块时才更新
    if (y >= currentHeight && _isOpaque(state)) {
        m_heights[static_cast<size_t>(index)] = y + 1; // 高度图存储的是 Y+1（即上方空气方块的位置）
        return true;
    }

    return false;
}

BlockCoord Heightmap::getHeight(BlockCoord x, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return 0;
    }
    const i32 index = z * mc::world::CHUNK_WIDTH + x;
    return m_heights[static_cast<size_t>(index)];
}

void Heightmap::setData(const std::array<BlockCoord, SIZE>& data)
{
    m_heights = data;
}

bool Heightmap::_isOpaque(const BlockState* state) const
{
    if (!state) {
        return false;
    }

    // 获取方块
    const Block& block = state->owner();

    switch (m_type) {
        case HeightmapType::WorldSurface:
        case HeightmapType::WorldSurfaceWG:
            // 最高非空气方块
            return !block.isAir(*state);

        case HeightmapType::OceanFloor:
        case HeightmapType::OceanFloorWG:
            // 最高固体方块（排除水、岩浆等流体）
            return block.isSolid(*state);

        case HeightmapType::MotionBlocking:
            // 阻挡运动的方块（固体、水、岩浆等）
            return block.isSolid(*state) || state->isLiquid();

        case HeightmapType::MotionBlockingNoLeaves:
            // 阻挡运动但不包括树叶
            return (block.isSolid(*state) || state->isLiquid()) && (&block.material() != &Material::LEAVES) &&
                (&block.material() != &Material::PLANT);

        case HeightmapType::LightBlocking:
            // 阻挡光照的方块（不透明方块，透明度 > 0）
            return block.isSolid(*state) && state->getOpacity() > 0;

        default:
            return !block.isAir(*state);
    }
}

} // namespace mc::world::chunk
