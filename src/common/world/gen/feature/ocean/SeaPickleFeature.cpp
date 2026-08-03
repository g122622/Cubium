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

#include "SeaPickleFeature.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../WorldConstants.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <algorithm>
#include <memory>
#include <utility>

namespace mc {

namespace {

[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z)
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    // 测试场景中高度图可能未初始化，回退到显式扫描。
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
// SeaPickleFeature 实现
// ============================================================================

bool SeaPickleFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SeaPickleFeatureConfig& config)
{
    if (!config.seaPickleState) {
        return false;
    }

    bool placedAny = false;

    // 多次尝试放置
    for (i32 i = 0; i < config.tries; ++i) {
        // 在附近随机选择位置
        i32 dx = random.nextInt(8) - random.nextInt(8);
        i32 dz = random.nextInt(8) - random.nextInt(8);

        const i32 placeX = pos.x + dx;
        const i32 placeZ = pos.z + dz;
        const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
        if (oceanFloorY <= 0) {
            continue;
        }

        const BlockPos placePos(placeX, oceanFloorY + 1, placeZ);

        if (_canPlaceAt(world, placePos, *config.seaPickleState)) {
            // 随机数量 (1-4)
            const i32 maxCount = std::clamp(config.maxCount, 1, 4);
            const i32 count = random.nextInt(maxCount) + 1;

            const BlockState* pickleState = config.seaPickleState;
            if (VanillaBlocks::SEA_PICKLE != nullptr && config.seaPickleState->is(VanillaBlocks::SEA_PICKLE)) {
                pickleState = &config.seaPickleState->with(BlockStateProperties::PICKLES_1_4(), count);
            }

            world.setBlockState(placePos, pickleState);
            placedAny = true;
        }
    }

    return placedAny;
}

bool SeaPickleFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const BlockState& pickleState) const
{
    MC_UNUSED(pickleState);

    if (!_isWater(world, pos)) {
        return false;
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && belowState->isSolid();
}

bool SeaPickleFeature::_isWater(WorldGenRegion& world, const BlockPos& pos) const
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

// ============================================================================
// ConfiguredSeaPickleFeature 实现
// ============================================================================

ConfiguredSeaPickleFeature::ConfiguredSeaPickleFeature(
    std::unique_ptr<SeaPickleFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredSeaPickleFeature::place(WorldGenRegion& region,
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
