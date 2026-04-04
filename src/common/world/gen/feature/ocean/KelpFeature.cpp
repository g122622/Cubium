#include "KelpFeature.hpp"
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

    // 参考原版 KelpFeature：单次放置会尝试多次随机点，并在顶端设置 AGE 20-23。
    const i32 tries = std::max(1, config.tries);
    const i32 maxHeight = std::max(1, config.maxHeight);

    bool placedAny = false;
    for (i32 attempt = 0; attempt < tries; ++attempt) {
        const i32 placeX = pos.x + random.nextInt(16);
        const i32 placeZ = pos.z + random.nextInt(16);
        const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
        if (oceanFloorY <= 0) {
            continue;
        }

        BlockPos currentPos(placeX, oceanFloorY + 1, placeZ);
        if (!isWater(world, currentPos)) {
            continue;
        }

        const i32 height = 1 + random.nextInt(maxHeight);
        for (i32 y = 0; y <= height; ++y) {
            const BlockPos abovePos(currentPos.x, currentPos.y + 1, currentPos.z);
            const bool canGrowHere = isWater(world, currentPos) && isWater(world, abovePos) && canPlaceAt(world, currentPos);

            if (canGrowHere) {
                if (y == height) {
                    const i32 age = random.nextInt(4) + 20;
                    world.setBlock(
                        currentPos,
                        &config.kelpTopState->with(BlockStateProperties::AGE_0_25(), age));
                    placedAny = true;
                } else {
                    world.setBlock(currentPos, config.kelpState);
                }
            } else if (y > 0) {
                const BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
                const BlockPos belowBelowPos(belowPos.x, belowPos.y - 1, belowPos.z);
                const BlockState* belowBelowState = world.getBlock(belowBelowPos);

                const bool belowHasKelp =
                    (VanillaBlocks::KELP != nullptr && belowBelowState != nullptr && belowBelowState->is(VanillaBlocks::KELP));

                if (canPlaceAt(world, belowPos) && !belowHasKelp) {
                    const i32 age = random.nextInt(4) + 20;
                    world.setBlock(
                        belowPos,
                        &config.kelpTopState->with(BlockStateProperties::AGE_0_25(), age));
                    placedAny = true;
                }
                break;
            }

            currentPos = BlockPos(currentPos.x, currentPos.y + 1, currentPos.z);
        }
    }

    return placedAny;
}

bool KelpFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlock(belowPos);

    if (!belowState) {
        return false;
    }

    if (VanillaBlocks::KELP != nullptr && belowState->is(VanillaBlocks::KELP)) {
        return true;
    }
    if (VanillaBlocks::KELP_PLANT != nullptr && belowState->is(VanillaBlocks::KELP_PLANT)) {
        return true;
    }

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
    config->tries = 80;
    config->maxHeight = 10;

    return std::make_unique<ConfiguredKelpFeature>(std::move(config), "kelp");
}

} // namespace mc
