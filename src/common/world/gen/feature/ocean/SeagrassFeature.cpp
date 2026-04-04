#include "SeagrassFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"

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

    // 在当前区块内随机选择一个 X/Z，使用海底高度图定位放置起点。
    const i32 placeX = pos.x + random.nextInt(16);
    const i32 placeZ = pos.z + random.nextInt(16);
    const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
    if (oceanFloorY <= 0) {
        return false;
    }

    const BlockPos placePos(placeX, oceanFloorY + 1, placeZ);

    // 检查是否可以放置
    if (!canPlaceAt(world, placePos)) {
        return false;
    }

    // 决定是否放置高海草
    if (config.tallSeagrassChance > 0.0f &&
        config.tallSeagrassLowerState &&
        config.tallSeagrassUpperState &&
        random.nextFloat() < config.tallSeagrassChance)
    {
        // 检查上方是否有空间放置高海草
        BlockPos abovePos(placePos.x, placePos.y + 1, placePos.z);
        if (isWater(world, abovePos)) {
            return placeTallSeagrass(world, placePos, config);
        }
    }

    // 放置普通海草
    world.setBlock(placePos, config.seagrassState);
    return true;
}

bool SeagrassFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否为水
    if (!isWater(world, pos)) {
        return false;
    }

    // 检查下方方块是否为固体
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlock(belowPos);

    if (!belowState) {
        return false;
    }

    // 下方必须是固体方块
    return belowState->owner().isSolid(*belowState);
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
