/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "Placements.hpp"
#include "PlacementUtils.hpp"
#include "common/core/Constants.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/noise/PerlinSimplexNoise.hpp"
#include <cmath>
#include <memory>

namespace mc {

// ============================================================================
// 噪声辅助
// ============================================================================

// MC 1.21.11: NoisePlacement / CountNoisePlacement 使用 Biome.BIOME_INFO_NOISE。
// 该噪声为 PerlinSimplexNoise(seed=2345, octaves=[0])，项目已有 MC 准确的全局单例
// world::biome::biomeInfoNoise()，直接复用。

// ============================================================================
// NoisePlacement 实现
// ============================================================================

std::vector<BlockPos> NoisePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto* noiseConfig = dynamic_cast<const NoisePlacementConfig*>(&config);
    if (!noiseConfig) {
        return {basePos};
    }

    (void)region;
    (void)random;

    // MC 1.21.11: 使用 BIOME_INFO_NOISE 采样噪声值
    // noiseAt(x / noiseFactor, z / noiseFactor, false)
    const f64 noiseValue = world::biome::biomeInfoNoise().getValue(
        static_cast<f64>(basePos.x) / static_cast<f64>(noiseConfig->noiseFactor),
        static_cast<f64>(basePos.z) / static_cast<f64>(noiseConfig->noiseFactor),
        false);

    // 应用偏移
    const f64 adjustedValue = noiseValue + static_cast<f64>(noiseConfig->noiseOffset);

    // 比较阈值：低于阈值时通过
    if (adjustedValue < static_cast<f64>(noiseConfig->noiseLevel)) {
        return {basePos};
    }

    return {};
}

// ============================================================================
// CountNoisePlacement 实现
// ============================================================================

std::vector<BlockPos> CountNoisePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto* countConfig = dynamic_cast<const CountNoiseConfig*>(&config);
    if (!countConfig) {
        return {basePos};
    }

    (void)region;

    // MC 1.21.11: 使用 BIOME_INFO_NOISE 采样噪声值
    // noiseAt(x / 200.0, z / 200.0, false)
    const f64 noiseValue = world::biome::biomeInfoNoise().getValue(
        static_cast<f64>(basePos.x) / 200.0, static_cast<f64>(basePos.z) / 200.0, false);

    // 根据噪声阈值决定数量
    const i32 count = (noiseValue < countConfig->noiseLevel) ? countConfig->belowCount : countConfig->aboveCount;

    std::vector<BlockPos> positions;
    positions.reserve(static_cast<size_t>(count));

    for (i32 i = 0; i < count; ++i) {
        i32 dx = random.nextInt(world::CHUNK_WIDTH);
        i32 dz = random.nextInt(world::CHUNK_WIDTH);
        positions.emplace_back(basePos.x + dx, basePos.y, basePos.z + dz);
    }

    return positions;
}

// ============================================================================
// DepthAveragePlacement 实现
// ============================================================================

std::vector<BlockPos> DepthAveragePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto* depthConfig = dynamic_cast<const DepthAverageConfig*>(&config);
    if (!depthConfig) {
        return {basePos};
    }

    (void)region;

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
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)config;
    (void)random;

    // MC 1.21.11: 使用 MOTION_BLOCKING_NO_LEAVES 高度图
    i32 topY = region.getTopBlockY(basePos.x, basePos.z, HeightmapType::MotionBlockingNoLeaves);
    if (topY <= world::MIN_BUILD_HEIGHT) {
        // 回退到 MOTION_BLOCKING
        topY = region.getTopBlockY(basePos.x, basePos.z, HeightmapType::MotionBlocking);
    }

    if (topY <= world::MIN_BUILD_HEIGHT) {
        // 最终回退：手动搜索
        for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
            const BlockState* state = region.getBlockState(basePos.x, y, basePos.z);
            if (state && state->isSolid()) {
                topY = y;
                break;
            }
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
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;
    (void)config;

    // 在区块内按随机采样返回候选位置
    std::vector<BlockPos> positions;
    i32 count = random.nextInt(4) + 1;

    for (i32 i = 0; i < count; ++i) {
        i32 dx = random.nextInt(world::CHUNK_WIDTH);
        i32 dy = random.nextInt(40) + 10; // Y 10-50
        i32 dz = random.nextInt(world::CHUNK_WIDTH);
        positions.emplace_back(basePos.x + dx, dy, basePos.z + dz);
    }

    return positions;
}

// ============================================================================
// RandomOffsetPlacement 实现
// ============================================================================

std::vector<BlockPos> RandomOffsetPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;
    const auto* offsetConfig = dynamic_cast<const RandomOffsetConfig*>(&config);
    if (!offsetConfig || !offsetConfig->xzSpread || !offsetConfig->ySpread) {
        return {basePos};
    }

    i32 dx = offsetConfig->xzSpread->sample(random);
    i32 dy = offsetConfig->ySpread->sample(random);
    i32 dz = offsetConfig->xzSpread->sample(random);

    std::vector<BlockPos> positions;
    positions.emplace_back(basePos.x + dx, basePos.y + dy, basePos.z + dz);
    return positions;
}

// ============================================================================
// WaterDepthThresholdPlacement 实现
// ============================================================================

std::vector<BlockPos> WaterDepthThresholdPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto* depthConfig = dynamic_cast<const WaterDepthThresholdConfig*>(&config);
    if (!depthConfig) {
        return {basePos};
    }

    (void)random;

    // 检查水深
    i32 waterDepth = 0;
    for (i32 y = basePos.y; y > world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* state = region.getBlockState(basePos.x, y, basePos.z);
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
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto* seaConfig = dynamic_cast<const SeaLevelConfig*>(&config);
    if (!seaConfig) {
        return {basePos};
    }

    (void)region;
    (void)random;

    i32 y = world::SEA_LEVEL + seaConfig->offset;

    std::vector<BlockPos> positions;
    positions.emplace_back(basePos.x, y, basePos.z);
    return positions;
}

// ============================================================================
// SpreadPlacement 实现
// ============================================================================

std::vector<BlockPos> SpreadPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
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

// ============================================================================
// CountExtraPlacement 实现
// ============================================================================

std::vector<BlockPos> CountExtraPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto* extraConfig = dynamic_cast<const AtSurfaceWithExtraConfig*>(&config);
    if (!extraConfig) {
        return {basePos};
    }

    (void)region;

    // 基础数量 + 概率额外数量
    i32 count = extraConfig->count;
    if (random.nextFloat() < extraConfig->extraChance) {
        count += extraConfig->extraCount;
    }

    std::vector<BlockPos> positions;
    positions.reserve(count);

    for (i32 i = 0; i < count; ++i) {
        positions.push_back(basePos);
    }

    return positions;
}

} // namespace mc
