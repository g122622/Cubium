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
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// CarvingMask 实现
// ============================================================================

CarvingMask::CarvingMask(ChunkCoord chunkX, ChunkCoord chunkZ)
    : m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_mask(static_cast<size_t>(world::CHUNK_WIDTH) * world::CHUNK_WIDTH * world::CHUNK_HEIGHT, false)
{}

bool CarvingMask::isCarved(BlockCoord x, i32 y, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT ||
        y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }
    i32 index = getIndex(x, y, z);
    return m_mask[static_cast<size_t>(index)];
}

void CarvingMask::setCarved(BlockCoord x, i32 y, BlockCoord z)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT ||
        y >= world::MAX_BUILD_HEIGHT) {
        return;
    }
    i32 index = getIndex(x, y, z);
    m_mask[static_cast<size_t>(index)] = true;
}

// ============================================================================
// WorldCarver 实现
// ============================================================================

template <typename Config>
const BlockState* WorldCarver<Config>::getCaveAirState() const
{
    // 洞穴空气 - 用于洞穴、峡谷等地下结构生成
    return VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
}

template <typename Config>
bool WorldCarver<Config>::isCarvable(const BlockState& state)
{
    // 可雕刻方块列表：石头变种、泥土类、陶瓦、砂岩等

    // 石头变种
    if (state.is(VanillaBlocks::STONE) || state.is(VanillaBlocks::GRANITE) || state.is(VanillaBlocks::DIORITE) ||
        state.is(VanillaBlocks::ANDESITE)) {
        return true;
    }

    // 泥土类
    if (state.is(VanillaBlocks::DIRT) || state.is(VanillaBlocks::COARSE_DIRT) || state.is(VanillaBlocks::PODZOL) ||
        state.is(VanillaBlocks::GRASS_BLOCK)) {
        return true;
    }

    // 陶瓦（包括染色陶瓦）
    if (state.is(VanillaBlocks::TERRACOTTA) || state.is(VanillaBlocks::WHITE_TERRACOTTA) ||
        state.is(VanillaBlocks::ORANGE_TERRACOTTA) || state.is(VanillaBlocks::MAGENTA_TERRACOTTA) ||
        state.is(VanillaBlocks::LIGHT_BLUE_TERRACOTTA) || state.is(VanillaBlocks::YELLOW_TERRACOTTA) ||
        state.is(VanillaBlocks::LIME_TERRACOTTA) || state.is(VanillaBlocks::PINK_TERRACOTTA) ||
        state.is(VanillaBlocks::GRAY_TERRACOTTA) || state.is(VanillaBlocks::LIGHT_GRAY_TERRACOTTA) ||
        state.is(VanillaBlocks::CYAN_TERRACOTTA) || state.is(VanillaBlocks::PURPLE_TERRACOTTA) ||
        state.is(VanillaBlocks::BLUE_TERRACOTTA) || state.is(VanillaBlocks::BROWN_TERRACOTTA) ||
        state.is(VanillaBlocks::GREEN_TERRACOTTA) || state.is(VanillaBlocks::RED_TERRACOTTA) ||
        state.is(VanillaBlocks::BLACK_TERRACOTTA)) {
        return true;
    }

    // 沙子和砂岩
    if (state.is(VanillaBlocks::SAND) || state.is(VanillaBlocks::RED_SAND) || state.is(VanillaBlocks::SANDSTONE) ||
        state.is(VanillaBlocks::RED_SANDSTONE)) {
        return true;
    }

    // 其他可雕刻方块
    if (state.is(VanillaBlocks::MYCELIUM) || state.is(VanillaBlocks::SNOW) || state.is(VanillaBlocks::PACKED_ICE)) {
        return true;
    }

    return false;
}

template <typename Config>
bool WorldCarver<Config>::canCarveBlock(const BlockState* state, const BlockState* aboveState) const
{
    if (!state) {
        return false;
    }

    // 检查是否可雕刻
    if (isCarvable(*state)) {
        return true;
    }

    // 沙子和沙砾可以在特定条件下雕刻
    bool isSandOrGravel = state->is(VanillaBlocks::SAND) || state->is(VanillaBlocks::GRAVEL);
    if (isSandOrGravel && aboveState) {
        return !aboveState->isLiquid();
    }

    return false;
}

