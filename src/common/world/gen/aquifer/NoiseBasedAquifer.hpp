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

#pragma once

#include "Aquifer.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/gen/aquifer/FluidStatus.hpp"
#include <vector>

namespace mc {

// 前向声明
class BlockState;

namespace world::gen::density {
class DensityFunction;
} // namespace world::gen::density

namespace world::gen::aquifer {

/**
 * @brief 基于噪声的含水层（MC 1.21 Aquifer.NoiseBasedAquifer）
 *
 * 使用网格化的含水层中心和噪声函数确定地下水/熔岩分布。
 *
 * 核心算法：
 * 1. 将世界划分为 16×12×16 的网格（含水层网格）
 * 2. 每个网格单元随机生成一个含水层中心位置
 * 3. 对每个空腔方块，找到最近的 4 个含水层中心
 * 4. 根据距离相似度（similarity）和屏障噪声（barrier noise）判断：
 *    - similarity <= 0：完全在含水层内，返回流体
 *    - similarity > 0：过渡区域，使用压力计算
 * 5. 压力计算使用 barrierNoise、fluidLevelFloodednessNoise、fluidLevelSpreadNoise
 * 6. 熔岩使用 lavaNoise 在深层（Y <= -10）判断
 *
 * 网格参数：
 * - X_RANGE = 10, Y_RANGE = 9, Z_RANGE = 10（含水层中心偏移范围）
 * - X_SEPARATION = 6, Y_SEPARATION = 3, Z_SEPARATION = 6（最小间距）
 * - X_SPACING = 16, Y_SPACING = 12, Z_SPACING = 16（网格间距）
 * - SAMPLE_OFFSET_X = -5, SAMPLE_OFFSET_Y = 1, SAMPLE_OFFSET_Z = -5
 */
class NoiseBasedAquifer final : public Aquifer {
public:
    NoiseBasedAquifer(const density::NoiseChunk& noiseChunk,
        i32 chunkX,
        i32 chunkZ,
        const density::NoiseRouter& router,
        const math::PositionalRandomFactory& positionalRandom,
        i32 minY,
        i32 height,
        FluidPicker globalFluidPicker);

    [[nodiscard]] const BlockState* computeSubstance(i32 blockX, i32 blockY, i32 blockZ, f64 densityValue) override;

    [[nodiscard]] bool shouldScheduleFluidUpdate() const override { return m_shouldScheduleFluidUpdate; }

private:
    // ========== 网格常量 ==========
    static constexpr i32 X_RANGE = 10;
    static constexpr i32 Y_RANGE = 9;
    static constexpr i32 Z_RANGE = 10;
    static constexpr i32 X_SEPARATION = 6;
    static constexpr i32 Y_SEPARATION = 3;
    static constexpr i32 Z_SEPARATION = 6;
    static constexpr i32 X_SPACING = X_RANGE + X_SEPARATION; // 16
    static constexpr i32 Y_SPACING = Y_RANGE + Y_SEPARATION; // 12
    static constexpr i32 Z_SPACING = Z_RANGE + Z_SEPARATION; // 16
    static constexpr i32 MAX_REASONABLE_DISTANCE_TO_AQUIFER_CENTER_SQ = 11 * 11 * 3;
    static constexpr f64 FLOWING_UPDATE_SIMULARITY = 1.0 - (144.0 - 100.0) / 25.0;
    static constexpr i32 SAMPLE_OFFSET_X = -5;
    static constexpr i32 SAMPLE_OFFSET_Y = 1;
    static constexpr i32 SAMPLE_OFFSET_Z = -5;
    /// MC 1.21: MIN_Y << 4 where MIN_Y = -2032 → -32512
    static constexpr i32 WAY_BELOW_MIN_Y = -32512;

