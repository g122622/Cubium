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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "NoiseBasedAquifer.hpp"
#include "FluidStatus.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace mc::world::gen::aquifer {

NoiseBasedAquifer::NoiseBasedAquifer(const density::NoiseChunk& noiseChunk,
    i32 chunkX,
    i32 chunkZ,
    const density::NoiseRouter& router,
    const math::PositionalRandomFactory& positionalRandom,
    i32 minY,
    i32 height,
    FluidPicker globalFluidPicker)
    : m_noiseChunk(noiseChunk)
    , m_barrierNoise(router.barrierNoise())
    , m_fluidLevelFloodednessNoise(router.fluidLevelFloodednessNoise())
    , m_fluidLevelSpreadNoise(router.fluidLevelSpreadNoise())
    , m_lavaNoise(router.lavaNoise())
    , m_erosion(router.erosion())
    , m_depth(router.depth())
    , m_positionalRandom(positionalRandom)
    , m_globalFluidPicker(std::move(globalFluidPicker))
{
    const i32 minBlockX = chunkX * world::CHUNK_WIDTH;
    const i32 minBlockZ = chunkZ * world::CHUNK_WIDTH;
    const i32 maxBlockX = minBlockX + world::CHUNK_WIDTH - 1;
    const i32 maxBlockZ = minBlockZ + world::CHUNK_WIDTH - 1;

    m_minGridX = gridX(minBlockX + SAMPLE_OFFSET_X);
    m_minGridY = gridY(minY + SAMPLE_OFFSET_Y) - 1;
    m_minGridZ = gridZ(minBlockZ + SAMPLE_OFFSET_Z);

    const i32 maxGridX = gridX(maxBlockX + SAMPLE_OFFSET_X) + 1;
    const i32 maxGridY = gridY(minY + height + SAMPLE_OFFSET_Y) + 1;
    const i32 maxGridZ = gridZ(maxBlockZ + SAMPLE_OFFSET_Z) + 1;

    m_gridSizeX = maxGridX - m_minGridX + 1;
    m_gridSizeY = maxGridY - m_minGridY + 1;
    m_gridSizeZ = maxGridZ - m_minGridZ + 1;

    const i32 totalSize = m_gridSizeX * m_gridSizeY * m_gridSizeZ;
    m_aquiferLocationCache.resize(totalSize, std::numeric_limits<i64>::max());
    m_aquiferStatusCache.resize(totalSize);
    m_aquiferStatusComputed.resize(totalSize, false);

    const i32 maxSurfaceLevel = m_noiseChunk.maxPreliminarySurfaceLevel(fromGridX(m_minGridX, 0),
                                    fromGridZ(m_minGridZ, 0),
                                    fromGridX(maxGridX, X_RANGE - 1),
                                    fromGridZ(maxGridZ, Z_RANGE - 1)) +
        8;
    const i32 skipGridY = gridY(maxSurfaceLevel + 12) + 1;
    m_skipSamplingAboveY = fromGridY(skipGridY, Y_RANGE + 2) - 1;
}

