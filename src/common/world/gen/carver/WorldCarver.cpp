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

#include "WorldCarver.hpp"
#include "CarvingContext.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include "common/world/gen/carver/CarverConfiguration.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

namespace {
constexpr f32 CARVE_DISTANCE_PADDING = 2.0f;
// MC 1.21.11: 非升级区块顶部偏移为 7（升级区块为 0）
constexpr i32 CARVE_TOP_Y_OFFSET = 7;
} // namespace

// ============================================================================
// WorldCarver 实现
// ============================================================================

template <typename Config>
const BlockState* WorldCarver<Config>::getCaveAirState() const
{
    return VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
}

template <typename Config>
bool WorldCarver<Config>::canReplaceBlock(const BlockState& state, const Config& config) const
{
    if (config.replaceable) {
        return config.replaceable->contains(state);
    }
    // 回退：无 tag 时使用基础方块列表
    return state.is(VanillaBlocks::STONE) || state.is(VanillaBlocks::GRANITE) || state.is(VanillaBlocks::DIORITE) ||
        state.is(VanillaBlocks::ANDESITE) || state.is(VanillaBlocks::DIRT) || state.is(VanillaBlocks::GRASS_BLOCK);
}

template <typename Config>
const BlockState* WorldCarver<Config>::getCarveState(
    CarvingContext& context, i32 worldX, i32 worldY, i32 worldZ, const Config& config) const
{
    const i32 lavaLevel = config.lavaLevel.resolveY(context.getMinGenY(), context.getGenDepth());
    if (worldY <= lavaLevel) {
        return VanillaBlocks::getState(VanillaBlocks::LAVA);
    }

    if (context.hasAquifer()) {
        const BlockState* aquiferState = context.aquifer()->computeSubstance(worldX, worldY, worldZ, 0.0);
        if (aquiferState) {
            return aquiferState;
        }
        return nullptr;
    }

    return getCaveAirState();
}

template <typename Config>
bool WorldCarver<Config>::carveEllipsoid(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::IBiomeSource& biomeSource,
    ChunkCoord targetChunkX,
    ChunkCoord targetChunkZ,
    f32 centerX,
    f32 centerY,
    f32 centerZ,
    f32 horizontalRadius,
    f32 verticalRadius,
    CarvingMask& carvingMask,
    const CarveSkipChecker& skipChecker,
    const Config& config)
{
    const i32 startX = static_cast<i32>(std::floor(centerX - horizontalRadius)) - 1;
    const i32 endX = static_cast<i32>(std::floor(centerX + horizontalRadius));
    const i32 startY_world = static_cast<i32>(std::floor(centerY - verticalRadius)) - 1;
    const i32 endY_world = static_cast<i32>(std::floor(centerY + verticalRadius)) + 1;
    const i32 startZ = static_cast<i32>(std::floor(centerZ - horizontalRadius)) - 1;
    const i32 endZ = static_cast<i32>(std::floor(centerZ + horizontalRadius));

    const i32 chunkStartX = targetChunkX * world::CHUNK_WIDTH;
    const i32 chunkStartZ = targetChunkZ * world::CHUNK_WIDTH;

    // 检查椭球是否在目标区块范围外
    const f32 distLimit = horizontalRadius + CARVE_DISTANCE_PADDING + static_cast<f32>(world::CHUNK_WIDTH);
    const f32 dxC = centerX - static_cast<f32>(chunkStartX + world::CHUNK_WIDTH / 2);
    const f32 dzC = centerZ - static_cast<f32>(chunkStartZ + world::CHUNK_WIDTH / 2);
    if (std::abs(dxC) > distLimit || std::abs(dzC) > distLimit) {
        return false;
    }

    const i32 localMinX = std::max(0, startX - chunkStartX);
    const i32 localMaxX = std::min(world::CHUNK_WIDTH - 1, endX - chunkStartX);
    const i32 localMinZ = std::max(0, startZ - chunkStartZ);
    const i32 localMaxZ = std::min(world::CHUNK_WIDTH - 1, endZ - chunkStartZ);

    const i32 minGenY = context.getMinGenY();
    const i32 genDepth = context.getGenDepth();

    // MC 1.21.11: 已移除流体预检查逻辑（旧版本的 checkAreaForFluid 在 1.21 中已删除）
    // 洞穴和峡谷不会因为附近有流体而跳过雕刻

    bool carved = false;

    for (i32 lx = localMinX; lx <= localMaxX; ++lx) {
        const i32 worldX = targetChunkX * world::CHUNK_WIDTH + lx;
        const f32 dx = (static_cast<f32>(worldX) + 0.5f - centerX) / horizontalRadius;
        const f32 dxSq = dx * dx;

        for (i32 lz = localMinZ; lz <= localMaxZ; ++lz) {
            const i32 worldZ = targetChunkZ * world::CHUNK_WIDTH + lz;
            const f32 dz = (static_cast<f32>(worldZ) + 0.5f - centerZ) / horizontalRadius;
            const f32 dzSq = dz * dz;

            if (dxSq + dzSq >= 1.0f) {
                continue;
            }

            const i32 bottomY = std::max(minGenY + 1, startY_world);
            const i32 topY = std::min(minGenY + genDepth - CARVE_TOP_Y_OFFSET, endY_world);

            for (i32 y = topY; y > bottomY; --y) {
                const f32 dy = (static_cast<f32>(y) - 0.5f - centerY) / verticalRadius;

                if (skipChecker(CarverEllipsePos{dx, dy, dz, y})) {
                    continue;
                }

                if (carvingMask.isCarved(lx, y, lz)) {
                    continue;
                }

                const BlockState* state = chunk.getBlockState(lx, y, lz);
                if (!state) {
                    continue;
                }

                if (!canReplaceBlock(*state, config)) {
                    continue;
                }

                bool hasGrassOrMycelium = state->is(VanillaBlocks::GRASS_BLOCK) || state->is(VanillaBlocks::MYCELIUM);

                const BlockState* carveState = getCarveState(context, worldX, y, worldZ, config);
                if (!carveState) {
                    continue;
                }

                carvingMask.setCarved(lx, y, lz);
                chunk.setBlockState(lx, y, lz, carveState);

                // 含水层流体更新调度
                if (context.hasAquifer() && context.aquifer()->shouldScheduleFluidUpdate()) {
                    if (carveState && carveState->isLiquid()) {
                        chunk.markPosForPostprocessing(lx, y, lz);
                    }
                }

                // MC 1.21.11: 草地/菌丝表面替换
                // 当被雕刻的方块是草方块/菌丝时，检查下方方块是否为泥土，
                // 如果是则用 topMaterial() 替换为生物群系对应的地表方块。
                // MC 原版使用 CarvingContext.topMaterial() 评估 SurfaceRules，
                // 当前实现通过 Biome::surfaceBlock()/underWaterBlock() 实现等效逻辑。
                if (handlesSurfaceReplacement() && hasGrassOrMycelium && y > minGenY) {
                    if (y - 1 >= minGenY) {
                        const BlockState* belowState = chunk.getBlockState(lx, y - 1, lz);
                        if (belowState && belowState->is(VanillaBlocks::DIRT)) {
                            const bool hasFluid = carveState->isLiquid();
                            const BlockState* topBlock =
                                context.topMaterial(biomeSource, worldX, y - 1, worldZ, hasFluid);
                            if (topBlock) {
                                chunk.setBlockState(lx, y - 1, lz, topBlock);
                                // 含水地表方块需要调度流体更新
                                if (topBlock->isLiquid()) {
                                    chunk.markPosForPostprocessing(lx, y - 1, lz);
                                }
                            }
                        }
                    }
                }

                carved = true;
            }
        }
    }

    return carved;
}

