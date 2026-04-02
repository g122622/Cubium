#include "KelpFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/Fluid.hpp"

namespace mc {

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

    // 寻找有效的放置位置（向下找到水下的地面）
    BlockPos placePos = pos;
    bool foundGround = false;

    for (i32 y = pos.y; y >= 1; --y) {
        BlockPos checkPos(pos.x, y, pos.z);
        const BlockState* state = world.getBlock(checkPos);

        if (state && !state->isAir()) {
            // 找到非空气方块
            placePos = BlockPos(pos.x, y + 1, pos.z);
            foundGround = true;
            break;
        }
    }

    if (!foundGround) {
        return false;
    }

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

    // TODO: 使用海带方块状态
    // 目前使用占位状态，需要等待海带方块注册后更新
    // config->kelpState = &VanillaBlocks::KELP->defaultState();
    // config->kelpTopState = &VanillaBlocks::KELP_PLANT->defaultState();
    config->maxHeight = 25;

    return std::make_unique<ConfiguredKelpFeature>(std::move(config), "kelp");
}

} // namespace mc
