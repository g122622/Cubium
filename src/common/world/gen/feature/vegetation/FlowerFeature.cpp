#include "FlowerFeature.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// FlowerFeatureConfig 实现
// ============================================================================

const BlockState* FlowerFeatureConfig::getRandomFlower(math::Random& random) const
{
    if (flowers.empty()) {
        return nullptr;
    }
    return flowers[random.nextInt(static_cast<i32>(flowers.size()))];
}

// ============================================================================
// FlowerFeature 实现
// ============================================================================

bool FlowerFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const FlowerFeatureConfig& config)
{
    if (config.flowers.empty()) {
        return false;
    }

    i32 placedCount = 0;
    i32 xzSpread = config.xzSpread;

    for (i32 i = 0; i < config.tries; ++i) {
        // 在指定范围内随机选择位置
        i32 dx = random.nextInt(xzSpread) - random.nextInt(xzSpread);
        i32 dz = random.nextInt(xzSpread) - random.nextInt(xzSpread);

        BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

        // 寻找有效的Y坐标
        for (i32 y = 128; y >= 1; --y) {
            BlockPos checkPos(placePos.x, y, placePos.z);
            const BlockState* state = world.getBlockState(checkPos);

            if (!state || state->isAir()) {
                placePos = checkPos;
                break;
            }
        }

        // 检查是否可以放置
        if (canPlaceAt(world, placePos, config)) {
            const BlockState* flower = config.getRandomFlower(random);
            if (flower) {
                world.setBlockState(placePos, flower);
                ++placedCount;
            }
        }
    }

    return placedCount > 0;
}

bool FlowerFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const FlowerFeatureConfig& config) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state && !state->isAir()) {
        return false;
    }

    // 检查下方方块
    if (!isValidGround(world, BlockPos(pos.x, pos.y - 1, pos.z))) {
        return false;
    }

    // 检查是否需要水
    if (config.requiresWater) {
        // 检查周围是否有水
        bool hasWater = false;
        for (i32 dx = -4; dx <= 4; ++dx) {
            for (i32 dz = -4; dz <= 4; ++dz) {
                BlockPos waterPos(pos.x + dx, pos.y - 1, pos.z + dz);
                const BlockState* waterState = world.getBlockState(waterPos);
                if (waterState && waterState->blockId() == VanillaBlocks::WATER->blockId()) {
                    hasWater = true;
                    break;
                }
            }
            if (hasWater) break;
        }
        if (!hasWater) return false;
    }

    return true;
}

bool FlowerFeature::isValidGround(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) return false;

    u32 blockId = state->blockId();

    // 花卉可以生长在草方块和泥土上
    return blockId == VanillaBlocks::GRASS_BLOCK->blockId() || blockId == VanillaBlocks::DIRT->blockId() ||
        (VanillaBlocks::FARMLAND && blockId == VanillaBlocks::FARMLAND->blockId());
}

// ============================================================================
// ConfiguredFlowerFeature 实现
// ============================================================================

