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
#include "../../../core/Constants.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// CarvingMask 实现
// ============================================================================

CarvingMask::CarvingMask(ChunkCoord chunkX, ChunkCoord chunkZ)
    : m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_mask(16 * 16 * world::MAX_BUILD_HEIGHT, false) // 16x16 x height
{}

bool CarvingMask::isCarved(BlockCoord x, i32 y, BlockCoord z) const
{
    if (x < 0 || x >= 16 || z < 0 || z >= 16 || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }
    i32 index = getIndex(x, y, z);
    return m_mask[static_cast<size_t>(index)];
}

void CarvingMask::setCarved(BlockCoord x, i32 y, BlockCoord z)
{
    if (x < 0 || x >= 16 || z < 0 || z >= 16 || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
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
    // 参考 MC 1.16.5: net.minecraft.world.gen.carver.WorldCarver.CAVE_AIR
    return VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
}

template <typename Config>
bool WorldCarver<Config>::isCarvable(const BlockState& state)
{
    // 参考 MC WorldCarver.carvableBlocks（MC 1.16.5）
    // 原版列表：STONE, GRANITE, DIORITE, ANDESITE, DIRT, COARSE_DIRT, PODZOL,
    // GRASS_BLOCK, TERRACOTTA (及所有染色陶瓦), SANDSTONE, RED_SANDSTONE,
    // MYCELIUM, SNOW, PACKED_ICE

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
    // 参考 MC: (state.isIn(Blocks.SAND) || state.isIn(Blocks.GRAVEL)) &&
    // !aboveState.getFluidState().isTagged(FluidTags.WATER)
    bool isSandOrGravel = state->is(VanillaBlocks::SAND) || state->is(VanillaBlocks::GRAVEL);
    if (isSandOrGravel && aboveState) {
        return !aboveState->isLiquid();
    }

    return false;
}

template <typename Config>
bool WorldCarver<Config>::carveEllipsoid(ChunkPrimer& chunk,
    const BiomeProvider& /*biomeProvider*/,
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
    // 参考 MC WorldCarver.func_227208_a_
    const i32 startX = static_cast<i32>(centerX - horizontalRadius - 1.0f);
    const i32 endX = static_cast<i32>(centerX + horizontalRadius + 1.0f);
    const i32 startY = static_cast<i32>(centerY - verticalRadius - 1.0f);
    const i32 endY = static_cast<i32>(centerY + verticalRadius + 1.0f);
    const i32 startZ = static_cast<i32>(centerZ - horizontalRadius - 1.0f);
    const i32 endZ = static_cast<i32>(centerZ + horizontalRadius + 1.0f);

    // 区块边界
    const i32 chunkStartX = chunkX << 4;
    const i32 chunkEndX = chunkStartX + 15;
    const i32 chunkStartZ = chunkZ << 4;
    const i32 chunkEndZ = chunkStartZ + 15;

    // 检查椭球是否在区块范围外
    if (endX < chunkStartX - 16 || startX > chunkEndX + 16 || endZ < chunkStartZ - 16 || startZ > chunkEndZ + 16) {
        return false;
    }

    // 计算区块内有效范围
    const i32 localMinX = std::max(0, startX - chunkStartX);
    const i32 localMaxX = std::min(15, endX - chunkStartX);
    const i32 localMinZ = std::max(0, startZ - chunkStartZ);
    const i32 localMaxZ = std::min(15, endZ - chunkStartZ);

    // 检查是否有水（避免水下雕刻）
    if (checkAreaForFluid(chunk,
            chunkX,
            chunkZ,
            localMinX,
            localMaxX + 1,
            std::max(1, startY),
            std::min(m_maxHeight - 8, endY),
            localMinZ,
            localMaxZ + 1)) {
        return false;
    }

    math::Random rng(static_cast<u64>(seed) + static_cast<u64>(chunkX) + static_cast<u64>(chunkZ));
    bool carved = false;

    for (i32 lx = localMinX; lx <= localMaxX; ++lx) {
        const i32 worldX = (chunkX << 4) + lx;
        const f32 dx = (static_cast<f32>(worldX) + 0.5f - centerX) / horizontalRadius;
        const f32 dxSq = dx * dx;

        for (i32 lz = localMinZ; lz <= localMaxZ; ++lz) {
            const i32 worldZ = (chunkZ << 4) + lz;
            const f32 dz = (static_cast<f32>(worldZ) + 0.5f - centerZ) / horizontalRadius;
            const f32 dzSq = dz * dz;

            // 检查是否在椭球投影范围内
            if (dxSq + dzSq >= 1.0f) {
                continue;
            }

            for (i32 y = endY; y >= startY; --y) {
                // 边界检查
                if (y < 1 || y >= m_maxHeight - 8) {
                    continue;
                }

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

                // 标记为已雕刻
                carvingMask.setCarved(lx, y, lz);

                // 检查上方是否有草地/菌丝，需要替换为泥土
                bool hasGrassAbove = state->is(VanillaBlocks::GRASS_BLOCK);

                // 设置为空气或熔岩
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

                    // 如果上方有草地，替换为泥土
                    if (hasGrassAbove && y < world::MAX_BUILD_HEIGHT - 1) {
                        const BlockState* dirt = VanillaBlocks::getState(VanillaBlocks::DIRT);
                        if (dirt) {
                            chunk.setBlockState(lx, y + 1, lz, dirt);
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
    // 参考 MC WorldCarver.func_222702_a_
    const f32 chunkCenterX = static_cast<f32>(chunkX * 16 + 8);
    const f32 chunkCenterZ = static_cast<f32>(chunkZ * 16 + 8);

    const f32 dx = x - chunkCenterX;
    const f32 dz = z - chunkCenterZ;

    const f32 remainingSteps = static_cast<f32>(maxSteps - step);
    const f32 maxDist = radius + 2.0f + 16.0f;

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
    // 参考 MC WorldCarver.func_222700_a_
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
