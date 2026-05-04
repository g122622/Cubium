#include "SugarCaneFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

// ============================================================================
// SugarCaneFeature 实现
// ============================================================================

bool SugarCaneFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const SugarCaneFeatureConfig& config)
{
    if (!config.state) {
        return false;
    }

    i32 placedCount = 0;

    for (i32 i = 0; i < config.tries; ++i) {
        // 随机偏移
        i32 dx = random.nextInt(config.xzSpread) - random.nextInt(config.xzSpread);
        i32 dz = random.nextInt(config.xzSpread) - random.nextInt(config.xzSpread);

        BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

        // 向下寻找地面
        for (i32 y = 128; y >= 1; --y) {
            BlockPos checkPos(placePos.x, y, placePos.z);
            const BlockState* state = world.getBlockState(checkPos);

            if (state && !state->isAir()) {
                placePos = BlockPos(placePos.x, y + 1, placePos.z);
                break;
            }
        }

        // 检查是否可以放置
        if (!canPlaceAt(world, placePos)) {
            continue;
        }

        // 随机高度（1-3）
        i32 height = random.nextInt(config.maxHeight) + 1;

        // 检查是否有足够空间
        bool hasSpace = true;
        for (i32 y = 0; y < height; ++y) {
            BlockPos checkPos(placePos.x, placePos.y + y, placePos.z);
            const BlockState* state = world.getBlockState(checkPos);
            if (state && !state->isAir()) {
                height = y;
                break;
            }
        }

        if (height <= 0 || !hasSpace) {
            continue;
        }

        // 放置甘蔗
        for (i32 y = 0; y < height; ++y) {
            BlockPos canePos(placePos.x, placePos.y + y, placePos.z);
            world.setBlockState(canePos, config.state);
        }

        ++placedCount;
    }

    return placedCount > 0;
}

bool SugarCaneFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否为空气
    const BlockState* state = world.getBlockState(pos);
    if (state && !state->isAir()) {
        return false;
    }

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    if (!isValidGround(world, belowPos)) {
        return false;
    }

    // 检查周围是否有水
    return hasWaterNearby(world, belowPos);
}

bool SugarCaneFeature::hasWaterNearby(WorldGenRegion& world, const BlockPos& pos) const
{
    // 甘蔗需要周围有水（4个方向相邻）
    static const i32 directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (i32 i = 0; i < 4; ++i) {
        BlockPos waterPos(pos.x + directions[i][0], pos.y, pos.z + directions[i][1]);
        const BlockState* state = world.getBlockState(waterPos);

        if (state && state->blockId() == VanillaBlocks::WATER->blockId()) {
            return true;
        }
    }

    return false;
}

bool SugarCaneFeature::isValidGround(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    u32 blockId = state->blockId();

    // 甘蔗可以生长在草地、泥土、沙子、红沙、甘蔗上
    return blockId == VanillaBlocks::GRASS_BLOCK->blockId() ||
           blockId == VanillaBlocks::DIRT->blockId() ||
           blockId == VanillaBlocks::SAND->blockId() ||
           blockId == VanillaBlocks::PODZOL->blockId() ||
           blockId == VanillaBlocks::MYCELIUM->blockId() ||
           (VanillaBlocks::SUGAR_CANE && blockId == VanillaBlocks::SUGAR_CANE->blockId());
}

// ============================================================================
// ConfiguredSugarCaneFeature 实现
// ============================================================================

ConfiguredSugarCaneFeature::ConfiguredSugarCaneFeature(
    std::unique_ptr<SugarCaneFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredSugarCaneFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// SugarCaneFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredSugarCaneFeature>> SugarCaneFeatures::s_features;

void SugarCaneFeatures::initialize()
{
    s_features.clear();

    s_features.push_back(createNormal());
    s_features.push_back(createDense());
}

const std::vector<std::unique_ptr<ConfiguredSugarCaneFeature>>& SugarCaneFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredSugarCaneFeature>> SugarCaneFeatures::getAllFeaturesAndClear()
{
    std::vector<std::unique_ptr<ConfiguredSugarCaneFeature>> result;
    for (auto& feature : s_features) {
        result.push_back(std::move(feature));
    }
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredSugarCaneFeature> SugarCaneFeatures::createNormal()
{
    auto config = std::make_unique<SugarCaneFeatureConfig>();

    if (VanillaBlocks::SUGAR_CANE) {
        config->state = &VanillaBlocks::SUGAR_CANE->defaultState();
    }
    config->maxHeight = 3;
    config->tries = 20;
    config->xzSpread = 8;

    return std::make_unique<ConfiguredSugarCaneFeature>(std::move(config), "sugar_cane");
}

std::unique_ptr<ConfiguredSugarCaneFeature> SugarCaneFeatures::createDense()
{
    auto config = std::make_unique<SugarCaneFeatureConfig>();

    if (VanillaBlocks::SUGAR_CANE) {
        config->state = &VanillaBlocks::SUGAR_CANE->defaultState();
    }
    config->maxHeight = 4;
    config->tries = 40;
    config->xzSpread = 10;

    return std::make_unique<ConfiguredSugarCaneFeature>(std::move(config), "sugar_cane_dense");
}

} // namespace mc
