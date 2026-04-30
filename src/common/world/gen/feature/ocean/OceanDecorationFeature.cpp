#include "OceanDecorationFeature.hpp"

#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../WorldConstants.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../util/Direction.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace mc {

bool OceanDecorationFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const OceanDecorationFeatureConfig& config)
{
    if (config.tries <= 0) {
        return false;
    }

    bool placedAny = false;
    for (i32 i = 0; i < config.tries; ++i) {
        const i32 placeX = pos.x + random.nextInt(16);
        const i32 placeZ = pos.z + random.nextInt(16);
        const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
        if (oceanFloorY <= 0) {
            continue;
        }

        const BlockPos centerPos(placeX, oceanFloorY + 1, placeZ);
        if (!isWater(world, centerPos) || !hasSolidSupport(world, centerPos.down())) {
            continue;
        }

        if (placeSingleDecoration(world, random, centerPos, config)) {
            placedAny = true;
        }
    }

    return placedAny;
}

bool OceanDecorationFeature::isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlock(pos);
    if (state == nullptr || VanillaBlocks::WATER == nullptr) {
        return false;
    }

    return state->is(VanillaBlocks::WATER);
}

bool OceanDecorationFeature::hasSolidSupport(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlock(pos);
    return state != nullptr && !state->isAir() && state->owner().isSolid(*state);
}

i32 OceanDecorationFeature::findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) const
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT + 1; --y) {
        const BlockState* state = world.getBlock(x, y, z);
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

bool OceanDecorationFeature::placeSingleDecoration(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& centerPos,
    const OceanDecorationFeatureConfig& config)
{
    bool placed = false;

    const BlockPos floorPos = centerPos.down();
    if (config.prismarineState != nullptr) {
        world.setBlock(floorPos, config.prismarineState);
        placed = true;
    }

    if (config.conduitState != nullptr && isWater(world, centerPos)) {
        world.setBlock(centerPos, config.conduitState);
        placed = true;
    }

    const auto directions = Directions::horizontal();
    for (Direction direction : directions) {
        const BlockPos ringPos = centerPos.offset(direction);
        if (!isWater(world, ringPos) || !hasSolidSupport(world, ringPos.down())) {
            continue;
        }

        if (config.prismarineStairsState != nullptr && random.nextBoolean()) {
            world.setBlock(ringPos, config.prismarineStairsState);
            placed = true;
            continue;
        }

        if (config.prismarineSlabState != nullptr) {
            world.setBlock(ringPos, config.prismarineSlabState);
            placed = true;
        }
    }

    const i32 driedKelpCount = std::max(0, config.driedKelpCount);
    for (i32 i = 0; i < driedKelpCount; ++i) {
        const i32 dx = random.nextInt(5) - 2;
        const i32 dz = random.nextInt(5) - 2;
        const BlockPos kelpPos(centerPos.x + dx, centerPos.y, centerPos.z + dz);

        if (config.driedKelpBlockState == nullptr || !isWater(world, kelpPos) || !hasSolidSupport(world, kelpPos.down())) {
            continue;
        }

        world.setBlock(kelpPos, config.driedKelpBlockState);
        placed = true;
    }

    if (config.turtleEggState != nullptr) {
        const Direction nestDirection = directions[static_cast<size_t>(random.nextInt(4))];
        const BlockPos nestPos = centerPos.offset(nestDirection, 2);

        if (hasSolidSupport(world, nestPos.down())) {
            if (config.sandState != nullptr) {
                world.setBlock(nestPos.down(), config.sandState);
            }

            const i32 eggs = random.nextInt(4) + 1;
            const BlockState* eggState = &config.turtleEggState->with(BlockStateProperties::EGGS_1_4(), eggs);
            world.setBlock(nestPos, eggState);
            placed = true;
        }
    }

    if (config.bubbleColumnState != nullptr && config.magmaState != nullptr) {
        const Direction ventDirection = directions[static_cast<size_t>(random.nextInt(4))];
        const BlockPos ventSourcePos = centerPos.offset(ventDirection, 2).down();
        if (hasSolidSupport(world, ventSourcePos)) {
            world.setBlock(ventSourcePos, config.magmaState);

            const i32 maxColumnHeight = std::max(1, config.bubbleColumnMaxHeight);
            for (i32 yOffset = 1; yOffset <= maxColumnHeight; ++yOffset) {
                const BlockPos bubblePos(ventSourcePos.x, ventSourcePos.y + yOffset, ventSourcePos.z);
                if (!isWater(world, bubblePos)) {
                    break;
                }

                world.setBlock(bubblePos, config.bubbleColumnState);
                placed = true;
            }
        }
    }

    return placed;
}

ConfiguredOceanDecorationFeature::ConfiguredOceanDecorationFeature(
    std::unique_ptr<OceanDecorationFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredOceanDecorationFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

std::vector<std::unique_ptr<ConfiguredOceanDecorationFeature>> OceanDecorationFeatures::s_features;

void OceanDecorationFeatures::initialize()
{
    s_features.clear();
    s_features.push_back(createOceanProps());
}

const std::vector<std::unique_ptr<ConfiguredOceanDecorationFeature>>& OceanDecorationFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredOceanDecorationFeature>> OceanDecorationFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredOceanDecorationFeature> OceanDecorationFeatures::createOceanProps()
{
    auto config = std::make_unique<OceanDecorationFeatureConfig>();
    config->conduitState = VanillaBlocks::getState(VanillaBlocks::CONDUIT);
    config->driedKelpBlockState = VanillaBlocks::getState(VanillaBlocks::DRIED_KELP_BLOCK);
    config->turtleEggState = VanillaBlocks::getState(VanillaBlocks::TURTLE_EGG);
    config->bubbleColumnState = VanillaBlocks::getState(VanillaBlocks::BUBBLE_COLUMN);
    config->prismarineStairsState = VanillaBlocks::getState(VanillaBlocks::PRISMARINE_STAIRS);
    config->prismarineSlabState = VanillaBlocks::getState(VanillaBlocks::PRISMARINE_SLAB);
    config->prismarineState = VanillaBlocks::getState(VanillaBlocks::PRISMARINE);
    config->magmaState = VanillaBlocks::getState(VanillaBlocks::MAGMA);
    config->sandState = VanillaBlocks::getState(VanillaBlocks::SAND);
    config->tries = 2;
    config->bubbleColumnMaxHeight = 10;
    config->driedKelpCount = 5;

    return std::make_unique<ConfiguredOceanDecorationFeature>(std::move(config), "ocean_props");
}

} // namespace mc