    // ========== 网格坐标转换 ==========
    /// MC 1.21: gridX 使用 >> 4（算术右移，等效于 floorDiv(x, 16)）
    [[nodiscard]] static i32 gridX(i32 x) { return x >> 4; }
    [[nodiscard]] static i32 gridY(i32 y) { return math::floorDiv(y, Y_SPACING); }
    /// MC 1.21: gridZ 使用 >> 4（算术右移，等效于 floorDiv(z, 16)）
    [[nodiscard]] static i32 gridZ(i32 z) { return z >> 4; }
    [[nodiscard]] static i32 fromGridX(i32 gridX, i32 offset) { return (gridX << 4) + offset; }
    [[nodiscard]] static i32 fromGridY(i32 gridY, i32 offset) { return gridY * Y_SPACING + offset; }
    [[nodiscard]] static i32 fromGridZ(i32 gridZ, i32 offset) { return (gridZ << 4) + offset; }

    // ========== 含水层状态缓存 ==========
    /**
     * @brief 含水层状态（对应 MC 的 AquiferStatus）
     */
    struct AquiferStatus {
        i32 fluidLevel = WAY_BELOW_MIN_Y;
        const BlockState* fluidType = nullptr;

        [[nodiscard]] bool operator==(const AquiferStatus& other) const
        {
            return fluidLevel == other.fluidLevel && fluidType == other.fluidType;
        }
        [[nodiscard]] bool operator!=(const AquiferStatus& other) const { return !(*this == other); }
    };

    // ========== 内部方法 ==========

    /**
     * @brief 获取指定网格位置的含水层状态
     *
     * 使用缓存避免重复计算。第一次访问时计算并缓存。
     *
     * @param gridX 网格 X 索引
     * @param gridY 网格 Y 索引
     * @param gridZ 网格 Z 索引
     * @return 含水层状态
     */
    [[nodiscard]] AquiferStatus getAquiferStatus(i32 gridX, i32 gridY, i32 gridZ);

    /**
     * @brief 计算含水层的流体状态
     *
     * 根据噪声和地表高度确定含水层的流体液面和类型。
     * 对应 MC 1.21 NoiseBasedAquifer.computeFluid()。
     */
    [[nodiscard]] AquiferStatus computeFluid(i32 x, i32 y, i32 z);

    /**
     * @brief 计算含水层的液面高度
     *
     * 对应 MC 1.21 NoiseBasedAquifer.computeSurfaceLevel()。
     * 使用 fluidLevelFloodednessNoise 和 fluidLevelSpreadNoise。
     */
    [[nodiscard]] i32 computeSurfaceLevel(
        i32 x, i32 y, i32 z, const FluidStatus& globalFluid, i32 minSurface, bool centerHasFluid);

    /**
     * @brief 计算含水层的随机化液面高度
     *
     * 对应 MC 1.21 NoiseBasedAquifer.computeRandomizedFluidSurfaceLevel()。
     * 使用 fluidLevelSpreadNoise 在地表附近添加随机偏移。
     */
    [[nodiscard]] i32 computeRandomizedFluidSurfaceLevel(i32 x, i32 y, i32 z, i32 minSurface);

    /**
     * @brief 计算含水层的流体类型（水/熔岩）
     *
     * 对应 MC 1.21 NoiseBasedAquifer.computeFluidType()。
     * 在深层使用 lavaNoise 判断是否为熔岩。
     */
    [[nodiscard]] const BlockState* computeFluidType(i32 x, i32 y, i32 z, const FluidStatus& globalFluid, i32 level);

    /**
     * @brief 计算含水层之间的压力
     *
     * 对应 MC 1.21 NoiseBasedAquifer.calculatePressure()。
     * 使用 barrierNoise 在含水层边界创建屏障。
     *
     * @param blockX 方块 X 坐标
     * @param blockY 方块 Y 坐标
     * @param blockZ 方块 Z 坐标
     * @param a 第一个含水层状态
     * @param b 第二个含水层状态
     * @param cachedBarrierNoise 预计算的屏障噪声值（MC 1.21: 在 computeSubstance 开头计算一次）
     */
    [[nodiscard]] f64 calculatePressure(
        i32 blockX, i32 blockY, i32 blockZ, const AquiferStatus& a, const AquiferStatus& b, f64 cachedBarrierNoise);

