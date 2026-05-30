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

#include "ChunkPrimer.hpp"
#include "../WorldConstants.hpp"
#include "../block/BlockRegistry.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc {

namespace {

constexpr std::array<HeightmapType, 7> ALL_HEIGHTMAP_TYPES = {
    HeightmapType::WorldSurface,
    HeightmapType::OceanFloor,
    HeightmapType::MotionBlocking,
    HeightmapType::MotionBlockingNoLeaves,
    HeightmapType::WorldSurfaceWG,
    HeightmapType::OceanFloorWG,
    HeightmapType::LightBlocking,
};

void initializeAllHeightmaps(std::unordered_map<HeightmapType, Heightmap>& heightmaps)
{
    for (HeightmapType type : ALL_HEIGHTMAP_TYPES) {
        heightmaps[type] = Heightmap(type);
    }
}

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

ChunkPrimer::ChunkPrimer(ChunkCoord x, ChunkCoord z)
    : m_x(x)
    , m_z(z)
    , m_data(std::make_unique<ChunkData>(x, z))
    , m_chunkStatus(&ChunkStatuses::EMPTY)
    , m_status(ChunkLoadStatus::Empty)
{
    initializeAllHeightmaps(m_heightmaps);
}

ChunkPrimer::ChunkPrimer(std::unique_ptr<ChunkData> data)
    : m_x(data->x())
    , m_z(data->z())
    , m_data(std::move(data))
    , m_chunkStatus(&ChunkStatuses::FULL)
    , m_status(ChunkLoadStatus::Loaded)
{
    MC_ASSERT_RELEASE(m_data != nullptr);
    initializeAllHeightmaps(m_heightmaps);
    updateAllHeightmaps();
}

// ============================================================================
// 方块访问
// ============================================================================

const BlockState* ChunkPrimer::getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (!_isValidBlockCoord(x, y, z)) {
        return BlockRegistry::instance().airState();
    }
    return m_data->getBlockState(x, y, z);
}

void ChunkPrimer::setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (!_isValidBlockCoord(x, y, z)) {
        return;
    }
    m_data->setBlockState(x, y, z, state);
    m_modified = true;
}

u32 ChunkPrimer::getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (!_isValidBlockCoord(x, y, z)) {
        return 0; // Air
    }
    return m_data->getBlockStateId(x, y, z);
}

void ChunkPrimer::setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId)
{
    if (!_isValidBlockCoord(x, y, z)) {
        return;
    }
    m_data->setBlockStateId(x, y, z, stateId);
    m_modified = true;
}

// ============================================================================
// 区块段访问
// ============================================================================

ChunkSection* ChunkPrimer::getSection(i32 index)
{
    return m_data->getSection(index);
}

const ChunkSection* ChunkPrimer::getSection(i32 index) const
{
    return m_data->getSection(index);
}

bool ChunkPrimer::hasSection(i32 index) const
{
    return m_data->hasSection(index);
}

ChunkSection* ChunkPrimer::createSection(i32 index)
{
    m_modified = true;
    return m_data->createSection(index);
}

const ChunkSection* const* ChunkPrimer::getSections() const
{
    return m_data->getSections();
}

// ============================================================================
// 高度图
// ============================================================================

BlockCoord ChunkPrimer::getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    auto it = m_heightmaps.find(type);
    if (it != m_heightmaps.end()) {
        // Heightmap 内部保存的是“最高方块上方一格”的 Y，所以这里要减 1
        // 才是实际的方块坐标。OceanFloorWG 也遵循同一语义。
        return it->second.getHeight(x, z) - 1;
    }
    MC_ASSERT_RELEASE(false);
}

void ChunkPrimer::updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    auto& heightmap = getHeightmap(type);
    heightmap.update(x, y, z, state);
}

// ============================================================================
// 生成阶段管理
// ============================================================================

void ChunkPrimer::setChunkStatus(const ChunkStatus& status)
{
    m_chunkStatus = &status;
    m_modified = true;
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId ChunkPrimer::getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    return m_biomes.getBiomeAtBlock(x, y, z);
}

