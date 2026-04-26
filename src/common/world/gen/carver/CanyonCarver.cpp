#include "CanyonCarver.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../core/Constants.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

// ============================================================================
// CanyonCarver 实现
// ============================================================================

CanyonCarver::CanyonCarver(i32 maxHeight)
    : WorldCarver<ProbabilityConfig>(maxHeight)
    , m_heightThresholds(1024, 1.0f)  // MC 原版大小是 1024
{
    initializeHeightThresholds();
}

void CanyonCarver::initializeHeightThresholds()
{
    // 参考 MC CanyonWorldCarver 构造函数
    // 为每个高度预计算半径变化因子
    math::Random rng(0);  // 使用固定种子生成确定性阈值

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
    math::IRandom& rng,
    ChunkCoord /*chunkX*/,
    ChunkCoord /*chunkZ*/,
    const ProbabilityConfig& config) const
{
    return rng.nextFloat() <= config.probability;
}

bool CanyonCarver::carve(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    CarvingMask& carvingMask,
    const ProbabilityConfig& config)
{
    // 参考 MC CanyonWorldCarver.carveRegion
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL +
                     static_cast<u64>(chunkZ) * 132897987541ULL +
                     static_cast<u64>(m_maxHeight) + 1);

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    const i32 tunnelLength = (getRange() * 2 - 1) * 16;
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 峡谷起点
    // 参考 MC: double d1 = (double)(rand.nextInt(rand.nextInt(40) + 8) + 20);
    const f32 canyonX = static_cast<f32>(startX) + rng.nextFloat(0.0f, 16.0f);
    const f32 canyonY = static_cast<f32>(rng.nextInt(rng.nextInt(40) + 8) + 20);
    const f32 canyonZ = static_cast<f32>(startZ) + rng.nextFloat(0.0f, 16.0f);

    // 峡谷方向和尺寸
    const f32 yaw = rng.nextFloat(0.0f, math::TWO_PI);
    // 参考 MC: float f1 = (rand.nextFloat() - 0.5F) * 2.0F / 8.0F;
    const f32 pitch = (rng.nextFloat() - 0.5f) * 2.0f / 8.0f;
    // 参考 MC: float f2 = (rand.nextFloat() * 2.0F + rand.nextFloat()) * 2.0F;
    const f32 radius = (rng.nextFloat() * 2.0f + rng.nextFloat()) * 2.0f;

    // 峡谷长度
    const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

    // 生成蜿蜒峡谷
    generateCanyon(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                   static_cast<i64>(rng.nextU64()),
                   canyonX, canyonY, canyonZ,
                   radius, yaw, pitch,
                   0, length, 3.0f,
                   carvingMask);

    return true;
}

bool CanyonCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 参考 MC CanyonWorldCarver.func_222708_a_
    // return (dx * dx + dz * dz) * this.field_202536_i[y - 1] + dy * dy / 6.0D >= 1.0D;
    // 峡谷使用特殊的厚度检测：考虑Y坐标的半径变化

    // 如果 y <= 0 或超出阈值表范围，使用标准检测
    if (y <= 0 || y > static_cast<i32>(m_heightThresholds.size())) {
        return dx * dx + dy * dy + dz * dz >= 1.0f;
    }

    // 使用预计算的阈值
    // 注意：MC 使用 y - 1 作为索引
    const f32 threshold = m_heightThresholds[static_cast<size_t>(y - 1)];
    return (dx * dx + dz * dz) * threshold + dy * dy / 6.0f >= 1.0f;
}

void CanyonCarver::generateCanyon(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    i64 seed,
    f32 startX, f32 startY, f32 startZ,
    f32 radius,
    f32 yaw, f32 pitch,
    i32 startIndex, i32 endIndex,
    f32 horizontalScale,
    CarvingMask& carvingMask)
{
    // 参考 MC CanyonWorldCarver.func_227204_a_
    math::Random rng(static_cast<u64>(seed));

    // 重新初始化高度阈值（MC 每次生成时都重新计算）
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
        // 参考 MC: double d0 = 1.5D + (double)(MathHelper.sin((float)j * (float)Math.PI / (float)p_227204_18_) * p_227204_14_) * d0;
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIndex);
        const f32 sinProgress = std::sin(progress * math::PI);
        f32 horizontalRadius = radius * sinProgress;
        f32 verticalRadius = horizontalRadius * horizontalScale;

        // 添加随机变化（参考MC）
        horizontalRadius *= rng.nextFloat() * 0.25f + 0.75f;
        verticalRadius *= rng.nextFloat() * 0.25f + 0.75f;

        // 更新位置（参考MC的方向计算）
        // float f2 = MathHelper.cos(p_227204_16_);
        // randOffsetXCoord += (double)(MathHelper.cos(pitch) * f2);
        // startY += (double)MathHelper.sin(p_227204_16_);
        // randOffsetZCoord += (double)(MathHelper.sin(pitch) * f2);
        const f32 cosPitch = std::cos(pitch);
        startX += std::cos(yaw) * cosPitch;
        startY += std::sin(pitch);
        startZ += std::sin(yaw) * cosPitch;

        // 更新角度（参考MC的衰减和扰动顺序）
        // p_227204_16_ = p_227204_16_ * 0.7F;
        // p_227204_16_ = p_227204_16_ + f1 * 0.05F;
        // pitch += f * 0.1F;
        pitch *= 0.7f;
        pitch += pitchModifier * 0.05f;
        yaw += yawModifier * 0.05f;

        // 衰减和随机扰动
        // f1 = f1 * 0.8F;
        // f = f * 0.5F;
        // f1 = f1 + (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 2.0F;
        // f = f + (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 4.0F;
        pitchModifier *= 0.8f;
        yawModifier *= 0.5f;
        pitchModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        // 随机跳过一些点（每4步跳过1次）
        // MC: if (random.nextInt(4) != 0) { ... }
        if (rng.nextInt(4) == 0) {
            continue;
        }

        // 检查是否在雕刻范围内
        if (isInCarvingRange(chunkX, chunkZ, startX, startZ, i, endIndex, radius)) {
            carveEllipsoid(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                           startX, startY, startZ,
                           horizontalRadius,
                           verticalRadius,
                           carvingMask, static_cast<i64>(rng.nextU64()));
        }
    }
}

f32 CanyonCarver::updateRadius(
    f32 baseRadius,
    f32 progress,
    const std::vector<f32>& thresholds,
    i32 index) const
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
