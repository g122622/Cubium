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
#include "CarvingContext.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/gen/carver/CarverConfiguration.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/carver/WorldCarver.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <cmath>
#include <cstddef>
#include <vector>

namespace mc {

// ============================================================================
// CanyonCarver 实现
// ============================================================================

CanyonCarver::CanyonCarver(i32 maxHeight)
    : WorldCarver<CanyonCarverConfiguration>(maxHeight)
{}

bool CanyonCarver::shouldCarve(
    math::IRandom& rng, ChunkCoord /*chunkX*/, ChunkCoord /*chunkZ*/, const CanyonCarverConfiguration& config) const
{
    return rng.nextFloat() <= config.probability;
}

bool CanyonCarver::carve(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::IBiomeSource& biomeSource,
    ChunkCoord targetChunkX,
    ChunkCoord targetChunkZ,
    ChunkCoord originChunkX,
    ChunkCoord originChunkZ,
    CarvingMask& carvingMask,
    math::IRandom& rng,
    const CanyonCarverConfiguration& config)
{
    const i32 range = getRange() * 2 - 1;
    const i32 tunnelLength = range * world::CHUNK_WIDTH;

    // MC 1.21.11: 位置追踪使用 double 精度，避免长峡谷路径精度漂移
    const f64 startX =
        static_cast<f64>(world::toWorldCoord(originChunkX)) + static_cast<f64>(rng.nextInt(world::CHUNK_WIDTH));
    const i32 startY = config.y->sample(rng, context);
    const f64 startZ =
        static_cast<f64>(world::toWorldCoord(originChunkZ)) + static_cast<f64>(rng.nextInt(world::CHUNK_WIDTH));

    const f32 yaw = rng.nextFloat() * math::TWO_PI;
    const f32 pitch = config.verticalRotation->sample(rng);
    const f64 yScale = static_cast<f64>(config.yScale->sample(rng));
    const f32 thickness = config.shape.thickness->sample(rng);
    const i32 length = static_cast<i32>(static_cast<f32>(tunnelLength) * config.shape.distanceFactor->sample(rng));

    _generateCanyon(chunk,
        context,
        biomeSource,
        targetChunkX,
        targetChunkZ,
        rng.nextLong(),
        startX,
        static_cast<f64>(startY),
        startZ,
        thickness,
        yaw,
        pitch,
        0,
        length,
        yScale,
        carvingMask,
        config);

    return true;
}

std::vector<f32> CanyonCarver::_initWidthFactors(
    CarvingContext& context, const CanyonCarverConfiguration& config, math::IRandom& rng) const
{
    const i32 genDepth = context.getGenDepth();
    std::vector<f32> factors(static_cast<size_t>(genDepth));

    f32 currentFactor = 1.0f;
    for (i32 j = 0; j < genDepth; ++j) {
        if (j == 0 || rng.nextInt(config.shape.widthSmoothness) == 0) {
            currentFactor = 1.0f + rng.nextFloat() * rng.nextFloat();
        }
        factors[static_cast<size_t>(j)] = currentFactor * currentFactor;
    }

    return factors;
}

f32 CanyonCarver::_updateVerticalRadius(
    const CanyonCarverConfiguration& config, math::IRandom& rng, f32 baseRadius, f32 totalSteps, f32 currentStep) const
{
    const f32 progress = currentStep / totalSteps;
    const f32 f = 1.0f - std::abs(0.5f - progress) * 2.0f;
    const f32 f1 = config.shape.verticalRadiusDefaultFactor + config.shape.verticalRadiusCenterFactor * f;

    const f32 randomFactor = rng.nextFloat() * 0.25f + 0.75f;
    return f1 * baseRadius * randomFactor;
}

void CanyonCarver::_generateCanyon(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::IBiomeSource& biomeSource,
    ChunkCoord targetChunkX,
    ChunkCoord targetChunkZ,
    i64 seed,
    f64 startX,
    f64 startY,
    f64 startZ,
    f32 thickness,
    f32 yaw,
    f32 pitch,
    i32 startIndex,
    i32 endIndex,
    f64 yScale,
    CarvingMask& carvingMask,
    const CanyonCarverConfiguration& config)
{
    math::Random rng(static_cast<u64>(seed));

    const std::vector<f32> heightThresholds = _initWidthFactors(context, config, rng);

    const CarveSkipChecker skipChecker = _createSkipChecker(context, heightThresholds);

    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // MC 1.21.11: horizontalRadius 和 verticalRadius 使用 double 精度
        const f64 horizontalRadius =
            1.5 + std::sin(static_cast<f64>(i) * math::PI / static_cast<f64>(endIndex)) * static_cast<f64>(thickness);
        f64 verticalRadius = horizontalRadius * yScale;

        const f32 horizontalRadiusFactor = config.shape.horizontalRadiusFactor->sample(rng);
        const f64 scaledHorizontalRadius = horizontalRadius * static_cast<f64>(horizontalRadiusFactor);

        verticalRadius = _updateVerticalRadius(
            config, rng, static_cast<f32>(verticalRadius), static_cast<f32>(endIndex), static_cast<f32>(i));

        const f64 cosPitch = static_cast<f64>(std::cos(pitch));
        const f64 sinPitch = static_cast<f64>(std::sin(pitch));
        startX += static_cast<f64>(std::cos(yaw)) * cosPitch;
        startY += sinPitch;
        startZ += static_cast<f64>(std::sin(yaw)) * cosPitch;

        pitch *= 0.7f;
        pitch += pitchModifier * 0.05f;
        yaw += yawModifier * 0.05f;

        pitchModifier *= 0.8f;
        yawModifier *= 0.5f;
        pitchModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        if (rng.nextInt(4) != 0) {
            if (!isInCarvingRange(targetChunkX,
                    targetChunkZ,
                    static_cast<f32>(startX),
                    static_cast<f32>(startZ),
                    i,
                    endIndex,
                    thickness)) {
                return;
            }

            carveEllipsoid(chunk,
                context,
                biomeSource,
                targetChunkX,
                targetChunkZ,
                static_cast<f32>(startX),
                static_cast<f32>(startY),
                static_cast<f32>(startZ),
                static_cast<f32>(scaledHorizontalRadius),
                static_cast<f32>(verticalRadius),
                carvingMask,
                skipChecker,
                config);
        }
    }
}

CarveSkipChecker CanyonCarver::_createSkipChecker(
    CarvingContext& context, const std::vector<f32>& heightThresholds) const
{
    const i32 minGenY = context.getMinGenY();

    return [&heightThresholds, minGenY](const CarverEllipsePos& pos) -> bool {
        const i32 index = pos.y - minGenY - 1;
        if (index < 0 || index >= static_cast<i32>(heightThresholds.size())) {
            // 超出范围时使用标准椭球检测
            return pos.dx * pos.dx + pos.dy * pos.dy + pos.dz * pos.dz >= 1.0f;
        }
        const f32 threshold = heightThresholds[static_cast<size_t>(index)];
        return (pos.dx * pos.dx + pos.dz * pos.dz) * threshold + pos.dy * pos.dy / 6.0f >= 1.0f;
    };
}

} // namespace mc
