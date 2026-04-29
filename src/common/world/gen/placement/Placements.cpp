#include "Placements.hpp"
#include "PlacementUtils.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../noise/OctavesNoiseGenerator.hpp"
#include "../../IWorld.hpp"
#include "../../../core/Constants.hpp"
#include <cmath>
#include <memory>

namespace mc {

// ============================================================================
// 噪声辅助函数
// ============================================================================

namespace {

/**
 * @brief 获取或创建区块噪声缓存
 *
 * 使用简单的哈希来为每个区块创建一致的噪声值
 */
f32 getChunkNoise(i64 seed, i32 chunkX, i32 chunkZ, f32 scale) {
    // 使用确定性哈希来模拟噪声
    // 完整实现应使用实际的噪声生成器
    u64 hash = static_cast<u64>(chunkX) * 3418731287ULL ^ static_cast<u64>(chunkZ) * 132897987541ULL;
    hash ^= static_cast<u64>(seed);

    // 将哈希转换为 [-1, 1] 范围的浮点数
    f32 value = static_cast<f32>((hash & 0x7FFFFFFF) % 10000) / 5000.0f - 1.0f;
    return value * scale;
}

} // anonymous namespace

// ============================================================================
// NoisePlacement 实现
// ============================================================================

std::vector<BlockPos> NoisePlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    const auto* noiseConfig = dynamic_cast<const NoisePlacementConfig*>(&config);
    if (!noiseConfig) {
        return {basePos};
    }

    // 计算噪声值
    // 参考 MC 1.16.5: NoiseBasedPlacement 使用 Biome 提供的噪声
    i32 chunkX = basePos.x >> 4;
    i32 chunkZ = basePos.z >> 4;

    f32 noiseValue = getChunkNoise(
        static_cast<i64>(random.nextInt()) & 0xFFFFFFFFLL,
        chunkX, chunkZ,
        static_cast<f32>(noiseConfig->noiseFactor)
    );

    // 应用偏移
    noiseValue += static_cast<f32>(noiseConfig->noiseOffset);

    // 比较阈值
    if (noiseValue < static_cast<f32>(noiseConfig->noiseLevel)) {
        return {basePos};
    }

    return {};
}

// ============================================================================
// CountNoisePlacement 实现
// ============================================================================

std::vector<BlockPos> CountNoisePlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    const auto* countConfig = dynamic_cast<const CountNoiseConfig*>(&config);
    if (!countConfig) {
        return {basePos};
    }

    // 计算噪声值
    i32 chunkX = basePos.x >> 4;
    i32 chunkZ = basePos.z >> 4;

    f32 noiseValue = getChunkNoise(
        static_cast<i64>(random.nextInt()) & 0xFFFFFFFFLL,
        chunkX, chunkZ,
        1.0f
    );

    // 根据噪声阈值决定数量
    i32 count = (noiseValue < static_cast<f32>(countConfig->noiseLevel))
        ? countConfig->belowCount
        : countConfig->aboveCount;

    std::vector<BlockPos> positions;
    positions.reserve(count);

    for (i32 i = 0; i < count; ++i) {
        i32 dx = random.nextInt(16);
        i32 dz = random.nextInt(16);
        positions.emplace_back(basePos.x + dx, basePos.y, basePos.z + dz);
    }

    return positions;
}

// ============================================================================
// DepthAveragePlacement 实现
// ============================================================================

std::vector<BlockPos> DepthAveragePlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    const auto* depthConfig = dynamic_cast<const DepthAverageConfig*>(&config);
    if (!depthConfig) {
        return {basePos};
    }

    (void)region;

    // 参考 MC DepthAveragePlacement.java 第18行
    // 使用三角形分布：nextInt(spread) + nextInt(spread) - spread + baseline
    // 这产生一个以baseline为中心的三角形分布
    const i32 j = depthConfig->spread;
    const i32 y = random.nextInt(j) + random.nextInt(j) - j + depthConfig->baseline;

    std::vector<BlockPos> positions;
    positions.emplace_back(basePos.x, y, basePos.z);
    return positions;
}