const BlockState* NoiseBasedAquifer::computeSubstance(i32 blockX, i32 blockY, i32 blockZ, f64 densityValue)
{
    m_shouldScheduleFluidUpdate = false;

    // 密度 > 0 时为固体，不产生流体
    if (densityValue > 0.0) {
        return nullptr;
    }

    // MC 1.21: 在 computeSubstance 开头计算 barrierNoise 一次，供 calculatePressure 复用
    const f64 barrierNoise = m_barrierNoise.compute(blockX, blockY, blockZ);

    // 获取全局流体
    FluidStatus globalFluid = m_globalFluidPicker(blockX, blockY, blockZ);

    // 在地表以上，直接返回全局流体
    if (blockY > m_skipSamplingAboveY) {
        return globalFluid.at(blockY);
    }

    // 如果当前位置是熔岩区域，直接返回熔岩
    const BlockState* globalBlock = globalFluid.at(blockY);
    if (globalBlock != nullptr && globalBlock->isLiquid() && &globalBlock->getBlock() == VanillaBlocks::LAVA) {
        return globalBlock;
    }

    // 计算含水层网格坐标
    const i32 aquiferGridX = gridX(blockX + SAMPLE_OFFSET_X);
    const i32 aquiferGridY = gridY(blockY + SAMPLE_OFFSET_Y);
    const i32 aquiferGridZ = gridZ(blockZ + SAMPLE_OFFSET_Z);

    // MC 1.21: 搜索最近的 4 个含水层中心（2x3x2 = 12 格网格）
    // k1/l1/i2/j2: 4个最近距离（平方），k2/l2/i3/j3: 对应的网格索引
    i32 distSq1 = std::numeric_limits<i32>::max(); // 1st nearest (smallest)
    i32 distSq2 = std::numeric_limits<i32>::max(); // 2nd nearest
    i32 distSq3 = std::numeric_limits<i32>::max(); // 3rd nearest
    i32 distSq4 = std::numeric_limits<i32>::max(); // 4th nearest
    i32 gridIdx1X = 0, gridIdx1Y = 0, gridIdx1Z = 0;
    i32 gridIdx2X = 0, gridIdx2Y = 0, gridIdx2Z = 0;
    i32 gridIdx3X = 0, gridIdx3Y = 0, gridIdx3Z = 0;
    i32 gridIdx4X = 0, gridIdx4Y = 0, gridIdx4Z = 0;

    for (i32 dx = 0; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = 0; dz <= 1; ++dz) {
                const i32 gx = aquiferGridX + dx;
                const i32 gy = aquiferGridY + dy;
                const i32 gz = aquiferGridZ + dz;

                const i64 pos = getAquiferLocation(gx, gy, gz);
                const i32 ax = decodeBlockPosX(pos);
                const i32 ay = decodeBlockPosY(pos);
                const i32 az = decodeBlockPosZ(pos);

                const i32 ddx = blockX - ax;
                const i32 ddy = blockY - ay;
                const i32 ddz = blockZ - az;
                const i32 dSq = ddx * ddx + ddy * ddy + ddz * ddz;

                // 级联插入排序：保持 4 个最小距离
                if (dSq < distSq4) {
                    if (dSq < distSq1) {
                        distSq4 = distSq3;
                        gridIdx4X = gridIdx3X;
                        gridIdx4Y = gridIdx3Y;
                        gridIdx4Z = gridIdx3Z;
                        distSq3 = distSq2;
                        gridIdx3X = gridIdx2X;
                        gridIdx3Y = gridIdx2Y;
                        gridIdx3Z = gridIdx2Z;
                        distSq2 = distSq1;
                        gridIdx2X = gridIdx1X;
                        gridIdx2Y = gridIdx1Y;
                        gridIdx2Z = gridIdx1Z;
                        distSq1 = dSq;
                        gridIdx1X = gx;
                        gridIdx1Y = gy;
                        gridIdx1Z = gz;
                    } else if (dSq < distSq2) {
                        distSq4 = distSq3;
                        gridIdx4X = gridIdx3X;
                        gridIdx4Y = gridIdx3Y;
                        gridIdx4Z = gridIdx3Z;
                        distSq3 = distSq2;
                        gridIdx3X = gridIdx2X;
                        gridIdx3Y = gridIdx2Y;
                        gridIdx3Z = gridIdx2Z;
                        distSq2 = dSq;
                        gridIdx2X = gx;
                        gridIdx2Y = gy;
                        gridIdx2Z = gz;
                    } else if (dSq < distSq3) {
                        distSq4 = distSq3;
                        gridIdx4X = gridIdx3X;
                        gridIdx4Y = gridIdx3Y;
                        gridIdx4Z = gridIdx3Z;
                        distSq3 = dSq;
                        gridIdx3X = gx;
                        gridIdx3Y = gy;
                        gridIdx3Z = gz;
                    } else {
                        distSq4 = dSq;
                        gridIdx4X = gx;
                        gridIdx4Y = gy;
                        gridIdx4Z = gz;
                    }
                }
            }
        }
    }

    // 获取最近含水层的状态
    AquiferStatus aquifer1 = getAquiferStatus(gridIdx1X, gridIdx1Y, gridIdx1Z);

    // 计算 1st-2nd 相似度
    const f64 d1 = similarity(distSq1, distSq2);

    // MC 1.21: d1 <= 0 时，完全在含水层内部
    if (d1 <= 0.0) {
        // MC 1.21: 检查流体更新调度
        if (d1 >= FLOWING_UPDATE_SIMULARITY) {
            AquiferStatus aquifer2 = getAquiferStatus(gridIdx2X, gridIdx2Y, gridIdx2Z);
            if (aquifer1 != aquifer2) {
                m_shouldScheduleFluidUpdate = true;
            }
        }
        if (blockY < aquifer1.fluidLevel) {
            return aquifer1.fluidType;
        }
        // MC 1.21: 海平面以上返回空气 BlockState
        return VanillaBlocks::getState(VanillaBlocks::AIR);
    }

    // MC 1.21: 水在熔岩上方时触发流体更新
    // Java: blockstate.is(Blocks.WATER) && globalFluidPicker.computeFluid(x, y-1, z).at(y-1).is(Blocks.LAVA)
    AquiferStatus aquifer2 = getAquiferStatus(gridIdx2X, gridIdx2Y, gridIdx2Z);
    {
        const BlockState* blockAtY =
            (aquifer1.fluidType != nullptr && blockY < aquifer1.fluidLevel) ? aquifer1.fluidType : nullptr;
        if (blockAtY != nullptr && &blockAtY->getBlock() == VanillaBlocks::WATER) {
            // 检查全局流体选择器在下方位置的熔岩状态
            FluidStatus globalBelow = m_globalFluidPicker(blockX, blockY - 1, blockZ);
            const BlockState* belowFluid = globalBelow.at(blockY - 1);
            if (belowFluid != nullptr && &belowFluid->getBlock() == VanillaBlocks::LAVA) {
                m_shouldScheduleFluidUpdate = true;
                return blockAtY;
            }
        }
    }

    // MC 1.21: 三阶段压力计算
    // 第一阶段: 1st 和 2nd 最近点的压力
    const f64 pressure1 = d1 * calculatePressure(blockX, blockY, blockZ, aquifer1, aquifer2, barrierNoise);
    if (densityValue + pressure1 > 0.0) {
        return nullptr; // 压力使该位置保持固体
    }

    // 第二阶段: 1st 和 3rd 最近点的压力
    AquiferStatus aquifer3 = getAquiferStatus(gridIdx3X, gridIdx3Y, gridIdx3Z);
    const f64 d0 = similarity(distSq1, distSq3); // 1st vs 3rd
    if (d0 > 0.0) {
        const f64 pressure2 = d1 * d0 * calculatePressure(blockX, blockY, blockZ, aquifer1, aquifer3, barrierNoise);
        if (densityValue + pressure2 > 0.0) {
            return nullptr;
        }
    }

    // 第三阶段: 2nd 和 3rd 最近点的压力
    const f64 d4 = similarity(distSq2, distSq3); // 2nd vs 3rd
    if (d4 > 0.0) {
        const f64 pressure3 = d1 * d4 * calculatePressure(blockX, blockY, blockZ, aquifer2, aquifer3, barrierNoise);
        if (densityValue + pressure3 > 0.0) {
            return nullptr;
        }
    }

    // MC 1.21: 多条件流体更新调度
    const bool flag2 = (aquifer1 != aquifer2);                                      // 1st != 2nd
    const bool flag = (d4 >= FLOWING_UPDATE_SIMULARITY) && (aquifer2 != aquifer3);  // 2nd-3rd close and different
    const bool flag1 = (d0 >= FLOWING_UPDATE_SIMULARITY) && (aquifer1 != aquifer3); // 1st-3rd close and different

    if (!flag2 && !flag && !flag1) {
        // 只有当前三个含水层状态一致时，才检查第4个
        AquiferStatus aquifer4 = getAquiferStatus(gridIdx4X, gridIdx4Y, gridIdx4Z);
        const f64 d5 = similarity(distSq1, distSq4); // 1st vs 4th
        m_shouldScheduleFluidUpdate =
            (d0 >= FLOWING_UPDATE_SIMULARITY) && (d5 >= FLOWING_UPDATE_SIMULARITY) && (aquifer1 != aquifer4);
    } else {
        m_shouldScheduleFluidUpdate = true;
    }

    // 返回含水层流体
    if (blockY < aquifer1.fluidLevel) {
        return aquifer1.fluidType;
    }
    return nullptr;
}