ConfiguredFlowerFeature::ConfiguredFlowerFeature(std::unique_ptr<FlowerFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredFlowerFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// FlowerFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredFlowerFeature>> FlowerFeatures::s_features;

void FlowerFeatures::initialize()
{
    s_features.clear();

    // 添加各种花卉配置
    s_features.push_back(createPlainsFlowers());
    s_features.push_back(createForestFlowers());
    s_features.push_back(createFlowerForestFlowers());
    s_features.push_back(createSwampFlowers());
    s_features.push_back(createSunflower());
}

const std::vector<std::unique_ptr<ConfiguredFlowerFeature>>& FlowerFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredFlowerFeature>> FlowerFeatures::getAllFeaturesAndClear()
{
    std::vector<std::unique_ptr<ConfiguredFlowerFeature>> result;
    for (auto& feature : s_features) {
        result.push_back(std::move(feature));
    }
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredFlowerFeature> FlowerFeatures::createPlainsFlowers()
{
    auto config = std::make_unique<FlowerFeatureConfig>();
    config->tries = 64;
    config->xzSpread = 7;

    // 平原花卉：蒲公英、虞美人
    if (VanillaBlocks::DANDELION) {
        config->addFlower(&VanillaBlocks::DANDELION->defaultState());
    }
    if (VanillaBlocks::POPPY) {
        config->addFlower(&VanillaBlocks::POPPY->defaultState());
    }

    return std::make_unique<ConfiguredFlowerFeature>(std::move(config), "plains_flowers");
}

std::unique_ptr<ConfiguredFlowerFeature> FlowerFeatures::createForestFlowers()
{
    auto config = std::make_unique<FlowerFeatureConfig>();
    config->tries = 64;
    config->xzSpread = 7;

    // 森林花卉：蒲公英、虞美人
    if (VanillaBlocks::DANDELION) {
        config->addFlower(&VanillaBlocks::DANDELION->defaultState());
    }
    if (VanillaBlocks::POPPY) {
        config->addFlower(&VanillaBlocks::POPPY->defaultState());
    }
    if (VanillaBlocks::LILY_OF_THE_VALLEY) {
        config->addFlower(&VanillaBlocks::LILY_OF_THE_VALLEY->defaultState());
    }

    return std::make_unique<ConfiguredFlowerFeature>(std::move(config), "forest_flowers");
}

std::unique_ptr<ConfiguredFlowerFeature> FlowerFeatures::createFlowerForestFlowers()
{
    auto config = std::make_unique<FlowerFeatureConfig>();
    config->tries = 128; // 繁花森林有更多花卉
    config->xzSpread = 7;

    // 繁花森林：所有花卉
    if (VanillaBlocks::DANDELION) {
        config->addFlower(&VanillaBlocks::DANDELION->defaultState());
    }
    if (VanillaBlocks::POPPY) {
        config->addFlower(&VanillaBlocks::POPPY->defaultState());
    }
    if (VanillaBlocks::LILY_OF_THE_VALLEY) {
        config->addFlower(&VanillaBlocks::LILY_OF_THE_VALLEY->defaultState());
    }
    if (VanillaBlocks::CORNFLOWER) {
        config->addFlower(&VanillaBlocks::CORNFLOWER->defaultState());
    }
    if (VanillaBlocks::ALLIUM) {
        config->addFlower(&VanillaBlocks::ALLIUM->defaultState());
    }
    if (VanillaBlocks::AZURE_BLUET) {
        config->addFlower(&VanillaBlocks::AZURE_BLUET->defaultState());
    }
    if (VanillaBlocks::RED_TULIP) {
        config->addFlower(&VanillaBlocks::RED_TULIP->defaultState());
    }
    if (VanillaBlocks::ORANGE_TULIP) {
        config->addFlower(&VanillaBlocks::ORANGE_TULIP->defaultState());
    }
    if (VanillaBlocks::WHITE_TULIP) {
        config->addFlower(&VanillaBlocks::WHITE_TULIP->defaultState());
    }
    if (VanillaBlocks::PINK_TULIP) {
        config->addFlower(&VanillaBlocks::PINK_TULIP->defaultState());
    }
    if (VanillaBlocks::OXEYE_DAISY) {
        config->addFlower(&VanillaBlocks::OXEYE_DAISY->defaultState());
    }

    return std::make_unique<ConfiguredFlowerFeature>(std::move(config), "flower_forest_flowers");
}

std::unique_ptr<ConfiguredFlowerFeature> FlowerFeatures::createSwampFlowers()
{
    auto config = std::make_unique<FlowerFeatureConfig>();
    config->tries = 64;
    config->xzSpread = 7;

    // 沼泽：兰花
    if (VanillaBlocks::BLUE_ORCHID) {
        config->addFlower(&VanillaBlocks::BLUE_ORCHID->defaultState());
    }

    return std::make_unique<ConfiguredFlowerFeature>(std::move(config), "swamp_flowers");
}

std::unique_ptr<ConfiguredFlowerFeature> FlowerFeatures::createSunflower()
{
    auto config = std::make_unique<FlowerFeatureConfig>();
    config->tries = 32;
    config->xzSpread = 8;

    // 向日葵平原：向日葵
    if (VanillaBlocks::SUNFLOWER) {
        config->addFlower(&VanillaBlocks::SUNFLOWER->defaultState());
    }
    // 也添加蒲公英作为补充
    if (VanillaBlocks::DANDELION) {
        config->addFlower(&VanillaBlocks::DANDELION->defaultState());
    }

    return std::make_unique<ConfiguredFlowerFeature>(std::move(config), "sunflower");
}

} // namespace mc
