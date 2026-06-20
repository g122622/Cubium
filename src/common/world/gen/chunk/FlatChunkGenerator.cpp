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
 * THE above copyright notice and this permission notice shall be included in all
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

#include "FlatChunkGenerator.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"

namespace mc {

FlatChunkGenerator::FlatChunkGenerator(u64 seed, FlatLevelGeneratorSettings settings)
    : BaseChunkGenerator(seed, DimensionSettings::flat())
    , m_flatSettings(std::move(settings))
    , m_biomeSource(std::make_unique<world::biome::source::FixedBiomeSource>(seed, m_flatSettings.biomeId()))
{
    // 设置默认生物群系，让 BaseChunkGenerator::generateBiomes 使用
    m_defaultBiome = m_flatSettings.biomeId();
}

// ============================================================================
// 区块生成接口
// ============================================================================

void FlatChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_UNUSED(region);
    // TODO: 实现平坦世界结构生成（根据 structureOverrides）
    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
}

void FlatChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 逐层填充方块，null 条目跳过（由特性系统放置）
    const auto& layers = m_flatSettings.layers();
    const i32 minY = getMinY();
    const i32 genDepth = getGenDepth();
    const i32 fillHeight = std::min(genDepth, static_cast<i32>(layers.size()));

    for (i32 i = 0; i < fillHeight; ++i) {
        const BlockState* blockState = layers[static_cast<size_t>(i)];
        if (blockState == nullptr) {
            // null 条目跳过，由 FILL_LAYER 特性放置（如水层）
            continue;
        }

        const i32 worldY = minY + i;
        for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
            for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
                chunk.setBlockState(localX, worldY, localZ, blockState);
                chunk.updateHeightmap(HeightmapType::WorldSurfaceWG, localX, worldY, localZ, blockState);
                chunk.updateHeightmap(HeightmapType::OceanFloorWG, localX, worldY, localZ, blockState);
            }
        }
    }

    MC_UNUSED(region);
    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

void FlatChunkGenerator::buildSurface(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 平坦世界不需要地表生成，由 SurfaceRules 处理但平坦世界不需要
    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

void FlatChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // TODO: 实现 decoration 和 addLakes 标志控制
    MC_UNUSED(region);
    MC_UNUSED(chunk);
    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

i32 FlatChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    MC_UNUSED(region);
    MC_UNUSED(chunk);
    MC_UNUSED(outEntities);
    return 0;
}

// ============================================================================
// 生物群系和高度查询
// ============================================================================

BiomeId FlatChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);
    return m_flatSettings.biomeId();
}

BiomeId FlatChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    MC_UNUSED(noiseX);
    MC_UNUSED(noiseY);
    MC_UNUSED(noiseZ);
    return m_flatSettings.biomeId();
}

i32 FlatChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    MC_UNUSED(x);
    MC_UNUSED(z);

    // 从上到下扫描层列表，找到第一个不透明方块
    const auto& layers = m_flatSettings.layers();
    const i32 minY = getMinY();

    for (i32 i = static_cast<i32>(layers.size()) - 1; i >= 0; --i) {
        const BlockState* state = layers[static_cast<size_t>(i)];
        if (state != nullptr) {
            bool isOpaque = false;
            switch (type) {
                case HeightmapType::WorldSurface:
                case HeightmapType::WorldSurfaceWG:
                    isOpaque = true;
                    break;
                case HeightmapType::OceanFloor:
                case HeightmapType::OceanFloorWG:
                    isOpaque = state->owner().isSolid(*state);
                    break;
                case HeightmapType::MotionBlocking:
                case HeightmapType::MotionBlockingNoLeaves:
                    isOpaque = state->owner().isSolid(*state) || state->isLiquid();
                    break;
                case HeightmapType::LightBlocking:
                    isOpaque = state->owner().isSolid(*state) && state->getOpacity() > 0;
                    break;
            }
            if (isOpaque) {
                return minY + i + 1;
            }
        }
    }

    return minY;
}

i32 FlatChunkGenerator::getGroundHeight() const
{
    return getHeight(0, 0, HeightmapType::WorldSurfaceWG);
}

NoiseColumn FlatChunkGenerator::getBaseColumn(i32 x, i32 z) const
{
    MC_UNUSED(x);
    MC_UNUSED(z);

    // 返回展开层列表，null 替换为空气
    const auto& layers = m_flatSettings.layers();
    const i32 minY = getMinY();
    const i32 genDepth = getGenDepth();

    NoiseColumn column(minY, genDepth);
    const i32 fillCount = std::min(static_cast<i32>(layers.size()), genDepth);

    for (i32 i = 0; i < fillCount; ++i) {
        const BlockState* state = layers[static_cast<size_t>(i)];
        // null → 空气
        column.setBlock(minY + i, state != nullptr ? state : VanillaBlocks::getState(VanillaBlocks::AIR));
    }
    // 剩余位置保持 nullptr（空气）

    return column;
}

} // namespace mc