NoiseBasedAquifer::AquiferStatus NoiseBasedAquifer::getAquiferStatus(i32 gridX, i32 gridY, i32 gridZ)
{
    const i32 idx =
        (gridY - m_minGridY) * m_gridSizeZ * m_gridSizeX + (gridZ - m_minGridZ) * m_gridSizeX + (gridX - m_minGridX);

    if (idx < 0 || idx >= static_cast<i32>(m_aquiferStatusComputed.size())) {
        return {WAY_BELOW_MIN_Y, nullptr};
    }

    if (m_aquiferStatusComputed[idx]) {
        return m_aquiferStatusCache[idx];
    }

    // 获取含水层中心位置并计算状态
    const i64 pos = getAquiferLocation(gridX, gridY, gridZ);
    const i32 ax = decodeBlockPosX(pos);
    const i32 ay = decodeBlockPosY(pos);
    const i32 az = decodeBlockPosZ(pos);

    AquiferStatus status = computeFluid(ax, ay, az);
    m_aquiferStatusCache[idx] = status;
    m_aquiferStatusComputed[idx] = true;
    return status;
}

i64 NoiseBasedAquifer::getAquiferLocation(i32 gridX, i32 gridY, i32 gridZ)
{
    const i32 idx =
        (gridY - m_minGridY) * m_gridSizeZ * m_gridSizeX + (gridZ - m_minGridZ) * m_gridSizeX + (gridX - m_minGridX);

    if (idx < 0 || idx >= static_cast<i32>(m_aquiferLocationCache.size())) {
        return encodeBlockPos(
            fromGridX(gridX, X_RANGE / 2), fromGridY(gridY, Y_RANGE / 2), fromGridZ(gridZ, Z_RANGE / 2));
    }

    if (m_aquiferLocationCache[idx] != std::numeric_limits<i64>::max()) {
        return m_aquiferLocationCache[idx];
    }

    // 使用位置随机生成含水层中心
    auto rng = m_positionalRandom.at(gridX, gridY, gridZ);
    const i32 offsetX = rng->nextInt(X_RANGE); // 0 ~ 9
    const i32 offsetY = rng->nextInt(Y_RANGE); // 0 ~ 8
    const i32 offsetZ = rng->nextInt(Z_RANGE); // 0 ~ 9

    const i64 pos = encodeBlockPos(fromGridX(gridX, offsetX), fromGridY(gridY, offsetY), fromGridZ(gridZ, offsetZ));

    m_aquiferLocationCache[idx] = pos;
    return pos;
}