template <typename Config>
bool WorldCarver<Config>::isInCarvingRange(
    ChunkCoord targetChunkX, ChunkCoord targetChunkZ, f32 x, f32 z, i32 step, i32 maxSteps, f32 radius)
{
    const f32 chunkCenterX = static_cast<f32>(targetChunkX * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2);
    const f32 chunkCenterZ = static_cast<f32>(targetChunkZ * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2);

    const f32 dx = x - chunkCenterX;
    const f32 dz = z - chunkCenterZ;

    const f32 remainingSteps = static_cast<f32>(maxSteps - step);
    const f32 maxDist = radius + CARVE_DISTANCE_PADDING + static_cast<f32>(world::CHUNK_WIDTH);

    return dx * dx + dz * dz - remainingSteps * remainingSteps <= maxDist * maxDist;
}

template <typename Config>
bool WorldCarver<Config>::checkAreaForFluid(ChunkPrimer& chunk,
    ChunkCoord /*targetChunkX*/,
    ChunkCoord /*targetChunkZ*/,
    i32 minX,
    i32 maxX,
    i32 minY,
    i32 maxY,
    i32 minZ,
    i32 maxZ) const
{
    for (i32 lx = minX; lx < maxX; ++lx) {
        for (i32 lz = minZ; lz < maxZ; ++lz) {
            for (i32 y = minY - 1; y <= maxY + 1; ++y) {
                if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
                    continue;
                }

                const BlockState* state = chunk.getBlockState(lx, y, lz);
                if (state) {
                    if (state->is(VanillaBlocks::WATER) || state->is(VanillaBlocks::LAVA)) {
                        return true;
                    }
                }

                // 跳过中间的方块（优化）
                if (y != maxY + 1 && lx != minX && lx != maxX - 1 && lz != minZ && lz != maxZ - 1) {
                    y = maxY;
                }
            }
        }
    }

    return false;
}

// 显式实例化常用模板
template class WorldCarver<CaveCarverConfiguration>;
template class WorldCarver<CanyonCarverConfiguration>;

} // namespace mc