// ============================================================================
// TopSolidPlacement 实现
// ============================================================================

std::vector<BlockPos> TopSolidPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)config;
    (void)random;

    // 找到最高固体方块
    i32 topY = world::MAX_BUILD_HEIGHT - 1;
    for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* state = region.getBlock(basePos.x, y, basePos.z);
        if (state && state->isSolid()) {
            topY = y;
            break;
        }
    }

    std::vector<BlockPos> positions;
    positions.emplace_back(basePos.x, topY + 1, basePos.z);
    return positions;
}

// ============================================================================
// CarvingMaskPlacement 实现
// ============================================================================

std::vector<BlockPos> CarvingMaskPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)region;
    (void)config;

    // 在区块内按随机采样返回候选位置
    std::vector<BlockPos> positions;
    i32 count = random.nextInt(4) + 1;

    for (i32 i = 0; i < count; ++i) {
        i32 dx = random.nextInt(16);
        i32 dy = random.nextInt(40) + 10;  // Y 10-50
        i32 dz = random.nextInt(16);
        positions.emplace_back(basePos.x + dx, dy, basePos.z + dz);
    }

    return positions;
}

// ============================================================================
// RandomOffsetPlacement 实现
// ============================================================================

std::vector<BlockPos> RandomOffsetPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    const auto* offsetConfig = dynamic_cast<const RandomOffsetConfig*>(&config);
    if (!offsetConfig) {
        return {basePos};
    }

    (void)region;

    i32 dx = random.nextInt(offsetConfig->xzSpread * 2) - offsetConfig->xzSpread;
    i32 dy = random.nextInt(offsetConfig->ySpread * 2) - offsetConfig->ySpread;
    i32 dz = random.nextInt(offsetConfig->xzSpread * 2) - offsetConfig->xzSpread;

    std::vector<BlockPos> positions;
    positions.emplace_back(basePos.x + dx, basePos.y + dy, basePos.z + dz);
    return positions;
}

// ============================================================================
// WaterDepthThresholdPlacement 实现
// ============================================================================

std::vector<BlockPos> WaterDepthThresholdPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    const auto* depthConfig = dynamic_cast<const WaterDepthThresholdConfig*>(&config);
    if (!depthConfig) {
        return {basePos};
    }

    (void)random;

    // 检查水深
    i32 waterDepth = 0;
    for (i32 y = basePos.y; y > 0; --y) {
        const BlockState* state = region.getBlock(basePos.x, y, basePos.z);
        if (state && state->isLiquid()) {
            ++waterDepth;
        } else if (state && state->isSolid()) {
            break;
        }
    }

    if (waterDepth > depthConfig->maxWaterDepth) {
        return {};
    }

    return {basePos};
}

// ============================================================================
// SeaLevelPlacement 实现
// ============================================================================

std::vector<BlockPos> SeaLevelPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    const auto* seaConfig = dynamic_cast<const SeaLevelConfig*>(&config);
    if (!seaConfig) {
        return {basePos};
    }

    (void)region;
    (void)random;

    // 海平面默认为 63
    constexpr i32 SEA_LEVEL = 63;
    i32 y = SEA_LEVEL + seaConfig->offset;

    std::vector<BlockPos> positions;
    positions.emplace_back(basePos.x, y, basePos.z);
    return positions;
}

// ============================================================================
// SpreadPlacement 实现
// ============================================================================

std::vector<BlockPos> SpreadPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)region;
    (void)config;

    // 在原始位置周围扩散
    std::vector<BlockPos> positions;
    i32 count = random.nextInt(3) + 1;
    i32 spread = 8;

    for (i32 i = 0; i < count; ++i) {
        i32 dx = random.nextInt(spread * 2) - spread;
        i32 dz = random.nextInt(spread * 2) - spread;
        positions.emplace_back(basePos.x + dx, basePos.y, basePos.z + dz);
    }

    return positions;
}

} // namespace mc
