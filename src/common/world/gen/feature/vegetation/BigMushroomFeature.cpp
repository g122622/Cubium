#include "BigMushroomFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../core/Constants.hpp"

namespace mc {

// ============================================================================
// BigMushroomFeature 实现
// ============================================================================

bool BigMushroomFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const BigMushroomFeatureConfig& config)
{
    // 计算蘑菇高度
    i32 height = calculateHeight(random);

    // 检查是否可以放置
    if (!canPlaceAt(world, pos, height, config)) {
        return false;
    }

    // 生成蘑菇柄
    generateStem(world, random, pos, config, height);

    // 生成蘑菇盖
    generateCap(world, random, pos, height, config);

    return true;
}

void BigMushroomFeature::generateStem(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const BigMushroomFeatureConfig& config,
    i32 height)
{
    (void)random;  // 暂不使用

    for (i32 y = 0; y < height; ++y) {
        BlockPos stemPos(pos.x, pos.y + y, pos.z);

        // 检查是否可以放置（可被原木替换）
        const BlockState* currentState = world.getBlockState(stemPos);
        if (currentState && !currentState->isAir()) {
            // 蘑菇柄可以替换树叶方块
            if (currentState->is(VanillaBlocks::OAK_LEAVES) ||
                currentState->is(VanillaBlocks::SPRUCE_LEAVES) ||
                currentState->is(VanillaBlocks::BIRCH_LEAVES) ||
                currentState->is(VanillaBlocks::JUNGLE_LEAVES) ||
                currentState->is(VanillaBlocks::ACACIA_LEAVES) ||
                currentState->is(VanillaBlocks::DARK_OAK_LEAVES)) {
                // 允许替换树叶
            } else {
                continue;
            }
        }

        if (config.stemState) {
            world.setBlockState(stemPos, config.stemState);
        }
    }
}

i32 BigMushroomFeature::calculateHeight(math::Random& random) const
{
    // 参考 MC: random.nextInt(3) + 4
    // 有 1/12 概率高度翻倍
    i32 height = random.nextInt(3) + 4;
    if (random.nextInt(12) == 0) {
        height *= 2;
    }
    return height;
}

bool BigMushroomFeature::canPlaceAt(
    WorldGenRegion& world,
    const BlockPos& pos,
    i32 height,
    const BigMushroomFeatureConfig& config) const
{
    (void)config;

    // 检查Y坐标范围
    i32 baseY = pos.y;
    if (baseY < world::MIN_BUILD_HEIGHT + 1 || baseY + height + 1 >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 检查下方方块是否为土壤
    const BlockState* belowState = world.getBlockState(BlockPos(pos.x, pos.y - 1, pos.z));
    if (!belowState) {
        return false;
    }

    u32 belowBlockId = belowState->blockId();
    bool isValidGround = belowBlockId == VanillaBlocks::GRASS_BLOCK->blockId() ||
                         belowBlockId == VanillaBlocks::DIRT->blockId() ||
                         belowBlockId == VanillaBlocks::MYCELIUM->blockId() ||
                         belowBlockId == VanillaBlocks::PODZOL->blockId() ||
                         belowBlockId == VanillaBlocks::COARSE_DIRT->blockId();

    if (!isValidGround) {
        return false;
    }

    // 检查蘑菇生长空间
    i32 capRadius = config.capRadius;
    for (i32 y = 0; y <= height; ++y) {
        i32 radius = getCapRadius(-1, height, capRadius, y);

        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                BlockPos checkPos(pos.x + dx, pos.y + y, pos.z + dz);
                const BlockState* state = world.getBlockState(checkPos);

                // 必须为空气或树叶
                if (state && !state->isAir()) {
                    // 允许替换树叶方块
                    if (state->is(VanillaBlocks::OAK_LEAVES) ||
                        state->is(VanillaBlocks::SPRUCE_LEAVES) ||
                        state->is(VanillaBlocks::BIRCH_LEAVES) ||
                        state->is(VanillaBlocks::JUNGLE_LEAVES) ||
                        state->is(VanillaBlocks::ACACIA_LEAVES) ||
                        state->is(VanillaBlocks::DARK_OAK_LEAVES)) {
                        continue;
                    }
                    return false;
                }
            }
        }
    }

    return true;
}

// ============================================================================
// BigBrownMushroomFeature 实现
// ============================================================================

i32 BigBrownMushroomFeature::getCapRadius(
    i32 baseRadius,
    i32 totalHeight,
    i32 capRadius,
    i32 currentHeight) const
{
    (void)baseRadius;
    (void)totalHeight;

    // 棕色蘑菇：只有顶部有盖
    // 参考 MC: height <= 3 ? 0 : capRadius
    return currentHeight <= 3 ? 0 : capRadius;
}

void BigBrownMushroomFeature::generateCap(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    i32 height,
    const BigMushroomFeatureConfig& config)
{
    i32 capRadius = config.capRadius;

    // 棕色蘑菇盖：平顶，略带边缘
    // 参考 MC BigBrownMushroomFeature.generateCap
    for (i32 dx = -capRadius; dx <= capRadius; ++dx) {
        for (i32 dz = -capRadius; dz <= capRadius; ++dz) {
            bool isEdgeX = (dx == -capRadius || dx == capRadius);
            bool isEdgeZ = (dz == -capRadius || dz == capRadius);

            // 跳过角落
            if (isEdgeX && isEdgeZ) {
                continue;
            }

            BlockPos capPos(pos.x + dx, pos.y + height, pos.z + dz);

            // 检查是否可以放置
            const BlockState* currentState = world.getBlockState(capPos);
            if (currentState && !currentState->isAir()) {
                continue;
            }

            // 计算边缘方向
            // 参考 MC 的 west/east/north/south 属性
            // 当前实现直接放置蘑菇盖状态
            if (config.capState) {
                world.setBlockState(capPos, config.capState);
            }
        }
    }
}