template <typename Config>
bool WorldCarver<Config>::carveEllipsoid(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::BiomeSource& /*biomeSource*/,
    i32 /*seaLevel*/,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    f32 centerX,
    f32 centerY,
    f32 centerZ,
    f32 horizontalRadius,
    f32 verticalRadius,
    CarvingMask& carvingMask,
    i64 seed)
{
    const i32 startX = static_cast<i32>(std::floor(centerX - horizontalRadius)) - 1;
    const i32 endX = static_cast<i32>(std::floor(centerX + horizontalRadius));
    const i32 startY_world = static_cast<i32>(std::floor(centerY - verticalRadius)) - 1;
    const i32 endY_world = static_cast<i32>(std::floor(centerY + verticalRadius)) + 1;
    const i32 startZ = static_cast<i32>(std::floor(centerZ - horizontalRadius)) - 1;
    const i32 endZ = static_cast<i32>(std::floor(centerZ + horizontalRadius));

    // 区块边界
    const i32 chunkStartX = chunkX * world::CHUNK_WIDTH;
    const i32 chunkStartZ = chunkZ * world::CHUNK_WIDTH;

    // 检查椭球是否在区块范围外
    const f32 distLimit = horizontalRadius + 2.0f + static_cast<f32>(world::CHUNK_WIDTH);
    const f32 dxC = centerX - static_cast<f32>(chunkStartX + world::CHUNK_WIDTH / 2);
    const f32 dzC = centerZ - static_cast<f32>(chunkStartZ + world::CHUNK_WIDTH / 2);
    if (std::abs(dxC) > distLimit || std::abs(dzC) > distLimit) {
        return false;
    }

    // MC: 计算区块内有效范围
    const i32 localMinX = std::max(0, startX - chunkStartX);
    const i32 localMaxX = std::min(world::CHUNK_WIDTH - 1, endX - chunkStartX);
    const i32 localMinZ = std::max(0, startZ - chunkStartZ);
    const i32 localMaxZ = std::min(world::CHUNK_WIDTH - 1, endZ - chunkStartZ);

    // 检查是否有流体（水下/下界雕刻器跳过此检查）
    if (shouldCheckForFluid() &&
        checkAreaForFluid(chunk,
            chunkX,
            chunkZ,
            localMinX,
            localMaxX + 1,
            std::max(world::MIN_BUILD_HEIGHT + 1, startY_world),
            std::min(world::MIN_BUILD_HEIGHT + world::CHUNK_HEIGHT - 8, endY_world),
            localMinZ,
            localMaxZ + 1)) {
        return false;
    }

    math::Random rng(static_cast<u64>(seed) + static_cast<u64>(chunkX) + static_cast<u64>(chunkZ));
    bool carved = false;

    for (i32 lx = localMinX; lx <= localMaxX; ++lx) {
        const i32 worldX = chunkX * world::CHUNK_WIDTH + lx;
        const f32 dx = (static_cast<f32>(worldX) + 0.5f - centerX) / horizontalRadius;
        const f32 dxSq = dx * dx;

        for (i32 lz = localMinZ; lz <= localMaxZ; ++lz) {
            const i32 worldZ = chunkZ * world::CHUNK_WIDTH + lz;
            const f32 dz = (static_cast<f32>(worldZ) + 0.5f - centerZ) / horizontalRadius;
            const f32 dzSq = dz * dz;

            // 检查是否在椭球投影范围内
            if (dxSq + dzSq >= 1.0f) {
                continue;
            }

            // MC: Y 范围 [max(floor(centerY-vertRadius)-1, minGenY+1), min(floor(centerY+vertRadius)+1,
            // minGenY+genDepth-8)] 从 topY 向下迭代到 bottomY（不含 bottomY，即 y > bottomY）
            const i32 bottomY = std::max(world::MIN_BUILD_HEIGHT + 1, startY_world);
            const i32 topY = std::min(world::MIN_BUILD_HEIGHT + world::CHUNK_HEIGHT - 8, endY_world);

            for (i32 y = topY; y > bottomY; --y) {

                const f32 dy = (static_cast<f32>(y) - 0.5f - centerY) / verticalRadius;

                // 检查是否应该跳过
                if (shouldSkipEllipsoidPosition(dx, dy, dz, y)) {
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
                    (y < world::MAX_BUILD_HEIGHT - 1) ? chunk.getBlockState(lx, y + 1, lz) : nullptr;

                // 检查是否可以雕刻
                if (!canCarveBlock(state, aboveState)) {
                    continue;
                }

                // MC: 标记当前方块是否为草地或菌丝
                bool hasGrassOrMycelium = state->is(VanillaBlocks::GRASS_BLOCK) || state->is(VanillaBlocks::MYCELIUM);

                // 标记为已雕刻
                carvingMask.setCarved(lx, y, lz);

                // MC 1.21: 通过含水层决定雕刻后方块
                // 如果含水层可用，使用 computeSubstance 替代硬编码熔岩/空气判断
                const BlockState* carveState = nullptr;
                if (context.hasAquifer()) {
                    carveState = context.aquifer()->computeSubstance(worldX, y, worldZ, 0.0);
                }

                if (carveState) {
                    // 含水层返回了有效方块状态（水、熔岩或空气）
                    chunk.setBlockState(lx, y, lz, carveState);
                } else {
                    // 含水层返回 nullptr（保持默认/石头），使用回退逻辑
                    const i32 lavaLevel = getLavaLevel();
                    if (y < lavaLevel) {
                        const BlockState* lava = VanillaBlocks::getState(VanillaBlocks::LAVA);
                        if (lava) {
                            chunk.setBlockState(lx, y, lz, lava);
                        }
                    } else {
                        const BlockState* air = getCaveAirState();
                        if (air) {
                            chunk.setBlockState(lx, y, lz, air);
                        }
                    }
                }

                // MC: 如果之前雕刻到了草地/菌丝，检查下方方块是否为泥土
                // 如果是，替换为泥土（后续接入生物群系系统后应替换为生物群系表层材料）
                // NetherWorldCarver 不执行此替换
                if (handlesSurfaceReplacement() && hasGrassOrMycelium && y > world::MIN_BUILD_HEIGHT) {
                    const BlockState* belowState = chunk.getBlockState(lx, y - 1, lz);
                    if (belowState && belowState->is(VanillaBlocks::DIRT)) {
                        // TODO: 应替换为生物群系的 topMaterial，暂时使用泥土
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
    ChunkCoord chunkX, ChunkCoord chunkZ, f32 x, f32 z, i32 step, i32 maxSteps, f32 radius)
{
    const f32 chunkCenterX = static_cast<f32>(chunkX * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2);
    const f32 chunkCenterZ = static_cast<f32>(chunkZ * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2);

    const f32 dx = x - chunkCenterX;
    const f32 dz = z - chunkCenterZ;

    const f32 remainingSteps = static_cast<f32>(maxSteps - step);
    const f32 maxDist = radius + 2.0f + static_cast<f32>(world::CHUNK_WIDTH);

    return dx * dx + dz * dz - remainingSteps * remainingSteps <= maxDist * maxDist;
}

template <typename Config>
bool WorldCarver<Config>::checkAreaForFluid(ChunkPrimer& chunk,
    ChunkCoord /*chunkX*/,
    ChunkCoord /*chunkZ*/,
    i32 minX,
    i32 maxX,
    i32 minY,
    i32 maxY,
    i32 minZ,
    i32 maxZ) const
{
    // 检查区域内是否有液体（水/熔岩）
    for (i32 lx = minX; lx < maxX; ++lx) {
        for (i32 lz = minZ; lz < maxZ; ++lz) {
            for (i32 y = minY - 1; y <= maxY + 1; ++y) {
                if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
                    continue;
                }

                const BlockState* state = chunk.getBlockState(lx, y, lz);
                if (state) {
                    // 检查是否是水或熔岩
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
template class WorldCarver<ProbabilityConfig>;

} // namespace mc
