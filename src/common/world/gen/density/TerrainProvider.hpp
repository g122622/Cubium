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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/world/gen/density/DensityFunctions.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::density {

/**
 * @brief MC 1.21.11 TerrainProvider — 主世界地形样条数据
 *
 * 提供主世界 offset/factor/jaggedness 三层嵌套样条树，
 * 用于通过 (continentalness, erosion, ridges/weirdness, depth) 四维参数
 * 计算地形高度偏移、缩放因子和锯齿度。
 *
 * 样条树结构:
 * - 外层: continentalness 轴
 * - 中层: erosion 轴
 * - 内层: ridges/weirdness 轴
 *
 * 对应 MC 1.21.11 的 net.minecraft.data.worldgen.TerrainProvider。
 *
 * 注意：由于 DensityFunction 使用 unique_ptr 不可共享，
 * 但嵌套样条需要多个子样条共享同一个输入密度函数（如 ridges），
 * 因此 CubicSpline 的输入使用 shared_ptr。
 * 调用者需要为每个样条轴创建独立的密度函数实例，
 * 或使用 shared_ptr 共享同一实例。
 */
class TerrainProvider {
public:
    /**
     * @brief 创建主世界 offset 样条
     *
     * 输入轴: continentalness → erosion → ridges
     * 返回值决定地形高度偏移。
     *
     * @param continents  大陆度密度函数（shared_ptr，可被子样条共享）
     * @param erosionCopy1 用于 erosion 子样条的副本
     * @param ridgesCopy1 用于 ridges 子样条的副本
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> overworldOffset(std::shared_ptr<DensityFunction> continents,
        std::shared_ptr<DensityFunction> erosion,
        std::shared_ptr<DensityFunction> ridges);

    /**
     * @brief 创建主世界 factor 样条
     *
     * 输入轴: continentalness → erosion → ridges/weirdness
     * 返回值决定地形缩放因子（大值=陡峭，小值=平坦）。
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> overworldFactor(std::shared_ptr<DensityFunction> continents,
        std::shared_ptr<DensityFunction> erosion,
        std::shared_ptr<DensityFunction> ridges,
        std::shared_ptr<DensityFunction> weirdness);

    /**
     * @brief 创建主世界 jaggedness 样条
     *
     * 输入轴: continentalness → erosion → ridges → weirdness
     * 返回值决定地形锯齿度（山脉顶部的随机起伏）。
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> overworldJaggedness(
        std::shared_ptr<DensityFunction> continents,
        std::shared_ptr<DensityFunction> erosion,
        std::shared_ptr<DensityFunction> ridges,
        std::shared_ptr<DensityFunction> weirdness);

    // ========== 密度函数辅助 ==========

    /**
     * @brief noiseGradientDensity(factor, depth) — MC 1.21 地形密度核心公式
     *
     * 当 factor > 0 时: depth / factor
     * 当 factor <= 0 时: depth - factor
     * 整体乘以 4.0 并对负值取四分之一。
     *
     * MC 1.21: 4 * (factor * depth).quarterNegative()
     * 即 mul(4.0, quarterNegative(mul(factor, depth)))
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> noiseGradientDensity(
        std::unique_ptr<DensityFunction> factor, std::unique_ptr<DensityFunction> depth);

    /**
     * @brief splineWithBlending(splineFunc) — 样条缓存包装
     *
     * MC 1.21: flatCache(cache2d(splineFunc))
     * 旧区块混合（Blender/BlendAlpha/BlendOffset）已移除，本项目不兼容旧版存档，
     * 因此样条输出直接缓存，无 lerp 间接层。
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> splineWithBlending(
        std::unique_ptr<DensityFunction> splineFunc);

    /**
     * @brief yLimitedInterpolatable(yFunction, noiseFunction, minY, maxY, outOfRangeValue)
     *
     * MC 1.21: interpolated(rangeChoice(yFunction, minY, maxY+1, noiseFunction, constant(outOfRangeValue)))
     * 当 Y 在 [minY, maxY+1) 范围内时使用插值噪声，范围外使用 outOfRangeValue
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> yLimitedInterpolatable(
        std::unique_ptr<DensityFunction> yFunction,
        std::unique_ptr<DensityFunction> noiseFunction,
        i32 minY,
        i32 maxY,
        f64 outOfRangeValue);

    // ========== 常量 ==========

    static constexpr f64 DEEP_OCEAN_CONTINENTALNESS = -0.51;
    static constexpr f64 OCEAN_CONTINENTALNESS = -0.4;
    static constexpr f64 PLAINS_CONTINENTALNESS = 0.1;
    static constexpr f64 BEACH_CONTINENTALNESS = -0.15;
    static constexpr f64 GLOBAL_OFFSET = -0.50375;

private:
    // ========== 样条构建辅助 ==========

    static constexpr f64 PV_0_4 = -0.2;    // peaksAndValleys(0.4)
    static constexpr f64 PV_0_5667 = -0.7; // peaksAndValleys(0.56666666)
    static constexpr f64 PV_MID = -0.45;   // (PV_0_4 + PV_0_5667) / 2

    [[nodiscard]] static f64 peaksAndValleys(f64 v);
    [[nodiscard]] static f64 mountainContinentalness(f64 x, f64 offsetSplineScale);
    [[nodiscard]] static f64 calculateMountainRidgeZeroContinentalnessPoint(f64 offsetSplineScale);

    /**
     * @brief 构建山脊样条（5 点 ridgeSpline）
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> ridgeSpline(
        std::shared_ptr<DensityFunction> ridges, f64 p0, f64 p1, f64 p2, f64 p3, f64 p4, f64 pMinDerivative);

    /**
     * @brief 构建简单两点样条
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> simpleRidgeSpline(
        std::shared_ptr<DensityFunction> ridges, f64 loc0, f64 val0, f64 loc1, f64 val1);

    /**
     * @brief 构建山脊山脊子样条（带控制点）
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> buildMountainRidgeSplineWithPoints(
        std::shared_ptr<DensityFunction> ridges, f64 offsetSplineScale, bool isMountainTailEnd);

    /**
     * @brief 构建 erosion offset 子样条
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> buildErosionOffsetSpline(std::shared_ptr<DensityFunction> erosion,
        std::shared_ptr<DensityFunction> ridges,
        f64 offsetSplineY,
        f64 offsetSplineStart,
        f64 offsetSplineEnd,
        f64 offsetSplineScale,
        f64 offsetSplineMiddle,
        f64 offsetSplineTailEnd,
        bool isMountainStart,
        bool isMountainTailEnd);

    /**
     * @brief 构建 erosion factor 子样条
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> buildErosionFactorSpline(std::shared_ptr<DensityFunction> erosion,
        std::shared_ptr<DensityFunction> ridges,
        std::shared_ptr<DensityFunction> weirdness,
        f64 peakFactor,
        bool isNearCoast);

    /**
     * @brief 构建 erosion jaggedness 子样条
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> buildErosionJaggednessSpline(
        std::shared_ptr<DensityFunction> erosion,
        std::shared_ptr<DensityFunction> ridges,
        std::shared_ptr<DensityFunction> weirdness,
        f64 jaggednessScale,
        f64 jaggednessMidScale,
        f64 jaggednessEndScale,
        f64 jaggednessTailScale);

    /**
     * @brief 构建山脊锯齿度子样条
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> buildRidgeJaggednessSpline(
        std::shared_ptr<DensityFunction> ridges,
        std::shared_ptr<DensityFunction> weirdness,
        f64 scalePeak,
        f64 scaleMid);

    /**
     * @brief 构建奇异度锯齿度子样条
     */
    [[nodiscard]] static std::shared_ptr<CubicSpline> buildWeirdnessJaggednessSpline(
        std::shared_ptr<DensityFunction> weirdness, f64 scale);
};

} // namespace mc::world::gen::density
