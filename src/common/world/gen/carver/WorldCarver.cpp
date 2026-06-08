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
#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// CarvingMask 实现
// ============================================================================

CarvingMask::CarvingMask(ChunkCoord chunkX, ChunkCoord chunkZ, i32 minY, i32 height)
    : m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_minY(minY)
    , m_height(height)
    , m_mask(static_cast<size_t>(world::CHUNK_WIDTH) * world::CHUNK_WIDTH * static_cast<size_t>(height), false)
{}

bool CarvingMask::isCarved(BlockCoord x, i32 y, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH || y < m_minY || y >= m_minY + m_height) {
        return false;
    }
    const i32 index = (x) | ((z) << world::CHUNK_SHIFT) | ((y - m_minY) << (world::CHUNK_SHIFT + world::SECTION_SHIFT));
    return m_mask[static_cast<size_t>(index)];
}

void CarvingMask::setCarved(BlockCoord x, i32 y, BlockCoord z)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH || y < m_minY || y >= m_minY + m_height) {
        return;
    }
    const i32 index = (x) | ((z) << world::CHUNK_SHIFT) | ((y - m_minY) << (world::CHUNK_SHIFT + world::SECTION_SHIFT));
    m_mask[static_cast<size_t>(index)] = true;
}

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
    // MC 1.21.11: 先检查 lavaLevel
    const i32 lavaLevel = config.lavaLevel.resolveY(context.getMinGenY(), context.getGenDepth());
    if (worldY <= lavaLevel) {
        return VanillaBlocks::getState(VanillaBlocks::LAVA);
    }

    // 含水层决策
    if (context.hasAquifer()) {
        const BlockState* aquiferState = context.aquifer()->computeSubstance(worldX, worldY, worldZ, 0.0);
        if (aquiferState) {
            return aquiferState;
        }
        // 含水层返回 nullptr → 不雕刻
        return nullptr;
    }

    // 无含水层时的回退逻辑
    return getCaveAirState();
}

template <typename Config>
bool WorldCarver<Config>::carveEllipsoid(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::BiomeSource& /*biomeSource*/,
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

    // MC: 使用目标区块坐标计算边界
    const i32 chunkStartX = targetChunkX * world::CHUNK_WIDTH;
    const i32 chunkStartZ = targetChunkZ * world::CHUNK_WIDTH;

    // 检查椭球是否在目标区块范围外
    const f32 distLimit = horizontalRadius + 2.0f + static_cast<f32>(world::CHUNK_WIDTH);
    const f32 dxC = centerX - static_cast<f32>(chunkStartX + world::CHUNK_WIDTH / 2);
    const f32 dzC = centerZ - static_cast<f32>(chunkStartZ + world::CHUNK_WIDTH / 2);
    if (std::abs(dxC) > distLimit || std::abs(dzC) > distLimit) {
        return false;
    }

    // MC: 计算目标区块内有效范围
    const i32 localMinX = std::max(0, startX - chunkStartX);
    const i32 localMaxX = std::min(world::CHUNK_WIDTH - 1, endX - chunkStartX);
    const i32 localMinZ = std::max(0, startZ - chunkStartZ);
    const i32 localMaxZ = std::min(world::CHUNK_WIDTH - 1, endZ - chunkStartZ);

    // MC: Y 范围使用 CarvingContext
    const i32 minGenY = context.getMinGenY();
    const i32 genDepth = context.getGenDepth();

    // 检查是否有流体
    if (shouldCheckForFluid() &&
        checkAreaForFluid(chunk,
            targetChunkX,
            targetChunkZ,
            localMinX,
            localMaxX + 1,
            std::max(minGenY + 1, startY_world),
            std::min(minGenY + genDepth - 8, endY_world),
            localMinZ,
            localMaxZ + 1)) {
        return false;
    }

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

            // MC: Y 范围从 minGenY+1 到 minGenY+genDepth-8，从 topY 向下迭代
            const i32 bottomY = std::max(minGenY + 1, startY_world);
            const i32 topY = std::min(minGenY + genDepth - 8, endY_world);

            for (i32 y = topY; y > bottomY; --y) {
                const f32 dy = (static_cast<f32>(y) - 0.5f - centerY) / verticalRadius;

                // MC: 使用回调检查是否跳过（洞穴/峡谷各有不同逻辑）
                if (skipChecker(dx, dy, dz, y)) {
                    continue;
                }

                // 检查雕刻掩码
                if (carvingMask.isCarved(lx, y, lz)) {
                    continue;
                }

                // 获取当前方块
                const BlockState* state = chunk.getBlockState(lx, y, lz);
                if (!state) {
                    continue;
                }

                // 获取上方方块
                const BlockState* aboveState =
                    (y < minGenY + genDepth - 1) ? chunk.getBlockState(lx, y + 1, lz) : nullptr;

                // MC: 检查方块是否可雕刻（使用配置中的 replaceable tag）
                if (!canReplaceBlock(*state, config)) {
                    continue;
                }

                // MC: 标记当前方块是否为草地或菌丝
                bool hasGrassOrMycelium = state->is(VanillaBlocks::GRASS_BLOCK) || state->is(VanillaBlocks::MYCELIUM);

                // MC 1.21.11: 获取雕刻后方块状态
                const BlockState* carveState = getCarveState(context, worldX, y, worldZ, config);
                if (!carveState) {
                    // 含水层返回 nullptr → 不雕刻
                    continue;
                }

                // 标记为已雕刻
                carvingMask.setCarved(lx, y, lz);
                chunk.setBlockState(lx, y, lz, carveState);

                // MC 1.21: 含水层流体更新调度
                // 当含水层请求流体更新且当前方块有流体状态时，标记后处理
                if (context.hasAquifer() && context.aquifer()->shouldScheduleFluidUpdate()) {
                    if (carveState && carveState->isLiquid()) {
                        chunk.markPosForPostprocessing(lx, y, lz);
                    }
                }

                // MC: 草地/菌丝表面替换
                if (handlesSurfaceReplacement() && hasGrassOrMycelium && y > minGenY) {
                    const BlockState* belowState = chunk.getBlockState(lx, y - 1, lz);
                    if (belowState && belowState->is(VanillaBlocks::DIRT)) {
                        const BlockState* dirt = VanillaBlocks::getState(VanillaBlocks::DIRT);
                        if (dirt) {
                            chunk.setBlockState(lx, y - 1, lz, dirt);
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
    // MC: 使用目标区块坐标计算 canReach
    const f32 chunkCenterX = static_cast<f32>(targetChunkX * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2);
    const f32 chunkCenterZ = static_cast<f32>(targetChunkZ * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2);

    const f32 dx = x - chunkCenterX;
    const f32 dz = z - chunkCenterZ;

    const f32 remainingSteps = static_cast<f32>(maxSteps - step);
    const f32 maxDist = radius + 2.0f + static_cast<f32>(world::CHUNK_WIDTH);

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
