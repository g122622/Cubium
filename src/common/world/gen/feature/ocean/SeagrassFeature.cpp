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

#include "SeagrassFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace mc {

namespace {

[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z)
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > world::MIN_BUILD_HEIGHT) {
        return oceanFloorY;
    }

    // 某些测试会绕过高度图更新，回退到显式扫描。
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

} // namespace

// ============================================================================
// SeagrassFeature 实现
// ============================================================================

bool SeagrassFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SeagrassFeatureConfig& config)
{
    if (!config.seagrassState) {
        return false;
    }

    // 每次调用尝试多个随机偏移点
    const i32 tries = std::max(1, config.tries);
    const i32 spread = std::max(1, config.horizontalSpread);

    bool placedAny = false;
    for (i32 attempt = 0; attempt < tries; ++attempt) {
        const i32 dx = random.nextInt(spread) - random.nextInt(spread);
        const i32 dz = random.nextInt(spread) - random.nextInt(spread);

        const i32 placeX = pos.x + dx;
        const i32 placeZ = pos.z + dz;
        const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
        if (oceanFloorY <= world::MIN_BUILD_HEIGHT) {
            continue;
        }

        const BlockPos placePos(placeX, oceanFloorY + 1, placeZ);
        if (!_canPlaceAt(world, placePos, *config.seagrassState)) {
            continue;
        }

        const bool shouldPlaceTall = config.tallSeagrassChance > 0.0f && config.tallSeagrassLowerState != nullptr &&
            config.tallSeagrassUpperState != nullptr && random.nextFloat() < config.tallSeagrassChance;

        if (shouldPlaceTall) {
            const BlockPos abovePos(placePos.x, placePos.y + 1, placePos.z);
            if (_isWater(world, abovePos)) {
                _placeTallSeagrass(world, placePos, config);
                placedAny = true;
                continue;
            }
        }

        world.setBlockState(placePos, config.seagrassState);
        placedAny = true;
    }

    return placedAny;
}

bool SeagrassFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const BlockState& seagrassState) const
{
    MC_UNUSED(seagrassState);

    if (!_isWater(world, pos)) {
        return false;
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && belowState->isSolid();
}

bool SeagrassFeature::_isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    // 检查是否为水方块
    if (VanillaBlocks::WATER && state->blockId() == VanillaBlocks::WATER->blockId()) {
        return true;
    }

    return false;
}

bool SeagrassFeature::_placeTallSeagrass(
    WorldGenRegion& world, const BlockPos& pos, const SeagrassFeatureConfig& config) const
{
    // 放置下半部分
    world.setBlockState(pos, config.tallSeagrassLowerState);

    // 放置上半部分
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    world.setBlockState(abovePos, config.tallSeagrassUpperState);

    return true;
}

// ============================================================================
// ConfiguredSeagrassFeature 实现
// ============================================================================

ConfiguredSeagrassFeature::ConfiguredSeagrassFeature(
    std::unique_ptr<SeagrassFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredSeagrassFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
