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

#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"

namespace mc {
namespace world {
namespace spawn {

/**
 * @brief IWorld 到 ISpawnWorldReader 的适配器
 *
 * 将 IWorld 接口适配为 ISpawnWorldReader，供 EntitySpawnPlacementRegistry
 * 使用生成放置规则检查。适用于刷怪笼、增援生成等需要验证生成位置的场景。
 *
 * 使用方式：
 * @code
 * IWorldSpawnAdapter adapter(world);
 * bool canSpawn = EntitySpawnPlacementRegistry::canSpawnEntity(
 *     "minecraft:zombie", adapter, SpawnReason::Reinforcement, pos, rng);
 * @endcode
 */
class IWorldSpawnAdapter final : public ISpawnWorldReader {
public:
    explicit IWorldSpawnAdapter(IWorld& world)
        : m_world(world)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return m_world.getBlockState(BlockPos(x, y, z));
    }

    [[nodiscard]] bool isInWorldBounds(i32 x, i32 y, i32 z) const override
    {
        return m_world.isWithinWorldBounds(x, y, z);
    }

    [[nodiscard]] i32 getHeight(chunk::HeightmapType type, i32 x, i32 z) const override
    {
        const ChunkCoord chunkX = toChunkCoord(x);
        const ChunkCoord chunkZ = toChunkCoord(z);
        const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
        if (chunk == nullptr) {
            return m_world.getHeight(x, z);
        }
        const i32 localX = toLocalCoord(x);
        const i32 localZ = toLocalCoord(z);
        return chunk->getTopBlockY(type, localX, localZ);
    }

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override
    {
        const ChunkCoord chunkX = toChunkCoord(x);
        const ChunkCoord chunkZ = toChunkCoord(z);
        const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
        if (chunk == nullptr) {
            return Biomes::Plains;
        }
        const i32 localX = toLocalCoord(x);
        const i32 localZ = toLocalCoord(z);
        return chunk->getBiomeAtBlock(localX, y, localZ);
    }

    [[nodiscard]] u64 seed() const override { return m_world.seed(); }

    [[nodiscard]] Difficulty difficulty() const override { return m_world.difficulty(); }

    [[nodiscard]] i64 dayTime() const override { return m_world.dayTime(); }

    [[nodiscard]] i32 getMaxLocalRawBrightness(i32 x, i32 y, i32 z) const override
    {
        return m_world.getMaxLocalRawBrightness(BlockPos(x, y, z));
    }

private:
    IWorld& m_world;
};

} // namespace spawn
} // namespace world
} // namespace mc
