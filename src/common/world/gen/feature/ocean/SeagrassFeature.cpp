#include "SeagrassFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"

#include <algorithm>

namespace mc {

namespace {

[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) {
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    // 某些测试会绕过高度图更新，回退到显式扫描。
    for (i32 y = 255; y >= 1; --y) {
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

} // namespace

// ============================================================================
// SeagrassFeature 实现
// ============================================================================

bool SeagrassFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const SeagrassFeatureConfig& config)
{
    if (!config.seagrassState) {
        return false;
    }

    // 参考原版 SeaGrassFeature：每次调用尝试多个随机偏移点。
    const i32 tries = std::max(1, config.tries);
    const i32 spread = std::max(1, config.horizontalSpread);

    bool placedAny = false;
    for (i32 attempt = 0; attempt < tries; ++attempt) {
        const i32 dx = random.nextInt(spread) - random.nextInt(spread);
        const i32 dz = random.nextInt(spread) - random.nextInt(spread);

        const i32 placeX = pos.x + dx;
        const i32 placeZ = pos.z + dz;
        const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
        if (oceanFloorY <= 0) {
            continue;
        }

        const BlockPos placePos(placeX, oceanFloorY + 1, placeZ);
        if (!canPlaceAt(world, placePos, *config.seagrassState)) {
            continue;
        }

        const bool shouldPlaceTall =
            config.tallSeagrassChance > 0.0f &&
            config.tallSeagrassLowerState != nullptr &&
            config.tallSeagrassUpperState != nullptr &&
            random.nextFloat() < config.tallSeagrassChance;

        if (shouldPlaceTall) {
            const BlockPos abovePos(placePos.x, placePos.y + 1, placePos.z);
            if (isWater(world, abovePos)) {
                placeTallSeagrass(world, placePos, config);
                placedAny = true;
                continue;
            }
        }

        world.setBlock(placePos, config.seagrassState);
        placedAny = true;
    }

    return placedAny;
}

bool SeagrassFeature::canPlaceAt(
    WorldGenRegion& world,
    const BlockPos& pos,
    const BlockState& seagrassState) const
{
    MC_UNUSED(seagrassState);

    if (!isWater(world, pos)) {
        return false;
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlock(belowPos);
    return belowState != nullptr && belowState->isSolid();
}

bool SeagrassFeature::isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlock(pos);
    if (!state) {
        return false;
    }

    // 检查是否为水方块
    if (VanillaBlocks::WATER && state->blockId() == VanillaBlocks::WATER->blockId()) {
        return true;
    }

    return false;
}

bool SeagrassFeature::placeTallSeagrass(
    WorldGenRegion& world,
    const BlockPos& pos,
    const SeagrassFeatureConfig& config) const
{
    // 放置下半部分
    world.setBlock(pos, config.tallSeagrassLowerState);

    // 放置上半部分
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    world.setBlock(abovePos, config.tallSeagrassUpperState);

    return true;
}

// ============================================================================
// ConfiguredSeagrassFeature 实现
// ============================================================================

ConfiguredSeagrassFeature::ConfiguredSeagrassFeature(
    std::unique_ptr<SeagrassFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredSeagrassFeature::place(
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

// ============================================================================
// SeagrassFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredSeagrassFeature>> SeagrassFeatures::s_features;

void SeagrassFeatures::initialize()
{
    s_features.clear();
    s_features.push_back(createSimpleSeagrass());
    s_features.push_back(createMixedSeagrass());
}

const std::vector<std::unique_ptr<ConfiguredSeagrassFeature>>& SeagrassFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredSeagrassFeature>> SeagrassFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createSimpleSeagrass()
{
    auto config = std::make_unique<SeagrassFeatureConfig>();
    if (VanillaBlocks::SEAGRASS != nullptr) {
        config->seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    }

    return std::make_unique<ConfiguredSeagrassFeature>(std::move(config), "seagrass_simple");
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createMixedSeagrass()
{
    auto config = std::make_unique<SeagrassFeatureConfig>();
    if (VanillaBlocks::SEAGRASS != nullptr) {
        config->seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    }
    if (VanillaBlocks::TALL_SEAGRASS != nullptr) {
        config->tallSeagrassLowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
            BlockStateProperties::HALF(),
            BlockStateProperties::DoubleBlockHalf::Lower);
        config->tallSeagrassUpperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
            BlockStateProperties::HALF(),
            BlockStateProperties::DoubleBlockHalf::Upper);
    }
    config->tallSeagrassChance = 0.3f;

    return std::make_unique<ConfiguredSeagrassFeature>(std::move(config), "seagrass_mixed");
}

} // namespace mc
