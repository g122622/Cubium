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
 */

#include "Aquifer.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mc::world::gen::aquifer {

// ============================================================================
// FluidStatus
// ============================================================================

const BlockState* FluidStatus::at(i32 y) const
{
    if (y < fluidLevel) {
        return fluidType;
    }
    return nullptr; // 空气
}

// ============================================================================
// DisabledAquifer
// ============================================================================

const BlockState* DisabledAquifer::computeSubstance(i32 blockX, i32 blockY, i32 blockZ, f64 densityValue)
{
    if (densityValue > 0.0) {
        return nullptr; // 固体方块
    }

    FluidStatus fluid = m_globalFluidPicker(blockX, blockY, blockZ);
    return fluid.at(blockY);
}

// ============================================================================
// 工厂方法
// ============================================================================

std::unique_ptr<Aquifer> Aquifer::createNoiseBased(const density::NoiseChunk& noiseChunk,
    i32 chunkX,
    i32 chunkZ,
    const density::NoiseRouter& router,
    const math::PositionalRandomFactory& positionalRandom,
    i32 minY,
    i32 height,
    FluidPicker globalFluidPicker)
{
    return std::make_unique<NoiseBasedAquifer>(
        noiseChunk, chunkX, chunkZ, router, positionalRandom, minY, height, std::move(globalFluidPicker));
}

std::unique_ptr<Aquifer> Aquifer::createDisabled(FluidPicker globalFluidPicker)
{
    return std::make_unique<DisabledAquifer>(std::move(globalFluidPicker));
}

// ============================================================================
// NoiseBasedAquifer
// ============================================================================

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
    , m_skipSamplingAboveY(minY + height - 8)
{
    const i32 startBlockX = chunkX * 16;
    const i32 startBlockZ = chunkZ * 16;

    // 采样起始位置（含水层网格偏移后）
    const i32 sampleStartX = startBlockX + SAMPLE_OFFSET_X * X_SPACING;
    const i32 sampleStartY = minY + SAMPLE_OFFSET_Y * Y_SPACING;
    const i32 sampleStartZ = startBlockZ + SAMPLE_OFFSET_Z * Z_SPACING;

    m_minGridX = gridX(sampleStartX) - 1;
    m_minGridY = gridY(sampleStartY) - 1;
    m_minGridZ = gridZ(sampleStartZ) - 1;

    const i32 maxGridX = gridX(startBlockX + 16 + SAMPLE_OFFSET_X * X_SPACING) + 1;
    const i32 maxGridY = gridY(minY + height + SAMPLE_OFFSET_Y * Y_SPACING) + 1;
    const i32 maxGridZ = gridZ(startBlockZ + 16 + SAMPLE_OFFSET_Z * Z_SPACING) + 1;

    m_gridSizeX = maxGridX - m_minGridX + 1;
    m_gridSizeY = maxGridY - m_minGridY + 1;
    m_gridSizeZ = maxGridZ - m_minGridZ + 1;

    const i32 totalSize = m_gridSizeX * m_gridSizeY * m_gridSizeZ;
    m_aquiferLocationCache.resize(totalSize, std::numeric_limits<i64>::max());
    m_aquiferStatusCache.resize(totalSize);
    m_aquiferStatusComputed.resize(totalSize, false);
}

