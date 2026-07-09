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

#include "BlueIceFeature.hpp"

#include "../../../../util/Direction.hpp"
#include "../../../WorldConstants.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"

#include <algorithm>

namespace mc {

bool BlueIceFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BlueIceFeatureConfig& config, i32 seaLevel)
{
    if (config.blueIceState == nullptr || config.packedIceState == nullptr) {
        return false;
    }

    // 每次特征调用仅选一个起点，再做 200 次邻接扩散
    const i32 placeX = pos.x + random.nextInt(world::CHUNK_WIDTH);
    const i32 placeZ = pos.z + random.nextInt(world::CHUNK_WIDTH);
    const i32 oceanFloorY = _findOceanFloorY(world, placeX, placeZ);
    if (oceanFloorY <= 0) {
        return false;
    }

    const BlockPos startPos(placeX, oceanFloorY + 1, placeZ);
    if (startPos.y > seaLevel - 1) {
        return false;
    }

    const BlockPos belowPos(startPos.x, startPos.y - 1, startPos.z);
    if (!_isWater(world, startPos) && !_isWater(world, belowPos)) {
        return false;
    }

    bool hasPackedIceNeighbor = false;
    for (Direction direction : Directions::all()) {
        if (direction == Direction::Down) {
            continue;
        }

        const BlockPos neighborPos = startPos.offset(direction);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState != nullptr && neighborState->is(&config.packedIceState->owner())) {
            hasPackedIceNeighbor = true;
            break;
        }
    }

    if (!hasPackedIceNeighbor) {
        return false;
    }

    world.setBlockState(startPos, config.blueIceState);

    const i32 spreadAttempts = std::max(1, config.spreadAttempts);
    for (i32 i = 0; i < spreadAttempts; ++i) {
        const i32 dy = random.nextInt(5) - random.nextInt(6);
        i32 range = 3;
        if (dy < 2) {
            range += dy / 2;
        }

        if (range < 1) {
            continue;
        }

        const BlockPos targetPos(startPos.x + random.nextInt(range) - random.nextInt(range),
            startPos.y + dy,
            startPos.z + random.nextInt(range) - random.nextInt(range));

        if (!_isReplaceableForSpread(world, targetPos, config)) {
            continue;
        }

        for (Direction direction : Directions::all()) {
            const BlockPos neighborPos = targetPos.offset(direction);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState != nullptr && neighborState->is(&config.blueIceState->owner())) {
                world.setBlockState(targetPos, config.blueIceState);
                break;
            }
        }
    }

    return true;
}

bool BlueIceFeature::_isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || VanillaBlocks::WATER == nullptr) {
        return false;
    }

    return state->is(VanillaBlocks::WATER);
}

bool BlueIceFeature::_isReplaceableForSpread(
    WorldGenRegion& world, const BlockPos& pos, const BlueIceFeatureConfig& config) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    if (state->isAir()) {
        return true;
    }

    if (_isWater(world, pos)) {
        return true;
    }

    if (state->is(&config.packedIceState->owner())) {
        return true;
    }

    return VanillaBlocks::ICE != nullptr && state->is(VanillaBlocks::ICE);
}

i32 BlueIceFeature::_findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) const
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT + 1; --y) {
        const BlockState* state = world.getBlockState(x, y, z);
        if (state == nullptr || state->isAir()) {
            continue;
        }

        if (VanillaBlocks::WATER != nullptr && state->is(VanillaBlocks::WATER)) {
            continue;
        }

        return y;
    }

    return -1;
}

ConfiguredBlueIceFeature::ConfiguredBlueIceFeature(
    std::unique_ptr<BlueIceFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBlueIceFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    return m_feature.place(region, random, pos, *m_config, generator.seaLevel());
}

} // namespace mc
