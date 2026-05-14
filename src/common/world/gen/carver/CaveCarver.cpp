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
    math::IRandom& rng, ChunkCoord /*chunkX*/, ChunkCoord /*chunkZ*/, const ProbabilityConfig& config) const
{
    return rng.nextFloat() <= config.probability;
}

bool CaveCarver::carve(ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    CarvingMask& carvingMask,
    const ProbabilityConfig& config)
{
    // 参考 MC CaveWorldCarver.carveRegion
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL +
        static_cast<u64>(m_maxHeight));

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    // 隧道长度范围
    const i32 tunnelLength = (getRange() * 2 - 1) * 16;

    // 确定洞穴数量
    // 参考 MC: int j = rand.nextInt(rand.nextInt(rand.nextInt(this.func_230357_a_()) + 1) + 1);
    const i32 numCaves = rng.nextInt(rng.nextInt(rng.nextInt(getMaxCaveCount()) + 1) + 1);

    bool carved = false;
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    for (i32 i = 0; i < numCaves; ++i) {
        // 随机起始位置
        const f32 startXPos = static_cast<f32>(startX) + rng.nextFloat(0.0f, 16.0f);
        const f32 startZPos = static_cast<f32>(startZ) + rng.nextFloat(0.0f, 16.0f);
        const f32 startYPos = static_cast<f32>(getCaveStartY(rng));

        // 有概率生成大型圆形房间
        i32 numTunnels = 1;

        if (rng.nextInt(4) == 0) {
            // 生成房间
            const f32 roomRadius = rng.nextFloat(1.0f, 7.0f);
            carveRoom(chunk,
                biomeProvider,
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
            numTunnels += rng.nextInt(5);
        }

        // 生成隧道
        for (i32 tunnelIdx = 0; tunnelIdx < numTunnels; ++tunnelIdx) {
            // 随机方向
            const f32 yaw = rng.nextFloat(0.0f, math::TWO_PI);
            const f32 pitch = rng.nextFloat(-0.25f, 0.25f);
            const f32 radius = getCaveRadius(rng);

            // 隧道长度
            const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

            carveTunnel(chunk,
                biomeProvider,
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

bool CaveCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 参考 MC CaveWorldCarver.func_222708_a_
    // return p_222708_3_ <= -0.7D || dx * dx + dy * dy + dz * dz >= 1.0D;
    // 其中 p_222708_3_ 是 dy
    (void)y; // MC 原版不使用 y 坐标，只使用 dy
    return dy <= -0.7f || dx * dx + dy * dy + dz * dz >= 1.0f;
}

i32 CaveCarver::getCaveStartY(math::IRandom& rng) const
{
    // 参考 MC: return rand.nextInt(rand.nextInt(120) + 8);
    return rng.nextInt(rng.nextInt(121) + 8);
}

f32 CaveCarver::getCaveRadius(math::IRandom& rng) const
{
    // 参考 MC CaveWorldCarver.func_230359_a_
    // float f = rand.nextFloat() * 2.0F + rand.nextFloat();
    // if (rand.nextInt(10) == 0) { f *= rand.nextFloat() * rand.nextFloat() * 3.0F + 1.0F; }
    f32 radius = rng.nextFloat() * 2.0f + rng.nextFloat();

    if (rng.nextInt(10) == 0) {
        radius *= rng.nextFloat() * rng.nextFloat() * 3.0f + 1.0f;
    }

    return radius;
}

void CaveCarver::carveTunnel(ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
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
    // 参考 MC CaveWorldCarver.func_227206_a_
    math::Random rng(static_cast<u64>(seed));

    // 分支点
    const i32 branchPoint = endIndex / 4 + static_cast<i32>(rng.nextInt(endIndex / 2 + 1));
    const bool canBranch = rng.nextInt(6) == 0;

    f32 currentYaw = yaw;
    f32 currentPitch = pitch;
    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;
    f32 currentX = startX;
    f32 currentY = startY;
    f32 currentZ = startZ;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // 椭球半径随距离变化（正弦曲线）
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIndex);
        const f32 sinProgress = std::sin(progress * math::PI);
        const f32 horizontalRadius = radius * sinProgress;
        const f32 vertRadius = horizontalRadius * verticalScale;

        // 更新位置（参考MC的方向计算）
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

            carveTunnel(chunk,
                biomeProvider,
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

            carveTunnel(chunk,
                biomeProvider,
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

        // 随机跳过一些点（增加不规则性）
        if (rng.nextInt(4) == 0) {
            continue;
        }

        // 检查是否在雕刻范围内
        if (isInCarvingRange(chunkX, chunkZ, currentX, currentZ, i, endIndex, radius)) {
            carveEllipsoid(chunk,
                biomeProvider,
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

void CaveCarver::carveRoom(ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
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
    // 参考 MC CaveWorldCarver.func_227205_a_
    // 生成一个椭圆形房间
    const f32 horizontalRadius = 1.5f + std::sin(math::HALF_PI) * radius;
    const f32 vertRadius = horizontalRadius * verticalScale;

    carveEllipsoid(chunk,
        biomeProvider,
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