const BlockState* NoiseBasedAquifer::computeSubstance(i32 blockX, i32 blockY, i32 blockZ, f64 densityValue)
{
    m_shouldScheduleFluidUpdate = false;

    // 密度 > 0 时为固体，不产生流体
    if (densityValue > 0.0) {
        return nullptr;
    }

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
    const i32 aquiferGridX = gridX(blockX + SAMPLE_OFFSET_X * X_SPACING);
    const i32 aquiferGridY = gridY(blockY + SAMPLE_OFFSET_Y * Y_SPACING);
    const i32 aquiferGridZ = gridZ(blockZ + SAMPLE_OFFSET_Z * Z_SPACING);

    // 搜索最近的含水层中心（2x3x2 网格范围）
    i32 distSq1 = std::numeric_limits<i32>::max();
    i32 distSq2 = std::numeric_limits<i32>::max();
    i32 nearestGridX = 0;
    i32 nearestGridY = 0;
    i32 nearestGridZ = 0;
    i32 secondNearestGridX = 0;
    i32 secondNearestGridY = 0;
    i32 secondNearestGridZ = 0;

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

                if (dSq < distSq1) {
                    distSq2 = distSq1;
                    secondNearestGridX = nearestGridX;
                    secondNearestGridY = nearestGridY;
                    secondNearestGridZ = nearestGridZ;
                    distSq1 = dSq;
                    nearestGridX = gx;
                    nearestGridY = gy;
                    nearestGridZ = gz;
                } else if (dSq < distSq2) {
                    distSq2 = dSq;
                    secondNearestGridX = gx;
                    secondNearestGridY = gy;
                    secondNearestGridZ = gz;
                }
            }
        }
    }

    // 获取最近含水层的状态
    AquiferStatus aquifer1 = getAquiferStatus(nearestGridX, nearestGridY, nearestGridZ);

    // 计算相似度
    const f64 sim = similarity(distSq1, distSq2);

    // 在含水层内部
    if (sim <= 0.0) {
        if (blockY < aquifer1.fluidLevel) {
            return aquifer1.fluidType;
        }
        return nullptr; // 空气
    }

    // 过渡区域：使用压力计算
    AquiferStatus aquifer2 = getAquiferStatus(secondNearestGridX, secondNearestGridY, secondNearestGridZ);

    const f64 pressure = sim * calculatePressure(blockX, blockY, blockZ, aquifer1, aquifer2);

    // 检查是否需要安排流体更新（MC 1.21: 比较 FluidStatus 的 fluidLevel 和 fluidType）
    if (sim > FLOWING_UPDATE_SIMULARITY && aquifer1 != aquifer2) {
        m_shouldScheduleFluidUpdate = true;
    }

    if (densityValue + pressure > 0.0) {
        return nullptr; // 压力使该位置保持固体
    }

    // 返回含水层流体
    if (blockY < aquifer1.fluidLevel) {
        return aquifer1.fluidType;
    }
    return nullptr;
}

NoiseBasedAquifer::AquiferStatus NoiseBasedAquifer::getAquiferStatus(i32 gridX, i32 gridY, i32 gridZ)
{
    const i32 idx = (gridX - m_minGridX) + m_gridSizeX * ((gridY - m_minGridY) + m_gridSizeY * (gridZ - m_minGridZ));

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
    const i32 idx = (gridX - m_minGridX) + m_gridSizeX * ((gridY - m_minGridY) + m_gridSizeY * (gridZ - m_minGridZ));

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

        const i32 surfaceLevel = static_cast<i32>(m_noiseChunk.samplePreliminarySurfaceLevel(sx, sz));
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
    i32 blockX, i32 blockY, i32 blockZ, const AquiferStatus& a, const AquiferStatus& b)
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

    // 只在屏障附近使用 barrierNoise
    if (pressure >= -2.0 && pressure <= 2.0) {
        const f64 barrierValue = m_barrierNoise.compute(blockX, blockY, blockZ);
        return 2.0 * (barrierValue + pressure);
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

// ============================================================================
// 流体选择器工厂
// ============================================================================

FluidPicker createOverworldFluidPicker(i32 seaLevel, const BlockState* defaultFluid)
{
    const i32 lavaLevel = -54;
    const BlockState* lavaState = &VanillaBlocks::LAVA->defaultState();
    const i32 minFluidLevel = std::min(lavaLevel, seaLevel);

    return [seaLevel, lavaLevel, minFluidLevel, defaultFluid, lavaState](i32, i32 y, i32) -> FluidStatus {
        if (y < minFluidLevel) {
            return {lavaLevel, lavaState};
        }
        return {seaLevel, defaultFluid};
    };
}

FluidPicker createNetherFluidPicker()
{
    const BlockState* lavaState = &VanillaBlocks::LAVA->defaultState();
    return [lavaState](i32, i32, i32) -> FluidStatus { return {32, lavaState}; };
}

FluidPicker createEndFluidPicker()
{
    return [](i32, i32, i32) -> FluidStatus { return {std::numeric_limits<i32>::min(), nullptr}; };
}

} // namespace mc::world::gen::aquifer
