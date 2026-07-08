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

#include "MultifaceGrowthFeature.hpp"

#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/MultifaceSpreader.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace cave {

namespace {

/// MC MultifaceGrowthFeature.isAirOrWater：空气或水（nullptr 视为空气）。
bool isAirOrWater(const BlockState* state)
{
    if (state == nullptr || state->isAir()) {
        return true;
    }
    return state->is(VanillaBlocks::WATER);
}

/// MC MultifaceGrowthConfiguration.getShuffledDirections：打乱 validDirections。
std::vector<Direction> getShuffledDirections(math::IRandom& random, const MultifaceGrowthConfig& config)
{
    std::vector<Direction> dirs = config.validDirections;
    random.shuffle(dirs);
    return dirs;
}

/// MC MultifaceGrowthConfiguration.getShuffledDirectionsExcept：打乱并排除某方向。
std::vector<Direction> getShuffledDirectionsExcept(
    math::IRandom& random, const MultifaceGrowthConfig& config, Direction excluded)
{
    std::vector<Direction> dirs;
    dirs.reserve(config.validDirections.size());
    for (Direction dir : config.validDirections) {
        if (dir != excluded) {
            dirs.push_back(dir);
        }
    }
    random.shuffle(dirs);
    return dirs;
}

/// MC canBePlacedOn 命中：方块标签包含 或 方块列表包含。
bool canBePlacedOn(const MultifaceGrowthConfig& config, const BlockState* state)
{
    if (state == nullptr) {
        return false;
    }
    if (config.canBePlacedOnTag != nullptr && config.canBePlacedOnTag->contains(*state)) {
        return true;
    }
    for (const Block* block : config.canBePlacedOnBlocks) {
        if (state->is(block)) {
            return true;
        }
    }
    return false;
}

/// MC MultifaceGrowthFeature.placeGrowthIfPossible：单点放置 + 概率扩散。
bool placeGrowthIfPossible(IWorld& world,
    const BlockPos& pos,
    const BlockState* currentState,
    const MultifaceGrowthConfig& config,
    math::IRandom& random,
    const std::vector<Direction>& directions)
{
    for (Direction direction : directions) {
        const BlockPos neighbor = pos.offset(direction);
        const BlockState* neighborState = world.getBlockState(neighbor);
        if (!canBePlacedOn(config, neighborState)) {
            continue;
        }
        // MC: placeBlock.getStateForPlacement(currentState, level, pos, direction)。
        const BlockState* placed = config.placeBlock->getStateForPlacement(currentState, world, pos, direction);
        if (placed == nullptr) {
            return false;
        }
        world.setBlockState(pos, placed, 3);
        if (random.nextFloat() < config.chanceOfSpreading) {
            // MC: placeBlock.getSpreader().spreadFromFaceTowardRandomDirection(placed, level, pos, direction, random,
            // true)。
            blocks::MultifaceSpreader spreader(*config.placeBlock);
            spreader.spreadFromFaceTowardRandomDirection(*placed, world, pos, direction, random);
        }
        return true;
    }
    return false;
}

} // namespace

// ============================================================================
// ConfiguredMultifaceGrowthFeature
// ============================================================================

ConfiguredMultifaceGrowthFeature::ConfiguredMultifaceGrowthFeature(
    std::unique_ptr<MultifaceGrowthConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredMultifaceGrowthFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, generator, random, pos, *m_config);
}

// ============================================================================
// MultifaceGrowthFeature
// ============================================================================

bool MultifaceGrowthFeature::place(IWorld& world,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin,
    const MultifaceGrowthConfig& config)
{
    const BlockState* originState = world.getBlockState(origin);
    if (!isAirOrWater(originState)) {
        return false;
    }

    const std::vector<Direction> list = getShuffledDirections(random, config);
    if (placeGrowthIfPossible(world, origin, originState, config, random, list)) {
        return true;
    }

    // MC: 沿每个方向逐步搜索 searchRange 步。注意 MC 源码每次 setWithOffset(blockpos, direction)
    // 相对 origin 偏移（非累积），即反复在同一相邻点 origin+direction 上以不同随机朝向重试。
    for (Direction direction : list) {
        const std::vector<Direction> list1 =
            getShuffledDirectionsExcept(random, config, Directions::opposite(direction));
        const BlockPos offsetPos = origin.offset(direction);
        for (i32 i = 0; i < config.searchRange; ++i) {
            const BlockState* blockstate = world.getBlockState(offsetPos);
            if (!isAirOrWater(blockstate) && !(blockstate != nullptr && blockstate->is(config.placeBlock))) {
                break;
            }
            if (placeGrowthIfPossible(world, offsetPos, blockstate, config, random, list1)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace cave
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
