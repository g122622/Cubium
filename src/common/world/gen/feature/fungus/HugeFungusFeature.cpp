#include "HugeFungusFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// HugeFungusFeature 实现
// ============================================================================

bool HugeFungusFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const HugeFungusFeatureConfig& config)
{
    // 检查是否可以放置
    if (!canPlaceAt(world, pos, config.fungusType)) {
        return false;
    }

    // 计算高度
    i32 height = config.minHeight + random.nextInt(config.maxHeight - config.minHeight + 1);

    // 生成菌柄
    generateStem(world, random, pos, height, config.fungusType);

    // 生成菌盖
    BlockPos topPos(pos.x, pos.y + height - 1, pos.z);
    generateCap(world, random, topPos, config.capRadius, config.fungusType);

    // 生成藤蔓
    generateVines(world, random, topPos, config.fungusType);

    // 生成菌光体
    generateShroomlights(world, random, topPos, config.capRadius);

    return true;
}

bool HugeFungusFeature::canPlaceAt(
    WorldGenRegion& world,
    const BlockPos& pos,
    FungusType type) const
{
    // 检查底部方块是否为菌岩
    const BlockState* groundState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (!groundState) {
        return false;
    }

    // 绯红真菌需要绯红菌岩，诡异真菌需要诡异菌岩
    // 由于菌岩方块尚未实现，暂时允许在下界岩上生成
    const Block& ground = groundState->getBlock();
    if (&ground != VanillaBlocks::NETHERRACK) {
        return false;
    }

    // 检查上方是否有足够空间
    for (i32 y = 0; y < 15; ++y) {
        const BlockState* state = world.getBlockState(pos.x, pos.y + y, pos.z);
        if (state && !state->isAir()) {
            return false;
        }
    }

    return true;
}

void HugeFungusFeature::generateStem(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    i32 height,
    FungusType type)
{
    // 根据类型选择菌柄方块
    const BlockState* stem = nullptr;
    if (type == FungusType::Crimson && VanillaBlocks::CRIMSON_STEM) {
        stem = VanillaBlocks::getState(VanillaBlocks::CRIMSON_STEM);
    } else if (type == FungusType::Warped && VanillaBlocks::WARPED_STEM) {
        stem = VanillaBlocks::getState(VanillaBlocks::WARPED_STEM);
    }
    // 回退到下界岩
    if (!stem) {
        stem = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);
    }
    if (!stem) {
        return;
    }

    // 生成菌柄
    for (i32 y = 0; y < height; ++y) {
        world.setBlockState(pos.x, pos.y + y, pos.z, stem);
    }
}

void HugeFungusFeature::generateCap(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& topPos,
    i32 radius,
    FungusType type)
{
    // 根据类型选择菌盖方块
    const BlockState* cap = nullptr;
    if (type == FungusType::Crimson && VanillaBlocks::NETHER_WART_BLOCK) {
        cap = VanillaBlocks::getState(VanillaBlocks::NETHER_WART_BLOCK);
    } else if (type == FungusType::Warped && VanillaBlocks::WARPED_NYLIUM) {
        // 诡异菌盖使用诡异疣块（暂用诡异菌岩代替）
        cap = VanillaBlocks::getState(VanillaBlocks::WARPED_NYLIUM);
    }
    // 回退到萤石
    if (!cap) {
        cap = VanillaBlocks::getState(VanillaBlocks::GLOWSTONE);
    }
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    if (!cap) {
        return;
    }

    // 生成菌盖（球形或扁平球形）

    // 菌盖层数
    i32 layers = 2 + random.nextInt(2);

    for (i32 layer = 0; layer < layers; ++layer) {
        i32 layerY = topPos.y + layer;
        i32 layerRadius = radius - (layer / 2);

        for (i32 x = -layerRadius; x <= layerRadius; ++x) {
            for (i32 z = -layerRadius; z <= layerRadius; ++z) {
                i32 distSq = x * x + z * z;
                if (distSq <= layerRadius * layerRadius) {
                    // 随机跳过一些边缘方块
                    if (distSq >= (layerRadius - 1) * (layerRadius - 1) && random.nextInt(3) == 0) {
                        continue;
                    }
                    world.setBlockState(topPos.x + x, layerY, topPos.z + z, cap);
                }
            }
        }
    }
}

