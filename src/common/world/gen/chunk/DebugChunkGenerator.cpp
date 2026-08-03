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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace mc {

// 静态成员初始化
const BlockState* DebugChunkGenerator::s_barrierState = nullptr;
const BlockState* DebugChunkGenerator::s_airState = nullptr;
std::vector<const BlockState*> DebugChunkGenerator::s_allValidStates;
i32 DebugChunkGenerator::s_gridWidth = 0;
i32 DebugChunkGenerator::s_gridHeight = 0;
std::once_flag DebugChunkGenerator::s_initFlag;

// ============================================================================
// 构造函数
// ============================================================================

DebugChunkGenerator::DebugChunkGenerator() noexcept
    : BaseChunkGenerator(0, DimensionSettings{}) // 种子和设置对调试模式无意义
    , m_biomeSource(std::make_unique<world::biome::source::FixedBiomeSource>(0, Biomes::Plains))
{
    std::call_once(s_initFlag, initializeValidStates);
}

// ============================================================================
// 初始化
// ============================================================================

void DebugChunkGenerator::initializeValidStates()
{
    auto& registry = BlockRegistry::instance();

    // 获取屏障和空气方块
    s_barrierState = registry.get(ResourceLocation("minecraft:barrier"));
    s_airState = registry.airState();

    if (!s_barrierState) {
        MC_ASSERT_FAIL("Barrier block not found in registry");
    }

    // 收集所有方块的所有状态
    // MC 1.21.11: 按 Block 注册顺序迭代（与 MC 的 BuiltInRegistries.BLOCK 迭代顺序一致）
    // BlockRegistry::forEachBlockState 使用 unordered_map，迭代顺序不确定，
    // 因此必须按方块 ID 排序以确保确定性
    s_allValidStates.clear();

    // 先收集到临时 vector 中，按方块 ID 排序
    std::vector<std::pair<i32, const BlockState*>> tempStates;
    registry.forEachBlockState(
        [&tempStates](const BlockState& state) { tempStates.emplace_back(state.blockId(), &state); });

    // 按方块 ID 排序，确保迭代顺序与注册顺序一致
    // MC 1.21.11: 同一方块的多个状态按 getPossibleStates() 的顺序排列，
    // 必须使用稳定排序以保持同一方块内状态的相对顺序
    std::stable_sort(
        tempStates.begin(), tempStates.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    s_allValidStates.reserve(tempStates.size());
    for (const auto& [id, state] : tempStates) {
        (void)id;
        s_allValidStates.push_back(state);
    }

    // 计算网格尺寸（近似正方形）
    if (!s_allValidStates.empty()) {
        auto count = static_cast<i32>(s_allValidStates.size());
        s_gridWidth = static_cast<i32>(std::ceil(std::sqrt(static_cast<f32>(count))));
        s_gridHeight = static_cast<i32>(std::ceil(static_cast<f32>(count) / s_gridWidth));
    } else {
        s_gridWidth = 0;
        s_gridHeight = 0;
    }
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

        if (gridX <= s_gridWidth && gridZ <= s_gridHeight) {
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
    return !s_allValidStates.empty();
}

// ============================================================================
// 区块生成接口
// ============================================================================

void DebugChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 调试世界在 generateNoise 阶段不做任何操作
    // 方块放置在 placeFeatures (applyBiomeDecoration) 阶段进行
    MC_UNUSED(region);
    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

void DebugChunkGenerator::buildSurface(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 调试模式不需要地表生成
    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

void DebugChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 调试世界在 applyBiomeDecoration 阶段放置方块
    // Y=屏障层（SEA_LEVEL-3）基座 + Y=SEA_LEVEL+7 方块状态网格
    std::call_once(s_initFlag, initializeValidStates);

    ChunkCoord chunkX = chunk.x();
    ChunkCoord chunkZ = chunk.z();

    for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
        for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
            i32 worldX = (chunkX << world::CHUNK_SHIFT) + localX;
            i32 worldZ = (chunkZ << world::CHUNK_SHIFT) + localZ;

            // Y=屏障层（SEA_LEVEL - 3）
            region.setBlockState(worldX, world::SEA_LEVEL - 3, worldZ, s_barrierState);

            // Y=方块状态网格（无条件放置，包括空气状态）
            // getBlockStateFor 在条件不满足时返回空气状态，不会返回 nullptr
            const BlockState* state = getBlockStateFor(worldX, worldZ);
            region.setBlockState(worldX, world::SEA_LEVEL + 7, worldZ, state);
        }
    }
    chunk.setChunkStatus(ChunkStatuses::FEATURES);
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
    // MC 1.21.11: DebugLevelSource.getBaseHeight() 始终返回 0
    MC_UNUSED(x);
    MC_UNUSED(z);
    MC_UNUSED(type);
    return 0;
}

} // namespace mc
