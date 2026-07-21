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

#pragma once

#include "../../../core/Constants.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/biome/BiomeRegistry.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#include <optional>

namespace mc {

/**
 * @brief 玩家出生点辅助工具
 *
 * 基于区块高度图扫描实现无天花板维度的出生点查找。
 */
class SpawnLocationHelper {
public:
    /**
     * @brief 在单列上查找可用出生点
     *
     * @param world 世界访问接口
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @param requireValidSpawnBlock 是否要求生物群系表层方块满足出生表面约束
     * @return 可用出生点；找不到时返回空
     */
    [[nodiscard]] static std::optional<BlockPos> findSpawnLocation(
        const IWorld& world, i32 x, i32 z, bool requireValidSpawnBlock)
    {
        const ChunkCoord chunkX = x >> world::CHUNK_SHIFT;
        const ChunkCoord chunkZ = z >> world::CHUNK_SHIFT;
        const ChunkData* chunk = world.getChunk(chunkX, chunkZ);
        if (chunk == nullptr) {
            return std::nullopt;
        }

        const i32 localX = x & world::CHUNK_MASK;
        const i32 localZ = z & world::CHUNK_MASK;
        const BiomeId biomeId = chunk->getBiomeAtBlock(localX, 0, localZ);
        const Biome& biome = BiomeRegistry::instance().get(biomeId);
        const BlockState* surfaceState = biome.surfaceBlock();
        if (surfaceState == nullptr) {
            return std::nullopt;
        }

        if (requireValidSpawnBlock && !_isValidSpawnSurface(*surfaceState)) {
            return std::nullopt;
        }

        const i32 motionBlockingY = _getTopBlockY(*chunk, localX, localZ, HeightmapType::MotionBlocking);
        if (motionBlockingY < 0) {
            return std::nullopt;
        }

        const i32 worldSurfaceY = _getTopBlockY(*chunk, localX, localZ, HeightmapType::WorldSurface);
        const i32 oceanFloorY = _getTopBlockY(*chunk, localX, localZ, HeightmapType::OceanFloor);
        if (worldSurfaceY <= motionBlockingY && worldSurfaceY > oceanFloorY) {
            return std::nullopt;
        }

        for (i32 y = motionBlockingY + 1; y >= world::MIN_BUILD_HEIGHT; --y) {
            const fluid::FluidState* fluidState = world.getFluidState(x, y, z);
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                break;
            }

            const BlockState* currentState = world.getBlockState(x, y, z);
            if (currentState != nullptr && currentState->stateId() == surfaceState->stateId()) {
                return BlockPos(x, y + 1, z);
            }
        }

        return std::nullopt;
    }

    /**
     * @brief 在整个区块内按原版顺序扫描出生点
     *
     * @param world 世界访问接口
     * @param chunkPos 目标区块
     * @param requireValidSpawnBlock 是否要求生物群系表层方块满足出生表面约束
     * @return 第一个可用出生点；找不到时返回空
     */
    [[nodiscard]] static std::optional<BlockPos> findSpawnLocationInChunk(
        const IWorld& world, const ChunkPos& chunkPos, bool requireValidSpawnBlock)
    {
        for (i32 x = chunkPos.worldX(); x < chunkPos.worldX() + world::CHUNK_WIDTH; ++x) {
            for (i32 z = chunkPos.worldZ(); z < chunkPos.worldZ() + world::CHUNK_WIDTH; ++z) {
                const auto spawnPos = findSpawnLocation(world, x, z, requireValidSpawnBlock);
                if (spawnPos.has_value()) {
                    return spawnPos;
                }
            }
        }

        return std::nullopt;
    }

private:
    /**
     * @brief 判定方块是否可作为原版出生表面
     *
     * 当前仓库还没有完整的 `BlockTags::VALID_SPAWN`，这里先按需要的语义
     * 约束到"非流体、非树叶/植物、可阻挡移动"的表层方块。
     */
    [[nodiscard]] static bool _isValidSpawnSurface(const BlockState& state)
    {
        if (!state.blocksMovement() || state.isLiquid()) {
            return false;
        }

        const Material& material = state.getMaterial();
        return &material != &Material::LEAVES && &material != &Material::PLANT;
    }

    /**
     * @brief 计算指定高度图语义下的列顶端
     *
     * 这里不直接依赖区块缓存的高度图，避免未初始化高度图时出现 `y` / `y+1`
     * 语义不一致的问题。
     */
    [[nodiscard]] static i32 _getTopBlockY(const ChunkData& chunk, i32 localX, i32 localZ, HeightmapType type)
    {
        for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
            const BlockState* state = chunk.getBlockState(localX, y, localZ);
            if (_matchesHeightmap(type, state)) {
                return y + 1;
            }
        }

        return -1;
    }

    /**
     * @brief 按高度图语义匹配当前方块
     *
     * 判定逻辑统一委托 Heightmap::isOpaqueForType，避免与 Heightmap 内部判定复制漂移
     * （历史上两份复制曾各自演化导致出生点与高度图判定不一致）。
     */
    [[nodiscard]] static bool _matchesHeightmap(HeightmapType type, const BlockState* state)
    {
        return Heightmap::isOpaqueForType(type, state);
    }
};

} // namespace mc