void HugeFungusFeature::generateVines(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& capPos,
    FungusType type)
{
    // 根据类型选择藤蔓方块
    const BlockState* vine = nullptr;
    if (type == FungusType::Crimson && VanillaBlocks::WEEPING_VINES) {
        vine = VanillaBlocks::getState(VanillaBlocks::WEEPING_VINES);
    } else if (type == FungusType::Warped && VanillaBlocks::TWISTING_VINES) {
        vine = VanillaBlocks::getState(VanillaBlocks::TWISTING_VINES);
    }

    if (!vine) {
        return;
    }

    // 在菌盖下方生成藤蔓
    i32 vineCount = 3 + random.nextInt(4);
    for (i32 i = 0; i < vineCount; ++i) {
        i32 offsetX = -2 + random.nextInt(5);
        i32 offsetZ = -2 + random.nextInt(5);
        i32 vineLength = 1 + random.nextInt(4);

        for (i32 y = 0; y < vineLength; ++y) {
            BlockPos vinePos(capPos.x + offsetX, capPos.y - y - 1, capPos.z + offsetZ);
            const BlockState* existing = world.getBlockState(vinePos);
            if (existing && existing->isAir()) {
                world.setBlockState(vinePos, vine);
            } else {
                break;
            }
        }
    }
}

void HugeFungusFeature::generateShroomlights(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& capPos,
    i32 radius)
{
    // 使用菌光体方块，回退到萤石
    const BlockState* shroomlight = nullptr;
    if (VanillaBlocks::SHROOMLIGHT) {
        shroomlight = VanillaBlocks::getState(VanillaBlocks::SHROOMLIGHT);
    }
    if (!shroomlight) {
        shroomlight = VanillaBlocks::getState(VanillaBlocks::GLOWSTONE);
    }

    if (!shroomlight) {
        return;
    }

    // 在菌盖内部随机放置菌光体
    i32 shroomlightCount = 1 + random.nextInt(3);

    for (i32 i = 0; i < shroomlightCount; ++i) {
        i32 offsetX = -radius + random.nextInt(2 * radius + 1);
        i32 offsetZ = -radius + random.nextInt(2 * radius + 1);
        i32 offsetY = random.nextInt(2);

        // 只替换菌盖方块（非空气方块）
        const BlockState* existing = world.getBlockState(capPos.x + offsetX, capPos.y + offsetY, capPos.z + offsetZ);
        if (existing && !existing->isAir() &&
            &existing->getBlock() != VanillaBlocks::AIR) {
            world.setBlockState(capPos.x + offsetX, capPos.y + offsetY, capPos.z + offsetZ, shroomlight);
        }
    }
}

// ============================================================================
// ConfiguredHugeFungusFeature 实现
// ============================================================================

ConfiguredHugeFungusFeature::ConfiguredHugeFungusFeature(
    std::unique_ptr<HugeFungusFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredHugeFungusFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// HugeFungusFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredHugeFungusFeature>> HugeFungusFeatures::s_features;

void HugeFungusFeatures::initialize() {
    if (!s_features.empty()) return;

    // 创建绯红巨型真菌
    s_features.push_back(createCrimson());

    // 创建诡异巨型真菌
    s_features.push_back(createWarped());
}

const std::vector<std::unique_ptr<ConfiguredHugeFungusFeature>>& HugeFungusFeatures::getAllFeatures() {
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredHugeFungusFeature>> HugeFungusFeatures::getAllFeaturesAndClear() {
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredHugeFungusFeature> HugeFungusFeatures::createCrimson() {
    auto config = std::make_unique<HugeFungusFeatureConfig>(
        FungusType::Crimson,  // 绯红类型
        4,                     // 最小高度
        13,                    // 最大高度
        2,                     // 菌盖半径
        false                  // 非种植
    );
    return std::make_unique<ConfiguredHugeFungusFeature>(std::move(config), "crimson_fungus");
}

std::unique_ptr<ConfiguredHugeFungusFeature> HugeFungusFeatures::createWarped() {
    auto config = std::make_unique<HugeFungusFeatureConfig>(
        FungusType::Warped,   // 诡异类型
        4,                     // 最小高度
        13,                    // 最大高度
        2,                     // 菌盖半径
        false                  // 非种植
    );
    return std::make_unique<ConfiguredHugeFungusFeature>(std::move(config), "warped_fungus");
}

} // namespace mc
