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
#include "../../../core/Constants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../block/BlockRegistry.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// CaveCarver 实现
// ============================================================================

CaveCarver::CaveCarver(i32 maxHeight)
    : WorldCarver<ProbabilityConfig>(maxHeight)
{}

bool CaveCarver::shouldCarve(
    math::IRandom& rng, ChunkCoord /*chunkX*/, ChunkCoord /*chunkZ*/, const ProbabilityConfig& config) const noexcept
{
    return rng.nextFloat() <= config.probability;
}

bool CaveCarver::carve(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::BiomeSource& biomeSource,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    CarvingMask& carvingMask,
    const ProbabilityConfig& config)
{
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL +
        static_cast<u64>(m_maxHeight));

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    // 隧道长度范围
    const i32 tunnelLength = (getRange() * 2 - 1) * world::CHUNK_WIDTH;

    // 确定洞穴数量
    const i32 numCaves = rng.nextInt(rng.nextInt(rng.nextInt(getMaxCaveCount()) + 1) + 1);

    bool carved = false;
    const i32 startX = chunkX << world::CHUNK_SHIFT;
    const i32 startZ = chunkZ << world::CHUNK_SHIFT;

    for (i32 i = 0; i < numCaves; ++i) {
        // 随机起始位置
        const f32 startXPos = static_cast<f32>(startX) + rng.nextFloat(0.0f, static_cast<f32>(world::CHUNK_WIDTH));
        const f32 startZPos = static_cast<f32>(startZ) + rng.nextFloat(0.0f, static_cast<f32>(world::CHUNK_WIDTH));
        const f32 startYPos = static_cast<f32>(getCaveStartY(rng));

        // 有概率生成大型圆形房间
        i32 numTunnels = 1;

        if (rng.nextInt(4) == 0) {
            // 生成房间
            const f32 roomRadius = rng.nextFloat(1.0f, 7.0f);
            _carveRoom(chunk,
                context,
                biomeSource,
                seaLevel,
                chunkX,
                chunkZ,
                static_cast<i64>(rng.nextU64()),
                startXPos,
                startYPos,
                startZPos,
                roomRadius,
                0.5f,
                carvingMask);
            numTunnels += rng.nextInt(4);
        }

        // 生成隧道
        for (i32 tunnelIdx = 0; tunnelIdx < numTunnels; ++tunnelIdx) {
            // 随机方向
            const f32 yaw = rng.nextFloat(0.0f, math::TWO_PI);
            const f32 pitch = rng.nextFloat(-0.25f, 0.25f);
            const f32 radius = getCaveRadius(rng);

            // 隧道长度
            const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

            _carveTunnel(chunk,
                context,
                biomeSource,
                seaLevel,
                chunkX,
                chunkZ,
                static_cast<i64>(rng.nextU64()),
                startXPos,
                startYPos,
                startZPos,
                radius,
                yaw,
                pitch,
                0,
                length,
                getVerticalScale(),
                carvingMask);
        }

        carved = true;
    }

    return carved;
}

bool CaveCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 /*y*/) const noexcept
{
    // 椭球边界检测：dy <= -0.7 表示椭球底部，跳过以避免穿透地面
    // dx² + dy² + dz² >= 1.0 表示超出椭球范围
    return dy <= -0.7f || dx * dx + dy * dy + dz * dz >= 1.0f;
}

i32 CaveCarver::getCaveStartY(math::IRandom& rng) const
{
    // 洞穴起始Y坐标范围：8 到 128（通过嵌套随机实现）
    // 内层：rng.nextInt(121) 生成 0-120
    // 外层：+8 后范围变为 8-128
    // 这决定了洞穴主要生成在地下较深处
    return rng.nextInt(rng.nextInt(121) + 8);
}

f32 CaveCarver::getCaveRadius(math::IRandom& rng) const
{
    // 基础半径：0.0 - 2.0 范围的随机值
    f32 radius = rng.nextFloat() * 2.0f + rng.nextFloat();

    // 有 10% 概率生成大型洞穴
    if (rng.nextInt(10) == 0) {
        radius *= rng.nextFloat() * rng.nextFloat() * 3.0f + 1.0f;
    }

    return radius;
}

