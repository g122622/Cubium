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
#include "common/core/Constants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
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
    const world::biome::BiomeSource& biomeSource,
    ChunkCoord targetChunkX,
    ChunkCoord targetChunkZ,
    ChunkCoord originChunkX,
    ChunkCoord originChunkZ,
    CarvingMask& carvingMask,
    math::IRandom& rng,
    const CaveCarverConfiguration& config)
{
    // MC 1.21.11: shouldCarve (isStartChunk) 已由 applyCarvers 通过 setLargeFeatureSeed 完成
    // RNG 已正确初始化，直接使用

    // MC: int i = SectionPos.sectionToBlockCoord(this.getRange() * 2 - 1);
    const i32 tunnelLength = (getRange() * 2 - 1) * world::CHUNK_WIDTH;

    // MC: int j = random.nextInt(random.nextInt(random.nextInt(this.getCaveBound()) + 1) + 1);
    const i32 numCaves = rng.nextInt(rng.nextInt(rng.nextInt(getCaveBound()) + 1) + 1);

    bool carved = false;

    // MC: 使用 ORIGIN 区块坐标计算起始位置
    const i32 originBlockX = originChunkX * world::CHUNK_WIDTH;
    const i32 originBlockZ = originChunkZ * world::CHUNK_WIDTH;

    for (i32 i = 0; i < numCaves; ++i) {
        // MC: double d0 = chunkPos.getBlockX(random.nextInt(16));
        //     double d1 = config.y.sample(random, context);
        //     double d2 = chunkPos.getBlockZ(random.nextInt(16));
        const f64 startX = static_cast<f64>(originBlockX + rng.nextInt(world::CHUNK_WIDTH));
        const f64 startY = config.y->sample(rng, context);
        const f64 startZ = static_cast<f64>(originBlockZ + rng.nextInt(world::CHUNK_WIDTH));

        // MC: double d3 = config.horizontalRadiusMultiplier.sample(random);
        //     double d4 = config.verticalRadiusMultiplier.sample(random);
        //     double d5 = config.floorLevel.sample(random);
        const f64 horizontalRadiusMultiplier = config.horizontalRadiusMultiplier->sample(rng);
        const f64 verticalRadiusMultiplier = config.verticalRadiusMultiplier->sample(rng);
        const f64 floorLevel = config.floorLevel->sample(rng);

        // MC: CarveSkipChecker skipChecker = (dx, dy, dz, y) -> shouldSkip(dy, dx, dz, floorLevel);
        //     shouldSkip: return dy <= floorLevel || dx*dx + dy*dy + dz*dz >= 1.0
        CarveSkipChecker skipChecker = [floorLevel](f32 dx, f32 dy, f32 dz, i32 /*y*/) -> bool {
            return dy <= floorLevel || dx * dx + dy * dy + dz * dz >= 1.0;
        };

        i32 numTunnels = 1;

        if (rng.nextInt(4) == 0) {
            // MC: double d6 = config.yScale.sample(random);
            const f64 yScale = config.yScale->sample(rng);
            // MC: float f1 = 1.0F + random.nextFloat() * 6.0F;
            const f32 roomRadius = 1.0f + rng.nextFloat() * 6.0f;

            _createRoom(chunk,
                context,
                biomeSource,
                0,
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

        // MC: for (int k1 = 0; k1 < l; k1++)
        for (i32 tunnelIdx = 0; tunnelIdx < numTunnels; ++tunnelIdx) {
            // MC: float f = random.nextFloat() * (float) (Math.PI * 2);
            //     float f3 = (random.nextFloat() - 0.5F) / 4.0F;
            const f32 yaw = rng.nextFloat() * math::TWO_PI;
            const f32 pitch = (rng.nextFloat() - 0.5f) / 4.0f;

            // MC: float f2 = this.getThickness(random);
            const f32 thickness = getThickness(rng);

            // MC: int i1 = i - random.nextInt(i / 4);
            const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

            _createTunnel(chunk,
                context,
                biomeSource,
                0,
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
    // MC 1.21.11: float f = random.nextFloat() * 2.0F + random.nextFloat();
    f32 thickness = rng.nextFloat() * 2.0f + rng.nextFloat();

    // MC: if (random.nextInt(10) == 0) { f *= random.nextFloat() * random.nextFloat() * 3.0F + 1.0F; }
    if (rng.nextInt(10) == 0) {
        thickness *= rng.nextFloat() * rng.nextFloat() * 3.0f + 1.0f;
    }

    return thickness;
}

void CaveCarver::_createTunnel(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::BiomeSource& biomeSource,
    i32 /*seaLevel*/,
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

    // MC: int i = randomsource.nextInt(endIndex / 2) + endIndex / 4;
    const i32 branchPoint = endIndex / 4 + rng.nextInt(endIndex / 2);
    const bool canBranch = rng.nextInt(6) == 0;

    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;
    f64 currentX = startX;
    f64 currentY = startY;
    f64 currentZ = startZ;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // MC: double d0 = 1.5 + Mth.sin((float) Math.PI * j / endIndex) * thickness;
        //     double d1 = d0 * yScale;
        const f64 horizontalRadius = 1.5 + std::sin(static_cast<f64>(math::PI * i) / endIndex) * thickness;
        const f64 verticalRadius = horizontalRadius * yScale;

        // MC: float f2 = Mth.cos(pitch);
        const f32 cosPitch = std::cos(pitch);

        // MC: currentX += Mth.cos(yaw) * f2;
        //     currentY += Mth.sin(pitch);
        //     currentZ += Mth.sin(yaw) * f2;
        currentX += std::cos(static_cast<f64>(yaw)) * cosPitch;
        currentY += std::sin(static_cast<f64>(pitch));
        currentZ += std::sin(static_cast<f64>(yaw)) * cosPitch;

        // MC: pitch *= flag ? 0.92F : 0.7F;
        //     pitch += pitchModifier * 0.1F;
        //     yaw += yawModifier * 0.1F;
        pitch *= (canBranch ? 0.92f : 0.7f);
        pitch += pitchModifier * 0.1f;
        yaw += yawModifier * 0.1f;

        // MC: pitchModifier *= 0.9F;
        //     yawModifier *= 0.75F;
        //     pitchModifier += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 2.0F;
        //     yawModifier += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 4.0F;
        pitchModifier *= 0.9f;
        yawModifier *= 0.75f;
        pitchModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        // MC: if (j == i && thickness > 1.0F)
        if (i == branchPoint && thickness > 1.0f) {
            // MC: 分支递归调用 createTunnel
            const f32 branchThickness = rng.nextFloat() * 0.5f + 0.5f;

            _createTunnel(chunk,
                context,
                biomeSource,
                0,
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
                0,
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

        // MC: if (random.nextInt(4) != 0)
        if (rng.nextInt(4) != 0) {
            // MC: if (!canReach(chunk.getPos(), currentX, currentZ, j, endIndex, thickness))
            if (!isInCarvingRange(targetChunkX,
                    targetChunkZ,
                    static_cast<f32>(currentX),
                    static_cast<f32>(currentZ),
                    i,
                    endIndex,
                    thickness)) {
                return;
            }

            // MC: carveEllipsoid(context, config, chunk, biomeSource, aquifer,
            //     currentX, currentY, currentZ,
            //     d0 * horizontalRadiusMultiplier, d1 * verticalRadiusMultiplier,
            //     carvingMask, skipChecker);
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
    const world::biome::BiomeSource& biomeSource,
    i32 /*seaLevel*/,
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
    // MC: double d0 = 1.5 + Mth.sin((float) (Math.PI / 2)) * radius;
    //     double d1 = d0 * yScale;
    const f64 horizontalRadius = 1.5 + std::sin(math::HALF_PI) * radius;
    const f64 verticalRadius = horizontalRadius * yScale;

    // MC: carveEllipsoid(context, config, chunk, biomeSource, aquifer,
    //     centerX + 1.0, centerY, centerZ, d0, d1, carvingMask, skipChecker);
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
