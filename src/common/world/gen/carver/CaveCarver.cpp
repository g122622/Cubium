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

#include "CaveCarver.hpp"
#include "CarverConfiguration.hpp"
#include "CarvingContext.hpp"
#include "WorldCarver.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// CaveCarver 实现
// ============================================================================

CaveCarver::CaveCarver()
    : WorldCarver<CaveCarverConfiguration>()
{}

bool CaveCarver::shouldCarve(math::IRandom& rng,
    ChunkCoord /*chunkX*/,
    ChunkCoord /*chunkZ*/,
    const CaveCarverConfiguration& config) const noexcept
{
    return rng.nextFloat() <= config.probability;
}

bool CaveCarver::carve(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::IBiomeSource& biomeSource,
    ChunkCoord targetChunkX,
    ChunkCoord targetChunkZ,
    ChunkCoord originChunkX,
    ChunkCoord originChunkZ,
    CarvingMask& carvingMask,
    math::IRandom& rng,
    const CaveCarverConfiguration& config)
{
    const i32 tunnelLength = (getRange() * 2 - 1) * world::CHUNK_WIDTH;

    const i32 numCaves = rng.nextInt(rng.nextInt(rng.nextInt(getCaveBound()) + 1) + 1);

    bool carved = false;

    const i32 originBlockX = originChunkX * world::CHUNK_WIDTH;
    const i32 originBlockZ = originChunkZ * world::CHUNK_WIDTH;

    for (i32 i = 0; i < numCaves; ++i) {
        const f64 startX = static_cast<f64>(originBlockX + rng.nextInt(world::CHUNK_WIDTH));
        const f64 startY = config.y->sample(rng, context);
        const f64 startZ = static_cast<f64>(originBlockZ + rng.nextInt(world::CHUNK_WIDTH));

        const f64 horizontalRadiusMultiplier = config.horizontalRadiusMultiplier->sample(rng);
        const f64 verticalRadiusMultiplier = config.verticalRadiusMultiplier->sample(rng);
        const f64 floorLevel = config.floorLevel->sample(rng);

        CarveSkipChecker skipChecker = [floorLevel](const CarverEllipsePos& pos) -> bool {
            return pos.dy <= floorLevel || pos.dx * pos.dx + pos.dy * pos.dy + pos.dz * pos.dz >= 1.0;
        };

        i32 numTunnels = 1;

        if (rng.nextInt(4) == 0) {
            const f64 yScale = config.yScale->sample(rng);
            const f32 roomRadius = 1.0f + rng.nextFloat() * 6.0f;

            _createRoom(chunk,
                context,
                biomeSource,
                targetChunkX,
                targetChunkZ,
                startX,
                startY,
                startZ,
                roomRadius,
                yScale,
                carvingMask,
                skipChecker,
                config);

            numTunnels += rng.nextInt(4);
        }

        for (i32 tunnelIdx = 0; tunnelIdx < numTunnels; ++tunnelIdx) {
            const f32 yaw = rng.nextFloat() * math::TWO_PI;
            const f32 pitch = (rng.nextFloat() - 0.5f) / 4.0f;

            const f32 thickness = getThickness(rng);

            const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

            _createTunnel(chunk,
                context,
                biomeSource,
                targetChunkX,
                targetChunkZ,
                static_cast<i64>(rng.nextU64()),
                startX,
                startY,
                startZ,
                horizontalRadiusMultiplier,
                verticalRadiusMultiplier,
                thickness,
                yaw,
                pitch,
                0,
                length,
                static_cast<f64>(getYScale()),
                carvingMask,
                skipChecker,
                config);
        }

        carved = true;
    }

    return carved;
}

f32 CaveCarver::getThickness(math::IRandom& rng) const
{
    f32 thickness = rng.nextFloat() * 2.0f + rng.nextFloat();

    if (rng.nextInt(10) == 0) {
        thickness *= rng.nextFloat() * rng.nextFloat() * 3.0f + 1.0f;
    }

    return thickness;
}