// ============================================================================
// 光源位置
// ============================================================================

void ChunkPrimer::addLightPosition(BlockCoord x, BlockCoord y, BlockCoord z)
{
    m_lightPositions.push_back(x);
    m_lightPositions.push_back(y);
    m_lightPositions.push_back(z);
}

// ============================================================================
// 高度图管理
// ============================================================================

Heightmap& ChunkPrimer::getHeightmap(HeightmapType type)
{
    auto it = m_heightmaps.find(type);
    if (it == m_heightmaps.end()) {
        it = m_heightmaps.emplace(type, Heightmap(type)).first;
    }
    return it->second;
}

const Heightmap& ChunkPrimer::getHeightmap(HeightmapType type) const
{
    static Heightmap dummy(HeightmapType::WorldSurface);
    auto it = m_heightmaps.find(type);
    return it != m_heightmaps.end() ? it->second : dummy;
}

void ChunkPrimer::updateAllHeightmaps()
{
    // 每次重建前先重置所有高度图，避免雕刻/替换方块后残留旧高度。
    initializeAllHeightmaps(m_heightmaps);

    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            std::array<bool, ALL_HEIGHTMAP_TYPES.size()> resolved{};
            i32 unresolvedCount = static_cast<i32>(ALL_HEIGHTMAP_TYPES.size());

            for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
                if (unresolvedCount <= 0) {
                    break;
                }

                const BlockState* state = m_data->getBlockState(x, y, z);
                if (!state || state->isAir()) {
                    continue;
                }

                for (size_t i = 0; i < ALL_HEIGHTMAP_TYPES.size(); ++i) {
                    if (resolved[i]) {
                        continue;
                    }

                    auto& heightmap = m_heightmaps[ALL_HEIGHTMAP_TYPES[i]];
                    if (heightmap.update(x, y, z, state)) {
                        resolved[i] = true;
                        --unresolvedCount;
                    }
                }
            }
        }
    }
}

// ============================================================================
// 转换方法
// ============================================================================

std::unique_ptr<ChunkData> ChunkPrimer::toChunkData()
{
    // 确保高度图已更新
    updateAllHeightmaps();

    // 标记为完全生成
    m_data->setBiomes(m_biomes);
    m_data->setFullyGenerated(true);
    m_data->setStatus(ChunkLoadStatus::Generated); // 设置 ChunkData 的状态

    // 设置状态
    m_status = ChunkLoadStatus::Generated;
    m_chunkStatus = &ChunkStatuses::FULL;

    // 清空生成的实体数据（调用者应该在调用此方法之前提取）
    m_spawnedEntities.clear();

    return std::move(m_data);
}

// ============================================================================
// 静态工具方法
// ============================================================================

u16 ChunkPrimer::packToLocal(BlockCoord x, BlockCoord y, BlockCoord z)
{
    return static_cast<u16>((x & world::CHUNK_MASK) | ((y & world::CHUNK_MASK) << world::SECTION_SHIFT) |
        ((z & world::CHUNK_MASK) << (world::SECTION_SHIFT * 2)));
}

void ChunkPrimer::unpackFromLocal(
    u16 packed, i32 yOffset, ChunkCoord chunkX, ChunkCoord chunkZ, BlockCoord& x, BlockCoord& y, BlockCoord& z)
{
    x = (packed & world::CHUNK_MASK) + (chunkX << world::CHUNK_SHIFT);
    y = ((packed >> world::SECTION_SHIFT) & world::CHUNK_MASK) + (yOffset << world::SECTION_SHIFT);
    z = ((packed >> (world::SECTION_SHIFT * 2)) & world::CHUNK_MASK) + (chunkZ << world::CHUNK_SHIFT);
}

// ============================================================================
// 辅助方法
// ============================================================================

bool ChunkPrimer::_isValidBlockCoord(BlockCoord x, BlockCoord y, BlockCoord z)
{
    return x >= 0 && x < world::CHUNK_WIDTH && y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT && z >= 0 &&
        z < world::CHUNK_WIDTH;
}

} // namespace mc
