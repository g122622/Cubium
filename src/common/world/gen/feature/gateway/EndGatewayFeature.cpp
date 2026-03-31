#include "EndGatewayFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// EndGatewayFeature 实现
// ============================================================================

bool EndGatewayFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const EndGatewayFeatureConfig& config)
{
    // 检查是否可以放置
    if (!canPlaceAt(world, pos)) {
        return false;
    }

    // 生成折跃门结构
    generateGateway(world, random, pos);

    return true;
}

BlockPos EndGatewayFeature::calculateTeleportTarget(const BlockPos& currentPos, u64 seed) {
    // 参考 MC 1.16.5: EndGatewayBlock
    // 传送到1024格外的外岛
    math::Random rng(seed);

    // 随机角度
    f64 angle = rng.nextDouble() * 2.0 * 3.14159265358979323846;

    // 距离范围：1024-1280格
    f64 distance = 1024.0 + rng.nextDouble() * 256.0;

    // 计算目标位置
    i32 targetX = static_cast<i32>(std::cos(angle) * distance);
    i32 targetZ = static_cast<i32>(std::sin(angle) * distance);

    // Y坐标：在末地岛屿上找一个合适的高度
    i32 targetY = 75;  // 外岛通常在这个高度

    return BlockPos(targetX, targetY, targetZ);
}

bool EndGatewayFeature::canPlaceAt(
    WorldGenRegion& world,
    const BlockPos& pos) const
{
    // 检查周围是否足够空间
    // 折跃门需要至少3x3的基岩平台

    for (i32 x = -1; x <= 1; ++x) {
        for (i32 z = -1; z <= 1; ++z) {
            const BlockState* state = world.getBlock(pos.x + x, pos.y - 1, pos.z + z);
            if (!state || state->isAir()) {
                return false;
            }
        }
    }

    return true;
}

void EndGatewayFeature::generateGateway(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos)
{
    // 获取方块状态
    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    const BlockState* endGateway = VanillaBlocks::getState(VanillaBlocks::END_GATEWAY);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    if (!bedrock || !endGateway || !air) {
        return;
    }

    // 折跃门结构：
    // 底层：基岩平台 (5x5)
    // 中层：基岩墙 + 折跃门方块
    // 顶层：基岩覆盖

    // 底层基岩平台 (5x5)
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 z = -2; z <= 2; ++z) {
            world.setBlock(pos.x + x, pos.y, pos.z + z, bedrock);
        }
    }

    // 中层：基岩墙 (3x3)
    for (i32 y = 1; y <= 3; ++y) {
        // 边缘是基岩
        for (i32 x = -1; x <= 1; ++x) {
            world.setBlock(pos.x + x, pos.y + y, pos.z - 1, bedrock);
            world.setBlock(pos.x + x, pos.y + y, pos.z + 1, bedrock);
        }
        for (i32 z = -1; z <= 1; ++z) {
            world.setBlock(pos.x - 1, pos.y + y, pos.z + z, bedrock);
            world.setBlock(pos.x + 1, pos.y + y, pos.z + z, bedrock);
        }
    }

    // 中间是空气（除了折跃门方块位置）
    for (i32 y = 1; y <= 2; ++y) {
        world.setBlock(pos.x, pos.y + y, pos.z, air);
    }

    // 折跃门方块（在中心，Y=3）
    world.setBlock(pos.x, pos.y + 3, pos.z, endGateway);

    // 顶层基岩覆盖 (3x3)
    for (i32 x = -1; x <= 1; ++x) {
        for (i32 z = -1; z <= 1; ++z) {
            world.setBlock(pos.x + x, pos.y + 4, pos.z + z, bedrock);
        }
    }
}

// ============================================================================
// ConfiguredEndGatewayFeature 实现
// ============================================================================

ConfiguredEndGatewayFeature::ConfiguredEndGatewayFeature(
    std::unique_ptr<EndGatewayFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredEndGatewayFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// EndGatewayFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredEndGatewayFeature>> EndGatewayFeatures::s_features;

void EndGatewayFeatures::initialize() {
    if (!s_features.empty()) return;

    // 创建标准末地折跃门
    s_features.push_back(createGateway());

    // 创建退出折跃门
    s_features.push_back(createExitGateway());
}

const std::vector<std::unique_ptr<ConfiguredEndGatewayFeature>>& EndGatewayFeatures::getAllFeatures() {
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredEndGatewayFeature>> EndGatewayFeatures::getAllFeaturesAndClear() {
    return std::move(s_features);
}

std::unique_ptr<ConfiguredEndGatewayFeature> EndGatewayFeatures::createGateway() {
    auto config = std::make_unique<EndGatewayFeatureConfig>(false);
    return std::make_unique<ConfiguredEndGatewayFeature>(std::move(config), "end_gateway");
}

std::unique_ptr<ConfiguredEndGatewayFeature> EndGatewayFeatures::createExitGateway() {
    auto config = std::make_unique<EndGatewayFeatureConfig>(true);
    return std::make_unique<ConfiguredEndGatewayFeature>(std::move(config), "end_gateway_exit");
}

} // namespace mc
