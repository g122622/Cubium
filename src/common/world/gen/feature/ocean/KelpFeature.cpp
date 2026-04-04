#include "KelpFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

namespace {

[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) {
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    // 某些测试场景会直接写方块而不更新高度图，回退到显式扫描。
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
// KelpFeature 实现
// ============================================================================

bool KelpFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const KelpFeatureConfig& config)
{
    if (!config.kelpState || !config.kelpTopState) {
        return false;
    }

    // 在当前区块内随机选择一个 X/Z，使用海底高度图定位生长起点。
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

    // 随机高度 (1-25)
    i32 height = random.nextInt(config.maxHeight) + 1;

    // 检查是否有足够空间（需要在水中）
    for (i32 y = 0; y < height; ++y) {
        BlockPos checkPos(placePos.x, placePos.y + y, placePos.z);
        const BlockState* state = world.getBlock(checkPos);

        // 必须在水或空气中
        if (state && !state->isAir()) {
            // 检查是否为水
            if (!isWater(world, checkPos)) {
                height = y;
                break;
            }
        }
    }

    if (height <= 0) {
        return false;
    }

    // 放置海带
    for (i32 y = 0; y < height; ++y) {
        BlockPos kelpPos(placePos.x, placePos.y + y, placePos.z);

        // 顶端使用 kelpTopState，其他使用 kelpState
        const BlockState* stateToPlace = (y == height - 1) ? config.kelpTopState : config.kelpState;
        world.setBlock(kelpPos, stateToPlace);
    }

    return true;
}

bool KelpFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
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

bool KelpFeature::isWater(WorldGenRegion& world, const BlockPos& pos) const
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

// ============================================================================
// ConfiguredKelpFeature 实现
// ============================================================================

ConfiguredKelpFeature::ConfiguredKelpFeature(
    std::unique_ptr<KelpFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredKelpFeature::place(
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
// KelpFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredKelpFeature>> KelpFeatures::s_features;

void KelpFeatures::initialize()
{
    s_features.clear();
    s_features.push_back(createNormalKelp());
}

const std::vector<std::unique_ptr<ConfiguredKelpFeature>>& KelpFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredKelpFeature>> KelpFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredKelpFeature> KelpFeatures::createNormalKelp()
{
    auto config = std::make_unique<KelpFeatureConfig>();
    if (VanillaBlocks::KELP_PLANT != nullptr && VanillaBlocks::KELP != nullptr) {
        // 主体使用 kelp_plant，顶端使用 kelp。
        config->kelpState = &VanillaBlocks::KELP_PLANT->defaultState();
        config->kelpTopState = &VanillaBlocks::KELP->defaultState();
    }
    config->maxHeight = 25;

    return std::make_unique<ConfiguredKelpFeature>(std::move(config), "kelp");
}

} // namespace mc
