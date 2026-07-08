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

#include "DripstoneClusterFeature.hpp"

#include "common/util/Direction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/Column.hpp"
#include "common/world/gen/feature/DripstoneUtils.hpp"
#include "common/world/gen/valueprovider/FloatProvider.hpp"

namespace mc::world::gen::feature::cave {

// ============================================================================
// DripstoneClusterConfig
// ============================================================================

DripstoneClusterConfig::DripstoneClusterConfig(i32 searchRange,
    std::unique_ptr<valueprovider::IntProvider> h,
    std::unique_ptr<valueprovider::IntProvider> r,
    i32 maxHeightDiff,
    i32 hDeviation,
    std::unique_ptr<valueprovider::IntProvider> blockLayerThickness,
    std::unique_ptr<valueprovider::FloatProvider> dens,
    std::unique_ptr<valueprovider::FloatProvider> wet,
    f32 chanceAtMaxDist,
    i32 maxDistEdge,
    i32 maxDistCenter)
    : floorToCeilingSearchRange(searchRange)
    , height(std::move(h))
    , radius(std::move(r))
    , maxStalagmiteStalactiteHeightDiff(maxHeightDiff)
    , heightDeviation(hDeviation)
    , dripstoneBlockLayerThickness(std::move(blockLayerThickness))
    , density(std::move(dens))
    , wetness(std::move(wet))
    , chanceOfDripstoneColumnAtMaxDistanceFromCenter(chanceAtMaxDist)
    , maxDistanceFromEdgeAffectingChanceOfDripstoneColumn(maxDistEdge)
    , maxDistanceFromCenterAffectingHeightBias(maxDistCenter)
{}

// ============================================================================
// ConfiguredDripstoneClusterFeature
// ============================================================================

ConfiguredDripstoneClusterFeature::ConfiguredDripstoneClusterFeature(
    std::unique_ptr<DripstoneClusterConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredDripstoneClusterFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// DripstoneClusterFeature
// ============================================================================

namespace {

[[nodiscard]] bool isLava(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && state->is(VanillaBlocks::LAVA);
}

[[nodiscard]] bool isWaterFluid(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    const fluid::FluidState* fluid = state->getFluidState();
    return fluid != nullptr && fluid->getFluid().isIn(fluid::FluidTags::WATER());
}

[[nodiscard]] bool canBeAdjacentToWater(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state != nullptr && BlockTags::BASE_STONE_OVERWORLD().contains(*state)) {
        return true;
    }
    return isWaterFluid(world, pos);
}

[[nodiscard]] bool canPlacePool(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    if (state->is(VanillaBlocks::WATER) || state->is(VanillaBlocks::DRIPSTONE_BLOCK) ||
        state->is(VanillaBlocks::POINTED_DRIPSTONE)) {
        return false;
    }
    if (isWaterFluid(world, pos.up())) {
        return false;
    }
    for (Direction direction : Directions::horizontal()) {
        if (!canBeAdjacentToWater(world, pos.offset(direction))) {
            return false;
        }
    }
    return canBeAdjacentToWater(world, pos.down());
}

void replaceBlocksWithDripstoneBlocks(IWorld& world, const BlockPos& pos, i32 layerThickness, Direction direction)
{
    BlockPosMutable cursor(pos);
    for (i32 i = 0; i < layerThickness; ++i) {
        if (!DripstoneUtils::placeDripstoneBlockIfPossible(world, cursor)) {
            return;
        }
        cursor.move(direction);
    }
}

[[nodiscard]] double getChanceOfStalagmiteOrStalactite(
    i32 radiusX, i32 radiusZ, i32 dx, i32 dz, const DripstoneClusterConfig& config)
{
    const i32 i = radiusX - std::abs(dx);
    const i32 j = radiusZ - std::abs(dz);
    const i32 k = std::min(i, j);
    return math::clampedMap(static_cast<f64>(k),
        0.0,
        static_cast<f64>(config.maxDistanceFromEdgeAffectingChanceOfDripstoneColumn),
        static_cast<f64>(config.chanceOfDripstoneColumnAtMaxDistanceFromCenter),
        1.0);
}

[[nodiscard]] i32 getDripstoneHeight(
    math::Random& random, i32 dx, i32 dz, f32 density, i32 maxHeight, const DripstoneClusterConfig& config)
{
    if (random.nextFloat() > density) {
        return 0;
    }
    const i32 dist = std::abs(dx) + std::abs(dz);
    // MC: f = clampedMap(i, 0, maxDistCenter, maxHeight/2, 0)；作为正态分布的均值。
    const f32 mean = static_cast<f32>(math::clampedMap(static_cast<f64>(dist),
        0.0,
        static_cast<f64>(config.maxDistanceFromCenterAffectingHeightBias),
        static_cast<f64>(maxHeight) / 2.0,
        0.0));
    // MC randomBetweenBiased(rng, 0, maxHeight, mean, heightDeviation)
    //  = ClampedNormalFloat.sample(rng, mean, heightDeviation, 0, maxHeight)
    return static_cast<i32>(valueprovider::ClampedNormalFloat::sampleStatic(
        random, mean, static_cast<f32>(config.heightDeviation), 0.0f, static_cast<f32>(maxHeight)));
}

} // namespace

bool DripstoneClusterFeature::place(
    IWorld& world, math::Random& random, const BlockPos& pos, const DripstoneClusterConfig& config)
{
    if (!DripstoneUtils::isEmptyOrWater(world, pos)) {
        return false;
    }

    const i32 height = config.height->sample(random);
    const f32 wetness = config.wetness->sample(random);
    const f32 density = config.density->sample(random);
    const i32 radiusX = config.radius->sample(random);
    const i32 radiusZ = config.radius->sample(random);

    for (i32 dx = -radiusX; dx <= radiusX; ++dx) {
        for (i32 dz = -radiusZ; dz <= radiusZ; ++dz) {
            const double chance = getChanceOfStalagmiteOrStalactite(radiusX, radiusZ, dx, dz, config);
            const BlockPos colPos = BlockPos(pos.x + dx, pos.y, pos.z + dz);
            placeColumn(world, random, colPos, dx, dz, wetness, chance, height, density, config);
        }
    }
    return true;
}

void DripstoneClusterFeature::placeColumn(IWorld& world,
    math::Random& random,
    const BlockPos& colPos,
    i32 dx,
    i32 dz,
    f32 wetness,
    double chance,
    i32 maxHeight,
    f32 density,
    const DripstoneClusterConfig& config)
{
    auto optional = Column::scan(world,
        colPos,
        config.floorToCeilingSearchRange,
        static_cast<bool (*)(const BlockState*)>(DripstoneUtils::isEmptyOrWater),
        static_cast<bool (*)(const BlockState*)>(DripstoneUtils::isNeitherEmptyNorWater));
    if (!optional.has_value()) {
        return;
    }

    std::unique_ptr<Column>& column = *optional;
    const std::optional<i32> ceilingOpt = column->getCeiling();
    const std::optional<i32> floorOpt = column->getFloor();
    if (!ceilingOpt.has_value() && !floorOpt.has_value()) {
        return;
    }

    bool placePool = random.nextFloat() < wetness;
    if (placePool && floorOpt.has_value() && canPlacePool(world, BlockPos(colPos.x, *floorOpt, colPos.z))) {
        const i32 floorY = *floorOpt;
        column = column->withFloor(std::optional<i32>(floorY - 1));
        world.setBlockState(colPos.x, floorY, colPos.z, &VanillaBlocks::WATER->defaultState());
    }

    const std::optional<i32> adjustedFloorOpt = column->getFloor();
    bool placeCeiling = random.nextDouble() < chance;
    i32 stalactiteHeight;
    if (ceilingOpt.has_value() && placeCeiling && !isLava(world, BlockPos(colPos.x, *ceilingOpt, colPos.z))) {
        const i32 layer = config.dripstoneBlockLayerThickness->sample(random);
        replaceBlocksWithDripstoneBlocks(world, BlockPos(colPos.x, *ceilingOpt, colPos.z), layer, Direction::Up);
        i32 limit;
        if (adjustedFloorOpt.has_value()) {
            limit = std::min(maxHeight, *ceilingOpt - *adjustedFloorOpt);
        } else {
            limit = maxHeight;
        }
        stalactiteHeight = getDripstoneHeight(random, dx, dz, density, limit, config);
    } else {
        stalactiteHeight = 0;
    }

    bool placeFloor = random.nextDouble() < chance;
    i32 stalagmiteHeight;
    if (adjustedFloorOpt.has_value() && placeFloor && !isLava(world, BlockPos(colPos.x, *adjustedFloorOpt, colPos.z))) {
        const i32 layer = config.dripstoneBlockLayerThickness->sample(random);
        replaceBlocksWithDripstoneBlocks(
            world, BlockPos(colPos.x, *adjustedFloorOpt, colPos.z), layer, Direction::Down);
        if (ceilingOpt.has_value()) {
            stalagmiteHeight = std::max(0,
                stalactiteHeight +
                    random.nextInt(
                        -config.maxStalagmiteStalactiteHeightDiff, config.maxStalagmiteStalactiteHeightDiff));
        } else {
            stalagmiteHeight = getDripstoneHeight(random, dx, dz, density, maxHeight, config);
        }
    } else {
        stalagmiteHeight = 0;
    }

    i32 stalactiteFinal;
    i32 stalagmiteFinal;
    if (ceilingOpt.has_value() && adjustedFloorOpt.has_value() &&
        *ceilingOpt - stalactiteHeight <= *adjustedFloorOpt + stalagmiteHeight) {
        const i32 floorY = *adjustedFloorOpt;
        const i32 ceilingY = *ceilingOpt;
        const i32 low = std::max(ceilingY - stalactiteHeight, floorY + 1);
        const i32 high = std::min(floorY + stalagmiteHeight, ceilingY - 1);
        const i32 split = random.nextInt(low, high + 1);
        const i32 splitBelow = split - 1;
        stalactiteFinal = ceilingY - split;
        stalagmiteFinal = splitBelow - floorY;
    } else {
        stalactiteFinal = stalactiteHeight;
        stalagmiteFinal = stalagmiteHeight;
    }

    const std::optional<i32> heightOpt = column->getHeight();
    const bool merge = random.nextBoolean() && stalactiteFinal > 0 && stalagmiteFinal > 0 && heightOpt.has_value() &&
        stalactiteFinal + stalagmiteFinal == *heightOpt;

    if (ceilingOpt.has_value()) {
        DripstoneUtils::growPointedDripstone(
            world, BlockPos(colPos.x, *ceilingOpt - 1, colPos.z), Direction::Down, stalactiteFinal, merge);
    }
    if (adjustedFloorOpt.has_value()) {
        DripstoneUtils::growPointedDripstone(
            world, BlockPos(colPos.x, *adjustedFloorOpt + 1, colPos.z), Direction::Up, stalagmiteFinal, merge);
    }
}

} // namespace mc::world::gen::feature::cave