NoiseBasedAquifer::AquiferStatus NoiseBasedAquifer::computeFluid(i32 x, i32 y, i32 z)
{
    FluidStatus globalFluid = m_globalFluidPicker(x, y, z);

    // 采样周围区块的地表高度
    i32 minSurface = std::numeric_limits<i32>::max();
    const i32 yPlus12 = y + 12;
    const i32 yMinus12 = y - 12;
    bool centerHasFluid = false;

    for (const auto& offset : SURFACE_SAMPLING_OFFSETS) {
        const i32 sx = x + offset[0] * 16;
        const i32 sz = z + offset[1] * 16;

        const i32 surfaceLevel = m_noiseChunk.samplePreliminarySurfaceLevel(sx, sz);
        const i32 adjustedSurface = surfaceLevel + 8;

        const bool isCenter = (offset[0] == 0 && offset[1] == 0);
        if (isCenter && yMinus12 > adjustedSurface) {
            return {globalFluid.fluidLevel, globalFluid.fluidType};
        }

        const bool isAboveSurface = yPlus12 > adjustedSurface;
        if (isAboveSurface || isCenter) {
            FluidStatus surfaceFluid = m_globalFluidPicker(sx, adjustedSurface, sz);
            // MC 1.21: !aquifer$fluidstatus1.at(k1).isAir() — 检查该处是否有流体
            if (surfaceFluid.at(adjustedSurface) != nullptr) {
                if (isCenter) {
                    centerHasFluid = true;
                }
                if (isAboveSurface) {
                    return {surfaceFluid.fluidLevel, surfaceFluid.fluidType};
                }
            }
        }

        minSurface = std::min(minSurface, surfaceLevel);
    }

    const i32 fluidLevel = computeSurfaceLevel(x, y, z, globalFluid, minSurface, centerHasFluid);
    const BlockState* fluidType = computeFluidType(x, y, z, globalFluid, fluidLevel);

    return {fluidLevel, fluidType};
}

i32 NoiseBasedAquifer::computeSurfaceLevel(
    i32 x, i32 y, i32 z, const FluidStatus& globalFluid, i32 minSurface, bool centerHasFluid)
{
    // 检查深暗之域区域
    const f64 erosionValue = m_erosion.compute(x, y, z);
    const f64 depthValue = m_depth.compute(x, y, z);
    if (erosionValue < -0.225 && depthValue > 0.9) {
        return WAY_BELOW_MIN_Y;
    }

    const i32 distFromSurface = minSurface + 8 - y;
    const f64 floodChance =
        centerHasFluid ? math::clamp(math::map(static_cast<f64>(distFromSurface), 0.0, 64.0, 1.0, 0.0), 0.0, 1.0) : 0.0;

    const f64 floodedness = math::clamp(m_fluidLevelFloodednessNoise.compute(x, y, z), -1.0, 1.0);

    const f64 floodThreshold = math::map(floodChance, 1.0, 0.0, -0.3, 0.8);
    const f64 levelThreshold = math::map(floodChance, 1.0, 0.0, -0.8, 0.4);

    const f64 floodValue = floodedness - levelThreshold;
    const f64 levelValue = floodedness - floodThreshold;

    if (levelValue > 0.0) {
        return globalFluid.fluidLevel;
    }
    if (floodValue > 0.0) {
        return computeRandomizedFluidSurfaceLevel(x, y, z, minSurface);
    }
    return WAY_BELOW_MIN_Y;
}