// ============================================================================
// BigRedMushroomFeature 实现
// ============================================================================

i32 BigRedMushroomFeature::getCapRadius(
    i32 baseRadius,
    i32 totalHeight,
    i32 capRadius,
    i32 currentHeight) const
{
    (void)baseRadius;

    // 红色蘑菇：多层蘑菇盖
    // 参考 MC: height < totalHeight - 3 ? 0 : (height == totalHeight ? capRadius : capRadius - 1)
    if (currentHeight < totalHeight - 3) {
        return 0;
    }
    if (currentHeight == totalHeight) {
        return capRadius;
    }
    return capRadius > 0 ? capRadius - 1 : 0;
}

void BigRedMushroomFeature::generateCap(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    i32 height,
    const BigMushroomFeatureConfig& config)
{
    i32 capRadius = config.capRadius;
    i32 innerRadius = capRadius - 2;

    // 红色蘑菇盖：多层圆顶形状
    // 参考 MC BigRedMushroomFeature.generateCap
    for (i32 y = height - 3; y <= height; ++y) {
        i32 currentRadius = (y < height) ? capRadius : (capRadius - 1);

        for (i32 dx = -currentRadius; dx <= currentRadius; ++dx) {
            for (i32 dz = -currentRadius; dz <= currentRadius; ++dz) {
                bool isEdgeX = (dx == -currentRadius || dx == currentRadius);
                bool isEdgeZ = (dz == -currentRadius || dz == currentRadius);
                bool isEdge = isEdgeX || isEdgeZ;

                // 顶层或非对角位置
                if (y >= height || isEdge != (isEdgeX && isEdgeZ)) {
                    BlockPos capPos(pos.x + dx, pos.y + y, pos.z + dz);

                    // 检查是否可以放置
                    const BlockState* currentState = world.getBlockState(capPos);
                    if (currentState && !currentState->isAir()) {
                        continue;
                    }

                    // 计算 up 属性（顶层为 true）
                    bool isTop = (y >= height - 1);

                    // 放置蘑菇盖
                    if (config.capState) {
                        world.setBlockState(capPos, config.capState);
                    }
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredBigMushroomFeature 实现
// ============================================================================

ConfiguredBigMushroomFeature::ConfiguredBigMushroomFeature(
    std::unique_ptr<BigMushroomFeatureConfig> config,
    const char* featureName,
    bool isBrown)
    : m_config(std::move(config))
    , m_name(featureName)
{
    if (isBrown) {
        m_feature = std::make_unique<BigBrownMushroomFeature>();
    } else {
        m_feature = std::make_unique<BigRedMushroomFeature>();
    }
}

bool ConfiguredBigMushroomFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature->place(region, random, pos, *m_config);
}

// ============================================================================
// BigMushroomFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>> BigMushroomFeatures::s_features;

void BigMushroomFeatures::initialize()
{
    s_features.clear();

    s_features.push_back(createBrownMushroom());
    s_features.push_back(createRedMushroom());
}

const std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>>& BigMushroomFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>> BigMushroomFeatures::getAllFeaturesAndClear()
{
    std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>> result;
    for (auto& feature : s_features) {
        result.push_back(std::move(feature));
    }
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredBigMushroomFeature> BigMushroomFeatures::createBrownMushroom()
{
    auto config = std::make_unique<BigMushroomFeatureConfig>();

    // 使用棕色蘑菇方块作为盖，蘑菇柄作为茎
    if (VanillaBlocks::BROWN_MUSHROOM_BLOCK) {
        config->capState = &VanillaBlocks::BROWN_MUSHROOM_BLOCK->defaultState();
    } else if (VanillaBlocks::BROWN_MUSHROOM) {
        config->capState = &VanillaBlocks::BROWN_MUSHROOM->defaultState();
    }
    if (VanillaBlocks::MUSHROOM_STEM) {
        config->stemState = &VanillaBlocks::MUSHROOM_STEM->defaultState();
    } else if (VanillaBlocks::BROWN_MUSHROOM) {
        config->stemState = &VanillaBlocks::BROWN_MUSHROOM->defaultState();
    }
    config->capRadius = 2;

    return std::make_unique<ConfiguredBigMushroomFeature>(
        std::move(config), "brown_mushroom", true);
}

std::unique_ptr<ConfiguredBigMushroomFeature> BigMushroomFeatures::createRedMushroom()
{
    auto config = std::make_unique<BigMushroomFeatureConfig>();

    // 使用红色蘑菇方块作为盖
    if (VanillaBlocks::RED_MUSHROOM_BLOCK) {
        config->capState = &VanillaBlocks::RED_MUSHROOM_BLOCK->defaultState();
    } else if (VanillaBlocks::RED_MUSHROOM) {
        config->capState = &VanillaBlocks::RED_MUSHROOM->defaultState();
    }
    if (VanillaBlocks::MUSHROOM_STEM) {
        config->stemState = &VanillaBlocks::MUSHROOM_STEM->defaultState();
    } else if (VanillaBlocks::RED_MUSHROOM) {
        config->stemState = &VanillaBlocks::RED_MUSHROOM->defaultState();
    }
    config->capRadius = 2;

    return std::make_unique<ConfiguredBigMushroomFeature>(
        std::move(config), "red_mushroom", false);
}

} // namespace mc
