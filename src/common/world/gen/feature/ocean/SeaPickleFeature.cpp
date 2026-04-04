#include "SeaPickleFeature.hpp"
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

    // 测试场景中高度图可能未初始化，回退到显式扫描。
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
// SeaPickleFeature 实现
// ============================================================================

bool SeaPickleFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const SeaPickleFeatureConfig& config)
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

        // 检查是否可以放置
        if (canPlaceAt(world, placePos)) {
            // 随机数量 (1-4)
            const i32 maxCount = std::clamp(config.maxCount, 1, 4);
            const i32 count = random.nextInt(maxCount) + 1;

            const BlockState* pickleState = config.seaPickleState;
            if (VanillaBlocks::SEA_PICKLE != nullptr && config.seaPickleState->is(VanillaBlocks::SEA_PICKLE)) {
                pickleState = &config.seaPickleState->with(BlockStateProperties::PICKLES_1_4(), count);
            }

            world.setBlock(placePos, pickleState);
            placedAny = true;
        }
    }

    return placedAny;
}

bool SeaPickleFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否为水
    if (!isWater(world, pos)) {
        return false;
    }

    // 检查下方是否为活珊瑚
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    return isLivingCoral(world, belowPos);
}

bool SeaPickleFeature::isWater(WorldGenRegion& world, const BlockPos& pos) const
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

bool SeaPickleFeature::isLivingCoral(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlock(pos);
    if (!state) {
        return false;
    }

    // 仅允许放置在活珊瑚块上，避免海泡菜扩散到普通海底方块。
    return (VanillaBlocks::TUBE_CORAL_BLOCK && state->is(VanillaBlocks::TUBE_CORAL_BLOCK)) ||
           (VanillaBlocks::BRAIN_CORAL_BLOCK && state->is(VanillaBlocks::BRAIN_CORAL_BLOCK)) ||
           (VanillaBlocks::BUBBLE_CORAL_BLOCK && state->is(VanillaBlocks::BUBBLE_CORAL_BLOCK)) ||
           (VanillaBlocks::FIRE_CORAL_BLOCK && state->is(VanillaBlocks::FIRE_CORAL_BLOCK)) ||
           (VanillaBlocks::HORN_CORAL_BLOCK && state->is(VanillaBlocks::HORN_CORAL_BLOCK));
}

// ============================================================================
// ConfiguredSeaPickleFeature 实现
// ============================================================================

ConfiguredSeaPickleFeature::ConfiguredSeaPickleFeature(
    std::unique_ptr<SeaPickleFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredSeaPickleFeature::place(
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
// SeaPickleFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredSeaPickleFeature>> SeaPickleFeatures::s_features;

void SeaPickleFeatures::initialize()
{
    s_features.clear();
    s_features.push_back(createNormalSeaPickle());
}

const std::vector<std::unique_ptr<ConfiguredSeaPickleFeature>>& SeaPickleFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredSeaPickleFeature>> SeaPickleFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredSeaPickleFeature> SeaPickleFeatures::createNormalSeaPickle()
{
    auto config = std::make_unique<SeaPickleFeatureConfig>();
    if (VanillaBlocks::SEA_PICKLE != nullptr) {
        config->seaPickleState = &VanillaBlocks::SEA_PICKLE->defaultState();
    }
    config->tries = 10;
    config->maxCount = 4;

    return std::make_unique<ConfiguredSeaPickleFeature>(std::move(config), "sea_pickle");
}

} // namespace mc