    /**
     * @brief 计算两个距离之间的相似度
     *
     * similarity = 1.0 - (dist2 - dist1) / 25.0
     * 当 similarity <= 0 时，表示完全在含水层内部。
     */
    [[nodiscard]] static f64 similarity(i32 dist1Sq, i32 dist2Sq);

    /**
     * @brief 获取缓存的含水层位置
     */
    [[nodiscard]] i64 getAquiferLocation(i32 gridX, i32 gridY, i32 gridZ);

    /**
     * @brief 编码方块位置为 64 位整数
     *
     * MC 1.21 BlockPos.asLong() 格式：
     * 高 26 位: X, 中 12 位: Y, 低 26 位: Z
     */
    [[nodiscard]] static i64 encodeBlockPos(i32 x, i32 y, i32 z);

    /**
     * @brief 解码 64 位整数中的 X 坐标
     */
    [[nodiscard]] static i32 decodeBlockPosX(i64 pos) { return static_cast<i32>(pos >> 38); }

    /**
     * @brief 解码 64 位整数中的 Y 坐标
     */
    [[nodiscard]] static i32 decodeBlockPosY(i64 pos) { return static_cast<i32>((pos << 52) >> 52); }

    /**
     * @brief 解码 64 位整数中的 Z 坐标
     */
    [[nodiscard]] static i32 decodeBlockPosZ(i64 pos) { return static_cast<i32>((pos << 26) >> 38); }

    // ========== 成员变量 ==========

    const density::NoiseChunk& m_noiseChunk;
    const density::DensityFunction& m_barrierNoise;
    const density::DensityFunction& m_fluidLevelFloodednessNoise;
    const density::DensityFunction& m_fluidLevelSpreadNoise;
    const density::DensityFunction& m_lavaNoise;
    const density::DensityFunction& m_erosion;
    const density::DensityFunction& m_depth;
    math::PositionalRandomFactory m_positionalRandom;
    FluidPicker m_globalFluidPicker;

    /// 跳过含水层采样的 Y 高度（地表以上不需要含水层）
    i32 m_skipSamplingAboveY;

    /// 含水层位置缓存 [gridZ][gridY][gridX] → 64位位置编码
    /// 使用展平的数组，索引 = (gridX - minGridX) + gridSizeX * ((gridY - minGridY) + gridSizeY * (gridZ - minGridZ))
    std::vector<i64> m_aquiferLocationCache;

    /// 含水层状态缓存（与 m_aquiferLocationCache 并行，惰性计算）
    std::vector<AquiferStatus> m_aquiferStatusCache;

    /// 缓存标记：已计算的状态
    std::vector<bool> m_aquiferStatusComputed;

    i32 m_minGridX;
    i32 m_minGridY;
    i32 m_minGridZ;
    i32 m_gridSizeX;
    i32 m_gridSizeY;
    i32 m_gridSizeZ;

    bool m_shouldScheduleFluidUpdate = false;

    // 表面采样偏移表（MC 1.21 Aquifer.SURFACE_SAMPLING_OFFSETS_IN_CHUNKS）
    // 13 个采样点：中心 + 周围 12 个区块偏移
    static constexpr i32 SURFACE_SAMPLING_OFFSETS[13][2] = {{0, 0},
        {-2, -1},
        {-1, -1},
        {0, -1},
        {1, -1},
        {-3, 0},
        {-2, 0},
        {-1, 0},
        {1, 0},
        {-2, 1},
        {-1, 1},
        {0, 1},
        {1, 1}};
};

} // namespace world::gen::aquifer
} // namespace mc
