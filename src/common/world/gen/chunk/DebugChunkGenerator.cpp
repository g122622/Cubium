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

#include "DebugChunkGenerator.hpp"

#include "common/core/Constants.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/chunk/IChunk.hpp"

#include <cmath>

namespace mc {

// 静态成员初始化
const BlockState* DebugChunkGenerator::s_barrierState = nullptr;
const BlockState* DebugChunkGenerator::s_airState = nullptr;
std::vector<const BlockState*> DebugChunkGenerator::s_allValidStates;
i32 DebugChunkGenerator::s_gridWidth = 0;
i32 DebugChunkGenerator::s_gridHeight = 0;
bool DebugChunkGenerator::s_initialized = false;

// ============================================================================
// 构造函数
// ============================================================================

DebugChunkGenerator::DebugChunkGenerator() noexcept
    : BaseChunkGenerator(0, DimensionSettings{}) // 种子和设置对调试模式无意义
{
    // 确保已初始化
    if (!s_initialized) {
        initializeValidStates();
    }
}

// ============================================================================
// 初始化
// ============================================================================

void DebugChunkGenerator::initializeValidStates()
{
    if (s_initialized) {
        return;
    }

    auto& registry = BlockRegistry::instance();

    // 获取屏障和空气方块
    s_barrierState = registry.get(ResourceLocation("minecraft:barrier"));
    s_airState = registry.airState();

    if (!s_barrierState) {
        MC_ASSERT_FAIL("Barrier block not found in registry");
    }

    // 收集所有方块的所有状态
    s_allValidStates.clear();

    registry.forEachBlockState([](const BlockState& state) {
        // 跳过空气方块的状态
        if (state.isAir()) {
            return;
        }
        s_allValidStates.push_back(&state);
    });

    // 计算网格尺寸（近似正方形）
    if (!s_allValidStates.empty()) {
        auto count = static_cast<i32>(s_allValidStates.size());
        s_gridWidth = static_cast<i32>(std::ceil(std::sqrt(static_cast<f32>(count))));
        s_gridHeight = static_cast<i32>(std::ceil(static_cast<f32>(count) / s_gridWidth));
    } else {
        s_gridWidth = 0;
        s_gridHeight = 0;
    }

    s_initialized = true;
}

// ============================================================================
// 静态查询方法
// ============================================================================

const std::vector<const BlockState*>& DebugChunkGenerator::getAllValidStates()
{
    return s_allValidStates;
}

i32 DebugChunkGenerator::getGridWidth()
{
    return s_gridWidth;
}

i32 DebugChunkGenerator::getGridHeight()
{
    return s_gridHeight;
}

const BlockState* DebugChunkGenerator::getBlockStateFor(i32 x, i32 z)
{
    // 方块只在奇数坐标放置
    if (x > 0 && z > 0 && (x % 2) != 0 && (z % 2) != 0) {
        i32 gridX = x / 2;
        i32 gridZ = z / 2;

        if (gridX < s_gridWidth && gridZ < s_gridHeight) {
            i32 index = std::abs(gridX * s_gridWidth + gridZ);
            if (index >= 0 && static_cast<size_t>(index) < s_allValidStates.size()) {
                return s_allValidStates[static_cast<size_t>(index)];
            }
        }
    }

    // 其他位置返回空气
    return s_airState;
}

bool DebugChunkGenerator::isInitialized()
{
    return s_initialized;
}

// ============================================================================
// 区块生成接口
// ============================================================================

void DebugChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 调试模式不生成结构
    MC_UNUSED(region);
    MC_UNUSED(chunk);
}

void DebugChunkGenerator::generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 调试模式不生成结构引用
    MC_UNUSED(region);
    MC_UNUSED(chunk);
}

void DebugChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 调试模式使用平原生物群系
    MC_UNUSED(region);
    auto& biomes = chunk.getBiomes();
    // 填充平原生物群系到所有位置
    for (i32 x = 0; x < BiomeContainer::BIOME_WIDTH; ++x) {
        for (i32 y = 0; y < BiomeContainer::BIOME_HEIGHT; ++y) {
            for (i32 z = 0; z < BiomeContainer::BIOME_DEPTH; ++z) {
                biomes.setBiome(x, y, z, Biomes::Plains);
            }
        }
    }
}

void DebugChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 确保已初始化
    if (!s_initialized) {
        initializeValidStates();
    }

    // 生成 Y=60 屏障层和 Y=70 方块网格
    ChunkCoord chunkX = chunk.x();
    ChunkCoord chunkZ = chunk.z();

    for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
        for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
            i32 worldX = (chunkX << world::CHUNK_SHIFT) + localX;
            i32 worldZ = (chunkZ << world::CHUNK_SHIFT) + localZ;

            // Y=60: 屏障基座
            region.setBlockState(worldX, 60, worldZ, s_barrierState);

            // Y=70: 方块状态网格
            const BlockState* state = getBlockStateFor(worldX, worldZ);
            if (state && !state->isAir()) {
                region.setBlockState(worldX, 70, worldZ, state);
            }
        }
    }
}

void DebugChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 调试模式不需要地表生成
    MC_UNUSED(region);
    MC_UNUSED(chunk);
}

void DebugChunkGenerator::applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid)
{
    // 调试模式不应用雕刻器
    MC_UNUSED(region);
    MC_UNUSED(chunk);
    MC_UNUSED(isLiquid);
}

void DebugChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 调试模式不放置特性
    MC_UNUSED(region);
    MC_UNUSED(chunk);
}

i32 DebugChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    // 调试模式不生成生物
    MC_UNUSED(region);
    MC_UNUSED(chunk);
    MC_UNUSED(outEntities);
    return 0;
}

// ============================================================================
// 生物群系和高度查询
// ============================================================================

BiomeId DebugChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);
    // 调试模式始终使用平原生物群系
    return Biomes::Plains;
}

BiomeId DebugChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    MC_UNUSED(noiseX);
    MC_UNUSED(noiseY);
    MC_UNUSED(noiseZ);
    return Biomes::Plains;
}

i32 DebugChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    MC_UNUSED(type);
    // 调试模式高度固定为 70（方块网格层）
    // 但如果该位置没有方块，返回 60（屏障层）
    const BlockState* state = getBlockStateFor(x, z);
    if (state && !state->isAir()) {
        return 70;
    }
    return 60;
}

} // namespace mc
