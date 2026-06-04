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

#include "IChunk.hpp"
#include "ChunkData.hpp"
#include "common/core/Constants.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// BiomeContainer 实现
// ============================================================================

void BiomeContainer::setBiome(i32 sectionIndex, i32 x, i32 y, i32 z, BiomeId biome)
{
    MC_ASSERT_RELEASE(sectionIndex >= 0 && sectionIndex < SECTION_COUNT);
    if (x >= 0 && x < HORIZ_SIZE && y >= 0 && y < VERT_SIZE && z >= 0 && z < HORIZ_SIZE) {
        const i32 index = sectionIndex * SECTION_BIOME_SIZE
            + y * HORIZ_SIZE * HORIZ_SIZE
            + z * HORIZ_SIZE
            + x;
        m_biomes[static_cast<size_t>(index)] = biome;
    }
}

BiomeId BiomeContainer::getBiome(i32 sectionIndex, i32 x, i32 y, i32 z) const
{
    MC_ASSERT_RELEASE(sectionIndex >= 0 && sectionIndex < SECTION_COUNT);
    if (x >= 0 && x < HORIZ_SIZE && y >= 0 && y < VERT_SIZE && z >= 0 && z < HORIZ_SIZE) {
        const i32 index = sectionIndex * SECTION_BIOME_SIZE
            + y * HORIZ_SIZE * HORIZ_SIZE
            + z * HORIZ_SIZE
            + x;
        return m_biomes[static_cast<size_t>(index)];
    }
    return 0;
}

BiomeId BiomeContainer::getBiomeAtBlock(i32 x, i32 y, i32 z) const
{
    // 将方块坐标映射到生物群系采样点
    // X 和 Z：每 4 个方块对应一个采样点
    const i32 biomeX = std::clamp(x >> 2, 0, HORIZ_SIZE - 1);
    const i32 biomeZ = std::clamp(z >> 2, 0, HORIZ_SIZE - 1);

    // Y：需要计算正确的 section 索引和 biome Y 索引
    // section 索引 = (y - MIN_BUILD_HEIGHT) / CHUNK_SECTION_HEIGHT
    // biome Y 索引 = ((y - MIN_BUILD_HEIGHT) % CHUNK_SECTION_HEIGHT) / 4
    const i32 yOffset = y - world::MIN_BUILD_HEIGHT;
    if (yOffset < 0 || yOffset >= world::CHUNK_HEIGHT) {
        return 0;
    }

    const i32 sectionIndex = yOffset / world::CHUNK_SECTION_HEIGHT;
    const i32 biomeY = (yOffset % world::CHUNK_SECTION_HEIGHT) >> 2;

    return getBiome(sectionIndex, biomeX, biomeY, biomeZ);
}

std::vector<u8> BiomeContainer::serialize() const
{
    std::vector<u8> data;
    data.reserve(TOTAL_SIZE * sizeof(BiomeId));
    for (BiomeId biome : m_biomes) {
        data.push_back(static_cast<u8>(biome & 0xFF));
        data.push_back(static_cast<u8>((biome >> 8) & 0xFF));
    }
    return data;
}

Result<BiomeContainer> BiomeContainer::deserialize(const u8* data, size_t size)
{
    const size_t expectedSize = TOTAL_SIZE * sizeof(BiomeId);
    if (size < expectedSize) {
        return Error(ErrorCode::InvalidArgument, "BiomeContainer deserialize: data too small");
    }

    BiomeContainer container;
    for (size_t i = 0; i < TOTAL_SIZE; ++i) {
        const u16 low = data[i * 2];
        const u16 high = data[i * 2 + 1];
        container.m_biomes[i] = static_cast<BiomeId>(low | (high << 8));
    }
    return container;
}

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

} // namespace mc