i32 NoiseBasedAquifer::computeRandomizedFluidSurfaceLevel(i32 x, i32 y, i32 z, i32 minSurface)
{
    // MC 1.21: 使用网格坐标采样噪声，量化到 3 的倍数
    const i32 k = math::floorDiv(x, 16);
    const i32 l = math::floorDiv(y, 40);
    const i32 i1 = math::floorDiv(z, 16);

    const i32 baseY = l * 40 + 20;
    const f64 spreadNoise = m_fluidLevelSpreadNoise.compute(k, l, i1) * 10.0;
    const i32 quantized = static_cast<i32>(std::floor(spreadNoise / 3.0)) * 3;
    const i32 fluidY = baseY + quantized;
    return std::min(minSurface, fluidY);
}

const BlockState* NoiseBasedAquifer::computeFluidType(i32 x, i32 y, i32 z, const FluidStatus& globalFluid, i32 level)
{
    // 熔岩只在深层出现（Y <= -10）
    if (level <= -10 && level != WAY_BELOW_MIN_Y) {
        // 检查全局流体不是熔岩
        if (globalFluid.fluidType != nullptr && &globalFluid.fluidType->getBlock() != VanillaBlocks::LAVA) {
            const i32 lavaGridX = math::floorDiv(x, 64);
            const i32 lavaGridY = math::floorDiv(y, 40);
            const i32 lavaGridZ = math::floorDiv(z, 64);
            const f64 lavaNoiseValue = m_lavaNoise.compute(lavaGridX, lavaGridY, lavaGridZ);

            if (std::abs(lavaNoiseValue) > 0.3) {
                return &VanillaBlocks::LAVA->defaultState();
            }
        }
    }
    return globalFluid.fluidType;
}

f64 NoiseBasedAquifer::calculatePressure(
    i32 blockX, i32 blockY, i32 blockZ, const AquiferStatus& a, const AquiferStatus& b, f64 cachedBarrierNoise)
{
    const BlockState* stateA = (a.fluidType != nullptr && blockY < a.fluidLevel) ? a.fluidType : nullptr;
    const BlockState* stateB = (b.fluidType != nullptr && blockY < b.fluidLevel) ? b.fluidType : nullptr;

    // 水和熔岩接触：创建石质屏障
    const bool aIsLava = (stateA != nullptr && &stateA->getBlock() == VanillaBlocks::LAVA);
    const bool bIsLava = (stateB != nullptr && &stateB->getBlock() == VanillaBlocks::LAVA);
    const bool aIsWater = (stateA != nullptr && &stateA->getBlock() == VanillaBlocks::WATER);
    const bool bIsWater = (stateB != nullptr && &stateB->getBlock() == VanillaBlocks::WATER);

    if ((aIsLava && bIsWater) || (aIsWater && bIsLava)) {
        return 2.0;
    }

    const i32 levelDiff = std::abs(a.fluidLevel - b.fluidLevel);
    if (levelDiff == 0) {
        return 0.0;
    }

    const f64 avgLevel = 0.5 * static_cast<f64>(a.fluidLevel + b.fluidLevel);
    const f64 distFromAvg = static_cast<f64>(blockY) + 0.5 - avgLevel;
    const f64 halfDiff = static_cast<f64>(levelDiff) / 2.0;
    const f64 distFromBoundary = halfDiff - std::abs(distFromAvg);

    f64 pressure;
    if (distFromAvg > 0.0) {
        const f64 v = distFromBoundary;
        pressure = v > 0 ? v / 1.5 : v / 2.5;
    } else {
        const f64 v = 3.0 + distFromBoundary;
        pressure = v > 0 ? v / 3.0 : v / 10.0;
    }

    // MC 1.21: 只在屏障附近使用 barrierNoise（值已缓存）
    if (pressure >= -2.0 && pressure <= 2.0) {
        return 2.0 * (cachedBarrierNoise + pressure);
    }

    return 2.0 * pressure;
}

f64 NoiseBasedAquifer::similarity(i32 dist1Sq, i32 dist2Sq)
{
    return 1.0 - static_cast<f64>(dist2Sq - dist1Sq) / 25.0;
}

i64 NoiseBasedAquifer::encodeBlockPos(i32 x, i32 y, i32 z)
{
    return (static_cast<i64>(x) & 0x3FFFFFF) << 38 | (static_cast<i64>(y) & 0xFFF) << 26 |
        (static_cast<i64>(z) & 0x3FFFFFF);
}

} // namespace mc::world::gen::aquifer
