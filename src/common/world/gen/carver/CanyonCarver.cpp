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

#include "CanyonCarver.hpp"
#include "../../../core/Constants.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../block/BlockRegistry.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// CanyonCarver 实现
// ============================================================================

CanyonCarver::CanyonCarver(i32 maxHeight)
    : WorldCarver<ProbabilityConfig>(maxHeight)
    , m_heightThresholds(HEIGHT_THRESHOLD_TABLE_SIZE, 1.0f)
{
    _initializeHeightThresholds();
}

void CanyonCarver::_initializeHeightThresholds()
{
    // 为每个高度预计算半径变化因子
    math::Random rng(0); // 使用固定种子生成确定性阈值

    for (size_t i = 0; i < m_heightThresholds.size(); ++i) {
        if (i == 0 || rng.nextInt(3) == 0) {
            // 生成新的随机因子
            f32 factor = 1.0f + rng.nextFloat() * rng.nextFloat();
            m_heightThresholds[i] = factor * factor;
        } else {
            // 使用上一个值
            if (i > 0) {
                m_heightThresholds[i] = m_heightThresholds[i - 1];
            }
        }
    }
}

bool CanyonCarver::shouldCarve(
    math::IRandom& rng, ChunkCoord /*chunkX*/, ChunkCoord /*chunkZ*/, const ProbabilityConfig& config) const
{
    return rng.nextFloat() <= config.probability;
}

bool CanyonCarver::carve(ChunkPrimer& chunk,
    const world::biome::BiomeSource& biomeSource,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    CarvingMask& carvingMask,
    const ProbabilityConfig& config)
{
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL +
        static_cast<u64>(m_maxHeight) + 1);

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    const i32 tunnelLength = (getRange() * 2 - 1) * world::CHUNK_WIDTH;
    const i32 startX = world::toWorldCoord(chunkX);
    const i32 startZ = world::toWorldCoord(chunkZ);

    // 峡谷起点
    const f32 canyonX = static_cast<f32>(startX) + rng.nextFloat(0.0f, static_cast<f32>(world::CHUNK_WIDTH));
    const f32 canyonY = static_cast<f32>(rng.nextInt(rng.nextInt(40) + 8) + 20);
    const f32 canyonZ = static_cast<f32>(startZ) + rng.nextFloat(0.0f, static_cast<f32>(world::CHUNK_WIDTH));

    // 峡谷方向和尺寸
    const f32 yaw = rng.nextFloat(0.0f, math::TWO_PI);
    const f32 pitch = (rng.nextFloat() - 0.5f) * 2.0f / 8.0f;
    const f32 radius = (rng.nextFloat() * 2.0f + rng.nextFloat()) * 2.0f;

    // 峡谷长度
    const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

    // 生成蜿蜒峡谷
    _generateCanyon(chunk,
        biomeSource,
        seaLevel,
        chunkX,
        chunkZ,
        static_cast<i64>(rng.nextU64()),
        canyonX,
        canyonY,
        canyonZ,
        radius,
        yaw,
        pitch,
        0,
        length,
        3.0f,
        carvingMask);

    return true;
}

bool CanyonCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 峡谷使用特殊的厚度检测：考虑Y坐标的半径变化

    // 如果 y <= 0 或超出阈值表范围，使用标准检测
    if (y <= 0 || y > static_cast<i32>(m_heightThresholds.size())) {
        return dx * dx + dy * dy + dz * dz >= 1.0f;
    }

    // 使用预计算的阈值
    const f32 threshold = m_heightThresholds[static_cast<size_t>(y - 1)];
    return (dx * dx + dz * dz) * threshold + dy * dy / 6.0f >= 1.0f;
}

void CanyonCarver::_generateCanyon(ChunkPrimer& chunk,
    const world::biome::BiomeSource& biomeSource,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    i64 seed,
    f32 startX,
    f32 startY,
    f32 startZ,
    f32 radius,
    f32 yaw,
    f32 pitch,
    i32 startIndex,
    i32 endIndex,
    f32 horizontalScale,
    CarvingMask& carvingMask)
{
    math::Random rng(static_cast<u64>(seed));

    // 重新初始化高度阈值
    std::vector<f32> heightThresholds(world::MAX_BUILD_HEIGHT);
    f32 currentThreshold = 1.0f;
    for (i32 i = 0; i < world::MAX_BUILD_HEIGHT; ++i) {
        if (i == 0 || rng.nextInt(3) == 0) {
            currentThreshold = 1.0f + rng.nextFloat() * rng.nextFloat();
            currentThreshold = currentThreshold * currentThreshold;
        }
        heightThresholds[static_cast<size_t>(i)] = currentThreshold;
    }

    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // 计算当前半径（正弦曲线变化）
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIndex);
        const f32 sinProgress = std::sin(progress * math::PI);
        f32 horizontalRadius = radius * sinProgress;
        f32 verticalRadius = horizontalRadius * horizontalScale;

        // 添加随机变化
        horizontalRadius *= rng.nextFloat() * 0.25f + 0.75f;
        verticalRadius *= rng.nextFloat() * 0.25f + 0.75f;

        // 更新位置
        const f32 cosPitch = std::cos(pitch);
        startX += std::cos(yaw) * cosPitch;
        startY += std::sin(pitch);
        startZ += std::sin(yaw) * cosPitch;

        // 更新角度
        pitch *= 0.7f;
        pitch += pitchModifier * 0.05f;
        yaw += yawModifier * 0.05f;

        // 衰减和随机扰动
        pitchModifier *= 0.8f;
        yawModifier *= 0.5f;
        pitchModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        // 随机跳过一些点
        if (rng.nextInt(4) == 0) {
            continue;
        }

        // 检查是否在雕刻范围内
        if (isInCarvingRange(chunkX, chunkZ, startX, startZ, i, endIndex, radius)) {
            carveEllipsoid(chunk,
                biomeSource,
                seaLevel,
                chunkX,
                chunkZ,
                startX,
                startY,
                startZ,
                horizontalRadius,
                verticalRadius,
                carvingMask,
                static_cast<i64>(rng.nextU64()));
        }
    }
}

f32 CanyonCarver::_updateRadius(f32 baseRadius, f32 progress, const std::vector<f32>& thresholds, i32 index) const
{
    // 峡谷入口较宽，深处较窄
    const f32 factor = 1.0f - progress * 0.3f;

    // 如果索引在阈值表范围内，使用阈值
    if (index >= 0 && static_cast<size_t>(index) < thresholds.size()) {
        return baseRadius * factor * thresholds[static_cast<size_t>(index)];
    }

    return baseRadius * factor;
}

} // namespace mc
