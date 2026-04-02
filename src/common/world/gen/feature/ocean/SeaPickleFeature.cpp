#include "SeaPickleFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

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

        BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

        // 向下查找有效的放置位置
        for (i32 y = pos.y; y >= 1; --y) {
            BlockPos checkPos(placePos.x, y, placePos.z);
            const BlockState* state = world.getBlock(checkPos);

            if (state && !state->isAir()) {
                // 找到非空气方块，在其上方放置
                placePos = BlockPos(placePos.x, y + 1, placePos.z);
                break;
            }
        }

        // 检查是否可以放置
        if (canPlaceAt(world, placePos)) {
            // 随机数量 (1-4)
            i32 count = random.nextInt(config.maxCount) + 1;

            // TODO: 使用海泡菜的 PICKLES 属性设置数量
            // const BlockState* pickleState = config.seaPickleState->with(PICKLES, count);
            world.setBlock(placePos, config.seaPickleState);
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

    // TODO: 检查是否为活珊瑚方块
    // 珊瑚方块ID: TUBE_CORAL_BLOCK, BRAIN_CORAL_BLOCK, BUBBLE_CORAL_BLOCK, FIRE_CORAL_BLOCK, HORN_CORAL_BLOCK
    // 目前简化为检查是否为固体方块
    return state->owner().isSolid(*state);
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

    // TODO: 使用海泡菜方块状态
    // config->seaPickleState = &VanillaBlocks::SEA_PICKLE->defaultState();
    config->tries = 10;
    config->maxCount = 4;

    return std::make_unique<ConfiguredSeaPickleFeature>(std::move(config), "sea_pickle");
}

} // namespace mc