void CaveCarver::_carveTunnel(ChunkPrimer& chunk,
    CarvingContext& context,
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
    f32 verticalScale,
    CarvingMask& carvingMask)
{
    math::Random rng(static_cast<u64>(seed));

    // MC: branchPoint = nextInt(endIndex / 2) + endIndex / 4
    const i32 branchPoint = endIndex / 4 + rng.nextInt(endIndex / 2);
    const bool canBranch = rng.nextInt(6) == 0;

    f32 currentYaw = yaw;
    f32 currentPitch = pitch;
    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;
    f32 currentX = startX;
    f32 currentY = startY;
    f32 currentZ = startZ;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // MC: d0 = 1.5 + sin(PI * j / maxSteps) * thickness
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIndex);
        const f32 horizontalRadius = 1.5f + radius * std::sin(progress * math::PI);
        const f32 vertRadius = horizontalRadius * verticalScale;

        // 更新位置
        const f32 cosPitch = std::cos(currentPitch);
        currentX += std::cos(currentYaw) * cosPitch;
        currentY += std::sin(currentPitch);
        currentZ += std::sin(currentYaw) * cosPitch;

        // 更新角度
        currentPitch *= (canBranch ? 0.92f : 0.7f);
        currentPitch += pitchModifier * 0.1f;
        currentYaw += yawModifier * 0.1f;

        // 衰减和随机扰动
        pitchModifier *= 0.9f;
        yawModifier *= 0.75f;
        pitchModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        // 在分支点生成分支
        if (i == branchPoint && radius > 1.0f) {
            // 生成两个分支隧道
            const f32 branchRadius = radius * rng.nextFloat(0.5f, 1.0f);

            _carveTunnel(chunk,
                context,
                biomeSource,
                seaLevel,
                chunkX,
                chunkZ,
                static_cast<i64>(rng.nextU64()),
                currentX,
                currentY,
                currentZ,
                branchRadius,
                currentYaw - math::HALF_PI,
                currentPitch / 3.0f,
                i,
                endIndex,
                1.0f,
                carvingMask);

            _carveTunnel(chunk,
                context,
                biomeSource,
                seaLevel,
                chunkX,
                chunkZ,
                static_cast<i64>(rng.nextU64()),
                currentX,
                currentY,
                currentZ,
                branchRadius,
                currentYaw + math::HALF_PI,
                currentPitch / 3.0f,
                i,
                endIndex,
                1.0f,
                carvingMask);
            return;
        }

        // MC: 随机跳过一些点（25% 概率跳过，75% 概率雕刻）
        if (rng.nextInt(4) != 0) {
            // MC: canReach 失败时终止整个隧道
            if (!isInCarvingRange(chunkX, chunkZ, currentX, currentZ, i, endIndex, radius)) {
                return;
            }

            carveEllipsoid(chunk,
                context,
                biomeSource,
                seaLevel,
                chunkX,
                chunkZ,
                currentX,
                currentY,
                currentZ,
                horizontalRadius,
                vertRadius,
                carvingMask,
                static_cast<i64>(rng.nextU64()));
        }
    }
}

void CaveCarver::_carveRoom(ChunkPrimer& chunk,
    CarvingContext& context,
    const world::biome::BiomeSource& biomeSource,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    i64 seed,
    f32 centerX,
    f32 centerY,
    f32 centerZ,
    f32 radius,
    f32 verticalScale,
    CarvingMask& carvingMask)
{
    // 生成一个椭圆形房间
    const f32 horizontalRadius = 1.5f + std::sin(math::HALF_PI) * radius;
    const f32 vertRadius = horizontalRadius * verticalScale;

    carveEllipsoid(chunk,
        context,
        biomeSource,
        seaLevel,
        chunkX,
        chunkZ,
        centerX + 1.0f,
        centerY,
        centerZ,
        horizontalRadius,
        vertRadius,
        carvingMask,
        seed);
}

} // namespace mc
