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
 */

#include "common/world/gen/surface/SurfaceSystem.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/world/gen/noise/Noises.hpp"
#include "common/world/gen/surface/SurfaceRule.hpp"
#include "common/world/gen/surface/SurfaceRuleContext.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <utility>

using namespace mc::trace;

namespace mc::world::gen::surface {

SurfaceSystem::SurfaceSystem(std::shared_ptr<SurfaceRule> surfaceRule,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    i32 minY,
    i32 height,
    world::gen::RandomState& randomState,
    const math::PositionalRandomFactory& positionalRandom)
    : m_surfaceRule(std::move(surfaceRule))
    , m_defaultBlock(defaultBlock)
    , m_defaultFluid(defaultFluid)
    , m_seaLevel(seaLevel)
    , m_minY(minY)
    , m_height(height)
    , m_randomState(randomState)
    , m_positionalRandom(positionalRandom)
{
    // MC 1.21: SurfaceSystem 使用 RandomState.getOrCreateNoise(Noises.*) 获取噪声实例
    // 所有噪声参数和种子派生统一由 Noises 注册表 + fromHashOf 管理
    m_surfaceDepthNoise = &randomState.getOrCreateNoise(noise::Noises::SURFACE);
    m_surfaceSecondaryNoise = &randomState.getOrCreateNoise(noise::Noises::SURFACE_SECONDARY);
    m_clayBandsOffsetNoise = &randomState.getOrCreateNoise(noise::Noises::CLAY_BANDS_OFFSET);

    // Badlands 噪声
    m_badlandsPillarNoise = &randomState.getOrCreateNoise(noise::Noises::BADLANDS_PILLAR);
    m_badlandsPillarRoofNoise = &randomState.getOrCreateNoise(noise::Noises::BADLANDS_PILLAR_ROOF);
    m_badlandsSurfaceNoise = &randomState.getOrCreateNoise(noise::Noises::BADLANDS_SURFACE);

    // 冰山噪声
    m_icebergPillarNoise = &randomState.getOrCreateNoise(noise::Noises::ICEBERG_PILLAR);
    m_icebergPillarRoofNoise = &randomState.getOrCreateNoise(noise::Noises::ICEBERG_PILLAR_ROOF);
    m_icebergSurfaceNoise = &randomState.getOrCreateNoise(noise::Noises::ICEBERG_SURFACE);
}

bool SurfaceSystem::isStone(const BlockState* state) const
{
    return state != nullptr && !state->isAir() && !state->isLiquid();
}

void SurfaceSystem::erodedBadlandsExtension(
    ChunkPrimer& chunk, i32 worldX, i32 worldZ, i32 surfaceY, i32 localX, i32 localZ) const
{
    // MC 1.21: SurfaceSystem.erodedBadlandsExtension()
    // 在 Eroded Badlands 中生成高耸石柱/方山地貌

    const f64 d1 = std::min(
        std::abs(m_badlandsSurfaceNoise->getValue(static_cast<f64>(worldX), 0.0, static_cast<f64>(worldZ)) * 8.25),
        m_badlandsPillarNoise->getValue(static_cast<f64>(worldX) * 0.2, 0.0, static_cast<f64>(worldZ) * 0.2) * 15.0);

    if (d1 <= 0.0) {
        return;
    }

    const f64 d4 = std::abs(
        m_badlandsPillarRoofNoise->getValue(static_cast<f64>(worldX) * 0.75, 0.0, static_cast<f64>(worldZ) * 0.75) *
        1.5);
    const f64 d5 = 64.0 + std::min(d1 * d1 * 2.5, std::ceil(d4 * 50.0) + 24.0);
    const i32 pillarTop = static_cast<i32>(d5);

    if (surfaceY > pillarTop) {
        return;
    }

    // 从 pillarTop 向下查找第一个默认方块（石头），碰到水则中止
    bool foundStone = false;
    for (i32 y = pillarTop; y >= m_minY; --y) {
        const BlockState* state = chunk.getBlockState(localX, y, localZ);
        if (state != nullptr && state->blockId() == m_defaultBlock->blockId()) {
            foundStone = true;
            break;
        }
        // MC 1.21: 只有水方块阻止石柱生成，其他流体（如熔岩）不阻止
        if (state != nullptr && &state->getBlock() == VanillaBlocks::WATER) {
            return;
        }
    }
    if (!foundStone) {
        return;
    }

    // 从 pillarTop 向下填充空气为默认方块，直到碰到非空气方块
    for (i32 y = pillarTop; y >= m_minY; --y) {
        const BlockState* state = chunk.getBlockState(localX, y, localZ);
        if (state == nullptr || state->isAir()) {
            chunk.setBlockState(localX, y, localZ, m_defaultBlock);
        } else {
            break;
        }
    }
}

void SurfaceSystem::frozenOceanExtension(ChunkPrimer& chunk,
    i32 worldX,
    i32 worldZ,
    i32 surfaceY,
    i32 localX,
    i32 localZ,
    i32 minSurfaceLevel,
    bool isDeepFrozenOcean,
    BiomeId biomeId) const
{
    // MC 1.21: SurfaceSystem.frozenOceanExtension()
    // 在 Frozen Ocean / Deep Frozen Ocean 中生成冰山

    const f64 d1 = std::min(
        std::abs(m_icebergSurfaceNoise->getValue(static_cast<f64>(worldX), 0.0, static_cast<f64>(worldZ)) * 8.25),
        m_icebergPillarNoise->getValue(static_cast<f64>(worldX) * 1.28, 0.0, static_cast<f64>(worldZ) * 1.28) * 15.0);

    if (d1 <= 1.8) {
        return;
    }

    const f64 d5 =
        m_icebergPillarRoofNoise->getValue(static_cast<f64>(worldX) * 1.17, 0.0, static_cast<f64>(worldZ) * 1.17) * 1.5;
    f64 d6 = std::min(d1 * d1 * 1.2, std::ceil(d5 * 40.0) + 14.0);

    // MC 1.21.11: SurfaceSystem.shouldMeltFrozenOceanIcebergSlightly()
    // 检查生物群系温度是否 > 0.1，如果是则减少冰山高度 2.0
    // 注意：使用 seaLevel 作为 Y 坐标（而非 surfaceY），阈值 0.1（而非 doesSnowGenerate 的 0.15）
    const Biome& biome = BiomeRegistry::instance().get(biomeId);
    if (biome.shouldMeltFrozenOceanIcebergSlightly(worldX, m_seaLevel, worldZ, m_seaLevel)) {
        // 温度足够高使冰山轻微融化，减少冰山高度
        d6 -= 2.0;
    }

    f64 icebergTop;
    f64 icebergBottom;

    // MC 1.21.11: d6 > 2.0 时计算冰山范围
    // d2 (icebergBottom) = seaLevel - d6 - 7.0
    // d6 被修改为 d6 + seaLevel (即 icebergTop = seaLevel + old_d6)
    if (d6 > 2.0) {
        icebergBottom = static_cast<f64>(m_seaLevel) - d6 - 7.0;
        icebergTop = static_cast<f64>(m_seaLevel) + d6;
    } else {
        icebergTop = 0.0;
        icebergBottom = 0.0;
    }

    if (icebergTop <= 0.0 && icebergBottom <= 0.0) {
        return;
    }

    // 使用位置随机数确定雪层参数
    auto rng = m_positionalRandom.at(worldX, 0, worldZ);
    const i32 snowLayerCount = 2 + rng->nextInt(4);                     // 2-5
    const i32 snowHeightThreshold = m_seaLevel + 18 + rng->nextInt(10); // seaLevel+18 到 seaLevel+27

    const BlockState* packedIce = VanillaBlocks::getState(VanillaBlocks::PACKED_ICE);
    const BlockState* snowBlock = VanillaBlocks::getState(VanillaBlocks::SNOW_BLOCK);

    const i32 startY = std::max(surfaceY, static_cast<i32>(icebergTop) + 1);
    i32 snowCount = 0;

    for (i32 y = startY; y >= minSurfaceLevel; --y) {
        const BlockState* state = chunk.getBlockState(localX, y, localZ);

        const bool isAir = (state != nullptr && state->isAir());

        // MC 1.21.11: 水方块条件：y > icebergBottom && y < seaLevel && icebergBottom != 0
        const bool isWater = (state != nullptr && state->is(VanillaBlocks::WATER));

        // MC 1.21.11: 放置条件（OR 逻辑）：
        // - 空气且 y < icebergTop：99% 放置冰/雪
        // - 水且 y > icebergBottom 且 y < seaLevel 且 icebergBottom != 0：85% 放置冰/雪
        bool shouldPlace = false;
        if (isAir && y < static_cast<i32>(icebergTop) && rng->nextDouble() > 0.01) {
            shouldPlace = true;
        } else if (isWater && y > static_cast<i32>(icebergBottom) && y < m_seaLevel && icebergBottom != 0.0 &&
            rng->nextDouble() > 0.15) {
            shouldPlace = true;
        }

        if (!shouldPlace) {
            continue;
        }

        // MC 1.21.11: 雪帽逻辑
        // k <= i (未超出雪层配额) && l > j (Y 高于雪层阈值) 时放雪块，否则放冰块
        if (snowCount <= snowLayerCount && y > snowHeightThreshold) {
            if (snowBlock) {
                chunk.setBlockState(localX, y, localZ, snowBlock);
            }
            snowCount++;
        } else {
            // 放冰块
            if (packedIce) {
                chunk.setBlockState(localX, y, localZ, packedIce);
            }
        }
    }
}

void SurfaceSystem::buildSurface(ChunkPrimer& chunk,
    const std::function<BiomeId(i32, i32, i32)>& getBiomeAt,
    const density::NoiseChunk& noiseChunk) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "BuildSurface", "x", chunk.x(), "z", chunk.z());

    if (!m_surfaceRule) {
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    // 创建上下文（含高度查询回调用于 steep 条件）
    SurfaceRuleContext ctx(m_seaLevel,
        m_minY,
        m_height,
        m_surfaceDepthNoise,
        m_surfaceSecondaryNoise,
        m_clayBandsOffsetNoise,
        noiseChunk,
        m_positionalRandom,
        &m_randomState,
        [&chunk, startX, startZ, this](i32 worldX, i32 worldZ) -> i32 {
            // 将世界坐标转换为本地坐标
            const i32 localX = worldX - startX;
            const i32 localZ = worldZ - startZ;
            // 边界检查：超出区块范围时使用当前列高度
            if (localX < 0 || localX >= world::CHUNK_WIDTH || localZ < 0 || localZ >= world::CHUNK_WIDTH) {
                return m_seaLevel + 1; // 默认海平面高度
            }
            return chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;
        });

    for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
        for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "BuildSurfaceColumn", "localX", localX, "localZ", localZ);

            const i32 worldX = startX + localX;
            const i32 worldZ = startZ + localZ;

            // MC 1.21: 先获取初始表面高度（erodedBadlands 可能修改此高度）
            const i32 initialSurfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;

            // 获取当前列的生物群系（使用表面高度处）
            const BiomeId biomeId = getBiomeAt(worldX, initialSurfaceY, worldZ);

            // MC 1.21: erodedBadlandsExtension — 在规则应用之前
            if (biomeId == Biomes::ErodedBadlands) {
                erodedBadlandsExtension(chunk, worldX, worldZ, initialSurfaceY, localX, localZ);
            }

            // 更新 XZ（erodedBadlandsExtension 可能修改了高度图，需重新获取）
            ctx.updateXZ(worldX, worldZ);
            ctx.setBiome(biomeId);

            // 从高度图获取表面高度（erodedBadlandsExtension 后重新获取）
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;

            // MC 1.21.11: 跟踪 stoneDepthAbove, stoneDepthBelow, waterHeight
            // k2 使用 WAY_BELOW_MIN_Y 哨兵值（MC: DimensionType.WAY_BELOW_MIN_Y = MIN_Y << 4）
            // 表示石头柱向下延伸到极深处，使得 stoneDepthBelow 计算为很大的正值
            i32 stoneDepthAbove = 0;
            i32 waterHeight = std::numeric_limits<int>::min();
            i32 stoneDepthBelowStart = world::MIN_BUILD_HEIGHT << 4; // -1024，哨兵值

            // 从上到下遍历列
            for (i32 y = surfaceY; y >= m_minY; --y) {
                const BlockState* currentState = chunk.getBlockState(localX, y, localZ);

                if (currentState == nullptr || currentState->isAir()) {
                    stoneDepthAbove = 0;
                    waterHeight = std::numeric_limits<i32>::min();
                    continue;
                }

                if (currentState->isLiquid()) {
                    if (waterHeight == std::numeric_limits<int>::min()) {
                        waterHeight = y + 1;
                    }
                    continue;
                }

                // 计算 stoneDepthBelow（到下方非石头方块的距离）
                // MC 1.21.11: 当 stoneDepthBelowStart >= y 时需要重新扫描
                // 使用 WAY_BELOW_MIN_Y 哨兵值重置，表示下方石头柱延伸到极深处
                if (stoneDepthBelowStart >= y) {
                    stoneDepthBelowStart = world::MIN_BUILD_HEIGHT << 4;
                    for (i32 dy = y - 1; dy >= m_minY - 1; --dy) {
                        const BlockState* belowState =
                            (dy >= m_minY) ? chunk.getBlockState(localX, dy, localZ) : nullptr;
                        if (!isStone(belowState)) {
                            stoneDepthBelowStart = dy + 1;
                            break;
                        }
                    }
                }

                const i32 stoneDepthBelow = y - stoneDepthBelowStart + 1;

                // 只替换默认方块
                if (currentState->blockId() != m_defaultBlock->blockId()) {
                    stoneDepthAbove++;
                    continue;
                }

                stoneDepthAbove++;

                // 更新 Y 相关状态
                ctx.updateY(stoneDepthAbove, stoneDepthBelow, waterHeight, worldX, y, worldZ);

                // 设置生物群系（使用当前位置的精确生物群系）
                const BiomeId yBiomeId = getBiomeAt(worldX, y, worldZ);
                ctx.setBiome(yBiomeId);

                // 应用表面规则
                const BlockState* replacement = m_surfaceRule->tryApply(worldX, y, worldZ, ctx);

                if (replacement != nullptr && replacement->blockId() != currentState->blockId()) {
                    chunk.setBlockState(localX, y, localZ, replacement);
                    // MC 1.21: SurfaceSystem — 写入流体方块时标记后处理
                    if (replacement->isLiquid()) {
                        chunk.markPosForPostprocessing(localX, y, localZ);
                    }
                }
            }

            // MC 1.21: frozenOceanExtension — 在规则应用之后
            if (biomeId == Biomes::FrozenOcean || biomeId == Biomes::DeepFrozenOcean) {
                frozenOceanExtension(chunk,
                    worldX,
                    worldZ,
                    initialSurfaceY,
                    localX,
                    localZ,
                    ctx.minSurfaceLevel(),
                    biomeId == Biomes::DeepFrozenOcean,
                    biomeId);
            }
        }
    }
}

} // namespace mc::world::gen::surface
