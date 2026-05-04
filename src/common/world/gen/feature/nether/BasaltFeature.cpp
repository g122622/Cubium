#include "BasaltFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// BasaltColumnFeature 实现
// ============================================================================

bool BasaltColumnFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const BasaltColumnFeatureConfig& config)
{
    // 检查起始位置是否在地面上
    if (!canPlaceAt(world, pos)) {
        return false;
    }

    // 计算柱高度
    i32 height = getColumnHeight(world, pos, config.minHeight, config.maxHeight);
    if (height <= 0) {
        return false;
    }

    // 获取玄武岩方块
    const BlockState* basalt = VanillaBlocks::getState(VanillaBlocks::BASALT);
    if (!basalt) {
        // 回退到下界岩
        basalt = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);
        if (!basalt) {
            return false;
        }
    }

    // 生成玄武岩柱
    for (i32 y = 0; y < height; ++y) {
        BlockPos columnPos(pos.x, pos.y + y, pos.z);
        world.setBlockState(columnPos, basalt);
    }

    return true;
}

bool BasaltColumnFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查当前方块是否为可替换方块
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        // 检查下方是否有支撑
        const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
        return belowState && !belowState->isAir();
    }
    return false;
}

i32 BasaltColumnFeature::getColumnHeight(
    WorldGenRegion& world,
    const BlockPos& pos,
    i32 minH, i32 maxH) const
{
    // 计算到天花板的距离
    i32 spaceAbove = 0;
    for (i32 y = pos.y; y < 128; ++y) {
        const BlockState* state = world.getBlockState(pos.x, y, pos.z);
        if (!state || state->isAir()) {
            ++spaceAbove;
        } else {
            break;
        }
    }

    // 限制在配置范围内
    return std::min(maxH, std::max(minH, spaceAbove - 1));
}

// ============================================================================
// ConfiguredBasaltColumnFeature 实现
// ============================================================================

ConfiguredBasaltColumnFeature::ConfiguredBasaltColumnFeature(
    std::unique_ptr<BasaltColumnFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredBasaltColumnFeature::place(
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
// BasaltColumnFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>> BasaltColumnFeatures::s_features;

void BasaltColumnFeatures::initialize() {
    if (!s_features.empty()) return;

    s_features.push_back(createNormal());
    s_features.push_back(createLarge());
}

const std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>>& BasaltColumnFeatures::getAllFeatures() {
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>> BasaltColumnFeatures::getAllFeaturesAndClear() {
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredBasaltColumnFeature> BasaltColumnFeatures::createNormal() {
    auto config = std::make_unique<BasaltColumnFeatureConfig>(
        0,   // minHeight
        5,   // maxHeight
        false
    );
    return std::make_unique<ConfiguredBasaltColumnFeature>(std::move(config), "basalt_column");
}

std::unique_ptr<ConfiguredBasaltColumnFeature> BasaltColumnFeatures::createLarge() {
    auto config = std::make_unique<BasaltColumnFeatureConfig>(
        3,   // minHeight
        10,  // maxHeight
        true
    );
    return std::make_unique<ConfiguredBasaltColumnFeature>(std::move(config), "basalt_column_large");
}

// ============================================================================
// BasaltDeltaFeature 实现
// ============================================================================

bool BasaltDeltaFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const BasaltDeltaFeatureConfig& config)
{
    // 获取方块状态
    const BlockState* basalt = VanillaBlocks::getState(VanillaBlocks::BASALT);
    const BlockState* magma = VanillaBlocks::getState(VanillaBlocks::MAGMA);
    const BlockState* netherrack = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);

    if (!basalt || !netherrack) {
        return false;
    }

    // 在区域内生成玄武岩地面
    i32 halfSize = config.size / 2;

    for (i32 dx = -halfSize; dx <= halfSize; ++dx) {
        for (i32 dz = -halfSize; dz <= halfSize; ++dz) {
            // 使用圆形掩码
            f32 distSq = static_cast<f32>(dx * dx + dz * dz);
            f32 radiusSq = static_cast<f32>(halfSize * halfSize);
            if (distSq > radiusSq) {
                continue;
            }

            // 边缘渐变
            f32 edgeFactor = 1.0f - (distSq / radiusSq);
            if (random.nextFloat() > edgeFactor) {
                continue;
            }

            BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

            // 检查当前位置
            const BlockState* currentState = world.getBlockState(placePos);
            if (!currentState || !currentState->is(VanillaBlocks::NETHERRACK)) {
                continue;
            }

            // 决定放置什么方块
            const BlockState* toPlace = basalt;
            if (magma && random.nextFloat() < config.magmaChance) {
                toPlace = magma;
            }

            world.setBlockState(placePos, toPlace);

            // 有时候向下替换一层
            if (random.nextFloat() < 0.3f) {
                BlockPos belowPos(placePos.x, placePos.y - 1, placePos.z);
                const BlockState* belowState = world.getBlockState(belowPos);
                if (belowState && belowState->is(VanillaBlocks::NETHERRACK)) {
                    world.setBlockState(belowPos, toPlace);
                }
            }
        }
    }

    return true;
}

// ============================================================================
// ConfiguredBasaltDeltaFeature 实现
// ============================================================================

ConfiguredBasaltDeltaFeature::ConfiguredBasaltDeltaFeature(
    std::unique_ptr<BasaltDeltaFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredBasaltDeltaFeature::place(
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
// BasaltDeltaFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> BasaltDeltaFeatures::s_features;

void BasaltDeltaFeatures::initialize() {
    if (!s_features.empty()) return;
    s_features.push_back(createNormal());
}

const std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>>& BasaltDeltaFeatures::getAllFeatures() {
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> BasaltDeltaFeatures::getAllFeaturesAndClear() {
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredBasaltDeltaFeature> BasaltDeltaFeatures::createNormal() {
    auto config = std::make_unique<BasaltDeltaFeatureConfig>(
        8,    // size
        0.2f, // magmaChance
        true  // useBasalt
    );
    return std::make_unique<ConfiguredBasaltDeltaFeature>(std::move(config), "basalt_delta");
}

} // namespace mc