void CaveCarver::_createTunnel(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::IBiomeSource& biomeSource,
    ChunkCoord targetChunkX,
    ChunkCoord targetChunkZ,
    i64 seed,
    f64 startX,
    f64 startY,
    f64 startZ,
    f64 horizontalRadiusMultiplier,
    f64 verticalRadiusMultiplier,
    f32 thickness,
    f32 yaw,
    f32 pitch,
    i32 startIndex,
    i32 endIndex,
    f64 yScale,
    CarvingMask& carvingMask,
    const CarveSkipChecker& skipChecker,
    const CaveCarverConfiguration& config)
{
    math::Random rng(static_cast<u64>(seed));

    const i32 branchPoint = endIndex / 4 + rng.nextInt(endIndex / 2);
    const bool canBranch = rng.nextInt(6) == 0;

    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;
    f64 currentX = startX;
    f64 currentY = startY;
    f64 currentZ = startZ;

    for (i32 i = startIndex; i < endIndex; ++i) {
        const f64 horizontalRadius = 1.5 + std::sin(static_cast<f64>(math::PI * i) / endIndex) * thickness;
        const f64 verticalRadius = horizontalRadius * yScale;

        const f32 cosPitch = std::cos(pitch);

        currentX += std::cos(static_cast<f64>(yaw)) * cosPitch;
        currentY += std::sin(static_cast<f64>(pitch));
        currentZ += std::sin(static_cast<f64>(yaw)) * cosPitch;

        pitch *= (canBranch ? 0.92f : 0.7f);
        pitch += pitchModifier * 0.1f;
        yaw += yawModifier * 0.1f;

        pitchModifier *= 0.9f;
        yawModifier *= 0.75f;
        pitchModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        if (i == branchPoint && thickness > 1.0f) {
            // 分支递归调用
            const f32 branchThickness = rng.nextFloat() * 0.5f + 0.5f;

            _createTunnel(chunk,
                context,
                biomeSource,
                targetChunkX,
                targetChunkZ,
                static_cast<i64>(rng.nextU64()),
                currentX,
                currentY,
                currentZ,
                horizontalRadiusMultiplier,
                verticalRadiusMultiplier,
                branchThickness,
                yaw - math::HALF_PI,
                pitch / 3.0f,
                i,
                endIndex,
                1.0,
                carvingMask,
                skipChecker,
                config);

            _createTunnel(chunk,
                context,
                biomeSource,
                targetChunkX,
                targetChunkZ,
                static_cast<i64>(rng.nextU64()),
                currentX,
                currentY,
                currentZ,
                horizontalRadiusMultiplier,
                verticalRadiusMultiplier,
                branchThickness,
                yaw + math::HALF_PI,
                pitch / 3.0f,
                i,
                endIndex,
                1.0,
                carvingMask,
                skipChecker,
                config);

            return;
        }

        if (rng.nextInt(4) != 0) {
            if (!isInCarvingRange(targetChunkX,
                    targetChunkZ,
                    static_cast<f32>(currentX),
                    static_cast<f32>(currentZ),
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
                static_cast<f32>(currentX),
                static_cast<f32>(currentY),
                static_cast<f32>(currentZ),
                static_cast<f32>(horizontalRadius * horizontalRadiusMultiplier),
                static_cast<f32>(verticalRadius * verticalRadiusMultiplier),
                carvingMask,
                skipChecker,
                config);
        }
    }
}

void CaveCarver::_createRoom(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::IBiomeSource& biomeSource,
    ChunkCoord targetChunkX,
    ChunkCoord targetChunkZ,
    f64 centerX,
    f64 centerY,
    f64 centerZ,
    f32 radius,
    f64 yScale,
    CarvingMask& carvingMask,
    const CarveSkipChecker& skipChecker,
    const CaveCarverConfiguration& config)
{
    const f64 horizontalRadius = 1.5 + std::sin(math::HALF_PI) * radius;
    const f64 verticalRadius = horizontalRadius * yScale;

    carveEllipsoid(chunk,
        context,
        biomeSource,
        targetChunkX,
        targetChunkZ,
        static_cast<f32>(centerX + 1.0),
        static_cast<f32>(centerY),
        static_cast<f32>(centerZ),
        static_cast<f32>(horizontalRadius),
        static_cast<f32>(verticalRadius),
        carvingMask,
        skipChecker,
        config);
}

} // namespace mc
