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

#include "IWorldSpawnAdapter.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

namespace mc::world::spawn {
IWorldSpawnAdapter::IWorldSpawnAdapter(IWorld& world)
    : m_world(world)
{}

const BlockState* IWorldSpawnAdapter::getBlockState(i32 x, i32 y, i32 z) const
{
    return m_world.getBlockState(x, y, z);
}

bool IWorldSpawnAdapter::isWithinWorldBorder(i32 x, i32 y, i32 z) const
{
    return m_world.worldBorder().isWithinBounds(BlockPos(x, y, z));
}

i32 IWorldSpawnAdapter::getHeight(HeightmapType type, i32 x, i32 z) const
{
    const auto* chunk = m_world.chunkManager()->tryToGetChunkInMem(toChunkCoord(x), toChunkCoord(z));
    return chunk == nullptr ? m_world.getHeight(x, z) : chunk->getTopBlockY(type, toLocalCoord(x), toLocalCoord(z)) + 1;
}

BiomeId IWorldSpawnAdapter::getBiome(i32 x, i32 y, i32 z) const
{
    const auto* chunk = m_world.chunkManager()->tryToGetChunkInMem(toChunkCoord(x), toChunkCoord(z));
    return chunk == nullptr ? Biomes::Plains : chunk->getBiomeAtBlock(toLocalCoord(x), y, toLocalCoord(z));
}

u64 IWorldSpawnAdapter::seed() const
{
    return m_world.seed();
}

Difficulty IWorldSpawnAdapter::difficulty() const
{
    return m_world.difficulty();
}

SpawnEnvironment IWorldSpawnAdapter::environment(const Vector3i& pos) const
{
    const auto dimension = m_world.dimension();
    return SpawnEnvironment{m_world.getSkyLight(pos.x, pos.y, pos.z),
        m_world.getBlockLight(pos.x, pos.y, pos.z),
        m_world.getSkyDarkening(),
        m_world.isThundering(),
        m_world.seaLevel(),
        dimension == -1 ? 15 : 0,
        dimension == -1      ? 7
            : dimension == 1 ? 15
                             : 0,
        dimension == 1 ? 15 : 7,
        m_world.environmentAttributes().getValue(attribute::EnvironmentAttributes::SURFACE_SLIME_SPAWN_CHANCE(),
            Vector3(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f))};
}

bool IWorldSpawnAdapter::hasNearbyPlayer(const Vector3d& center, f64 distance) const
{
    for (const auto* entity : m_world.getPlayers()) {
        const auto* player = dynamic_cast<const Player*>(entity);
        if (player == nullptr || player->isCreative() || player->isSpectator()) {
            continue;
        }
        const auto position = player->position();
        const f64 dx = static_cast<f64>(position.x) - center.x;
        const f64 dy = static_cast<f64>(position.y) - center.y;
        const f64 dz = static_cast<f64>(position.z) - center.z;
        if (dx * dx + dy * dy + dz * dz < distance * distance) {
            return true;
        }
    }
    return false;
}
} // namespace mc::world::spawn
