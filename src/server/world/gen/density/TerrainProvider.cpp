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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "server/world/gen/density/TerrainProvider.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/generation/density/DensityFunction.hpp"
#include "server/world/gen/density/DensityFunctions.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::density {

// ============================================================================
// 辅助函数
// ============================================================================

f32 TerrainProvider::peaksAndValleys(f32 v)
{
    return -(std::abs(std::abs(v) - 2.0f / 3.0f) - 1.0f / 3.0f) * 3.0f;
}

f32 TerrainProvider::mountainContinentalness(f32 x, f32 offsetSplineScale)
{
    constexpr f32 THRESHOLD = -0.7f;
    const f32 f2 = 1.0f - (1.0f - offsetSplineScale) * 0.5f;
    const f32 f3 = 0.5f * (1.0f - offsetSplineScale);
    const f32 f4 = (x + 1.17f) * 0.46082947f;
    const f32 f5 = f4 * f2 - f3;
    return x < THRESHOLD ? std::max(f5, -0.2222f) : std::max(f5, 0.0f);
}

f32 TerrainProvider::calculateMountainRidgeZeroContinentalnessPoint(f32 offsetSplineScale)
{
    const f32 f2 = 1.0f - (1.0f - offsetSplineScale) * 0.5f;
    const f32 f3 = 0.5f * (1.0f - offsetSplineScale);
    return f3 / (0.46082947f * f2) - 1.17f;
}

// ============================================================================
// 密度函数辅助
// ============================================================================

std::unique_ptr<DensityFunction> TerrainProvider::noiseGradientDensity(
    std::unique_ptr<DensityFunction> factor, std::unique_ptr<DensityFunction> depth)
{
    // MC 1.21: 4 * (factor * depth).quarterNegative()
    auto product = factory::mul(std::move(factor), std::move(depth));
    auto quarter = factory::quarterNegative(std::move(product));
    return factory::mul(factory::constant(4.0f), std::move(quarter));
}

std::unique_ptr<DensityFunction> TerrainProvider::splineWithBlending(std::unique_ptr<DensityFunction> splineFunc)
{
    // MC 1.21: flatCache(cache2d(splineFunc))
    // 旧区块混合（Blender/BlendAlpha/BlendOffset）已移除，样条输出直接缓存。
    return factory::flatCacheMarker(factory::cache2DMarker(std::move(splineFunc)));
}

std::unique_ptr<DensityFunction> TerrainProvider::yLimitedInterpolatable(std::unique_ptr<DensityFunction> yFunction,
    std::unique_ptr<DensityFunction> noiseFunction,
    i32 minY,
    i32 maxY,
    f32 outOfRangeValue)
{
    // MC 1.21: interpolated(rangeChoice(Y, minY, maxY+1, noise, constant(fallback)))
    auto rangeChoice = factory::rangeChoice(std::move(yFunction),
        static_cast<f32>(minY),
        static_cast<f32>(maxY + 1),
        std::move(noiseFunction),
        factory::constant(outOfRangeValue));
    return factory::interpolated(std::move(rangeChoice));
}

// ============================================================================
// 简单样条构建
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::simpleRidgeSpline(
    std::shared_ptr<DensityFunction> ridges, f32 loc0, f32 val0, f32 loc1, f32 val1)
{
    std::vector<SplinePoint> points;
    points.push_back({loc0, val0, 0.0f});
    points.push_back({loc1, val1, 0.0f});
    return std::make_shared<CubicSpline>(std::move(ridges), std::move(points));
}

// ============================================================================
// ridgeSpline
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::ridgeSpline(
    std::shared_ptr<DensityFunction> ridges, f32 p0, f32 p1, f32 p2, f32 p3, f32 p4, f32 pMinDerivative)
{
    const f32 f = std::max(0.5f * (p1 - p0), pMinDerivative);
    const f32 f1 = 5.0f * (p2 - p1);

    std::vector<SplinePoint> points;
    points.push_back({-1.0f, p0, f});
    points.push_back({-0.4f, p1, std::min(f, f1)});
    points.push_back({0.0f, p2, f1});
    points.push_back({0.4f, p3, 2.0f * (p3 - p2)});
    points.push_back({1.0f, p4, 0.7f * (p4 - p3)});

    return std::make_shared<CubicSpline>(std::move(ridges), std::move(points));
}

// ============================================================================
// buildWeirdnessJaggednessSpline
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::buildWeirdnessJaggednessSpline(
    std::shared_ptr<DensityFunction> weirdness, f32 scale)
{
    const f32 f = 0.63f * scale;
    const f32 f1 = 0.3f * scale;

    std::vector<SplinePoint> points;
    points.push_back({-0.01f, f, 0.0f});
    points.push_back({0.01f, f1, 0.0f});

    return std::make_shared<CubicSpline>(std::move(weirdness), std::move(points));
}

// ============================================================================
// buildRidgeJaggednessSpline
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::buildRidgeJaggednessSpline(
    std::shared_ptr<DensityFunction> ridges, std::shared_ptr<DensityFunction> weirdness, f32 scalePeak, f32 scaleMid)
{
    std::vector<SplinePoint> points;
    points.push_back({PV_0_4, 0.0f, 0.0f}); // peaksAndValleys(0.4f) = -0.2f

    if (scaleMid > 0.0f) {
        auto wJaggedMid = buildWeirdnessJaggednessSpline(weirdness, scaleMid);
        points.push_back({PV_MID, std::move(wJaggedMid), 0.0f}); // peaksAndValleys(mid) = -0.45f
    } else {
        points.push_back({PV_MID, 0.0f, 0.0f});
    }

    if (scalePeak > 0.0f) {
        auto wJaggedPeak = buildWeirdnessJaggednessSpline(weirdness, scalePeak);
        points.push_back({1.0f, std::move(wJaggedPeak), 0.0f});
    } else {
        points.push_back({1.0f, 0.0f, 0.0f});
    }

    return std::make_shared<CubicSpline>(std::move(ridges), std::move(points));
}

// ============================================================================
// buildMountainRidgeSplineWithPoints
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::buildMountainRidgeSplineWithPoints(
    std::shared_ptr<DensityFunction> ridges, f32 offsetSplineScale, bool isMountainTailEnd)
{
    const f32 f2_val = mountainContinentalness(-1.0f, offsetSplineScale);
    const f32 f4_val = mountainContinentalness(1.0f, offsetSplineScale);
    const f32 zeroPoint = calculateMountainRidgeZeroContinentalnessPoint(offsetSplineScale);
    const f32 f5_zero = mountainContinentalness(zeroPoint, offsetSplineScale);

    if (-0.65f < f5_zero && f5_zero < 1.0f && zeroPoint > -0.65f && zeroPoint < 1.0f) {
        const f32 f9_val = mountainContinentalness(-0.75f, offsetSplineScale);
        const f32 f14_val = mountainContinentalness(-0.65f, offsetSplineScale);
        const f32 f11_val = mountainContinentalness(zeroPoint, offsetSplineScale);

        const f32 slope1 = (f2_val - f9_val) / (-1.0f - (-0.75f));
        const f32 slope2 = (zeroPoint - 1.0f) != 0.0f ? (f11_val - f4_val) / (zeroPoint - 1.0f) : 0.0f;

        std::vector<SplinePoint> points;
        points.push_back({-1.0f, f2_val, slope1});
        points.push_back({-0.75f, f9_val, 0.0f});
        points.push_back({-0.65f, f14_val, 0.0f});
        points.push_back({zeroPoint - 0.01f, f11_val, 0.0f});
        points.push_back({zeroPoint, f11_val, slope2});
        points.push_back({1.0f, f4_val, slope2});

        return std::make_shared<CubicSpline>(std::move(ridges), std::move(points));
    }

    const f32 slope = (-2.0f != 0.0f) ? (f2_val - f4_val) / (-2.0f) : 0.0f;

    std::vector<SplinePoint> points;
    if (isMountainTailEnd) {
        const f32 midVal = 0.5f * (f2_val + f4_val);
        points.push_back({-1.0f, std::max(0.2f, f2_val), 0.0f});
        points.push_back({0.0f, midVal, slope});
        points.push_back({1.0f, f4_val, slope});
    } else {
        points.push_back({-1.0f, f2_val, slope});
        points.push_back({1.0f, f4_val, slope});
    }

    return std::make_shared<CubicSpline>(std::move(ridges), std::move(points));
}

// ============================================================================
// buildErosionOffsetSpline
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::buildErosionOffsetSpline(std::shared_ptr<DensityFunction> erosion,
    std::shared_ptr<DensityFunction> ridges,
    f32 offsetSplineY,
    f32 offsetSplineStart,
    f32 offsetSplineEnd,
    f32 offsetSplineScale,
    f32 offsetSplineMiddle,
    f32 offsetSplineTailEnd,
    bool isMountainStart,
    bool isMountainTailEnd)
{
    // 计算山脊子样条
    const f32 mountainHigh = 0.6f + (1.5f - 0.6f) * offsetSplineScale; // lerp(offsetSplineScale, 0.6f, 1.5f)
    const f32 mountainMid = 0.6f + (1.0f - 0.6f) * offsetSplineScale;  // lerp(offsetSplineScale, 0.6f, 1.0f)

    auto mountainHighRidge = buildMountainRidgeSplineWithPoints(ridges, mountainHigh, isMountainTailEnd);
    auto mountainMidRidge = buildMountainRidgeSplineWithPoints(ridges, mountainMid, isMountainTailEnd);
    auto mountainLowRidge = buildMountainRidgeSplineWithPoints(ridges, offsetSplineScale, isMountainTailEnd);

    // ridgeSpline3: (offsetSplineY - 0.15, 0.5*scale, 0.5*scale, 0.5*scale, 0.6*scale, 0.5)
    auto rs3 = ridgeSpline(ridges,
        offsetSplineY - 0.15f,
        0.5f * offsetSplineScale,
        0.5f * offsetSplineScale,
        0.5f * offsetSplineScale,
        0.6f * offsetSplineScale,
        0.5f);

    // ridgeSpline4: (offsetSplineY, offsetSplineMiddle*scale, offsetSplineStart*scale, 0.5*scale, 0.6*scale, 0.5)
    auto rs4 = ridgeSpline(ridges,
        offsetSplineY,
        offsetSplineMiddle * offsetSplineScale,
        offsetSplineStart * offsetSplineScale,
        0.5f * offsetSplineScale,
        0.6f * offsetSplineScale,
        0.5f);

    // ridgeSpline5: (offsetSplineY, offsetSplineMiddle, offsetSplineMiddle, offsetSplineStart, offsetSplineEnd, 0.5)
    auto rs5 = ridgeSpline(
        ridges, offsetSplineY, offsetSplineMiddle, offsetSplineMiddle, offsetSplineStart, offsetSplineEnd, 0.5f);

    // ridgeSpline8: (-0.02, offsetSplineTailEnd, offsetSplineTailEnd, offsetSplineStart, offsetSplineEnd, 0.0)
    auto rs8 =
        ridgeSpline(ridges, -0.02f, offsetSplineTailEnd, offsetSplineTailEnd, offsetSplineStart, offsetSplineEnd, 0.0f);

    // 构建 erosion 轴样条
    std::vector<SplinePoint> points;

    // -0.85: mountainHigh
    points.push_back({-0.85f, std::move(mountainHighRidge), 0.0f});
    // -0.7: mountainMid
    points.push_back({-0.7f, std::move(mountainMidRidge), 0.0f});
    // -0.4: mountainLow
    points.push_back({-0.4f, std::move(mountainLowRidge), 0.0f});
    // -0.35: ridgeSpline3
    points.push_back({-0.35f, std::move(rs3), 0.0f});
    // -0.1: ridgeSpline4
    points.push_back({-0.1f, std::move(rs4), 0.0f});
    // 0.2: ridgeSpline5
    points.push_back({0.2f, rs5, 0.0f}); // shared_ptr copy — may be reused below

    if (isMountainStart) {
        // 0.4: same as ridgeSpline5 (rs5)
        points.push_back({0.4f, rs5, 0.0f});

        // 0.45, 0.55: MC 1.21 两个点使用相同的 innerSpline（cubicspline7）
        std::vector<SplinePoint> innerPoints;
        innerPoints.push_back({-1.0f, offsetSplineY, 0.0f});
        innerPoints.push_back({-0.4f, rs5, 0.0f});
        innerPoints.push_back({0.0f, offsetSplineEnd + 0.07f, 0.0f});
        auto innerSpline = std::make_shared<CubicSpline>(ridges, std::move(innerPoints));

        points.push_back({0.45f, innerSpline, 0.0f});
        points.push_back({0.55f, innerSpline, 0.0f});
        // 0.58: same as ridgeSpline5
        points.push_back({0.58f, rs5, 0.0f});
    }

    // 0.7: ridgeSpline8
    points.push_back({0.7f, std::move(rs8), 0.0f});

    return std::make_shared<CubicSpline>(std::move(erosion), std::move(points));
}

// ============================================================================
// buildErosionFactorSpline
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::buildErosionFactorSpline(std::shared_ptr<DensityFunction> erosion,
    std::shared_ptr<DensityFunction> ridges,
    std::shared_ptr<DensityFunction> weirdness,
    f32 peakFactor,
    bool isNearCoast)
{
    // 共享内层样条: ridges -> (-0.2 -> 6.3, 0.2 -> peakFactor)
    auto sharedRidgeSpline = simpleRidgeSpline(ridges, -0.2f, 6.3f, 0.2f, peakFactor);

    // ridges at (-0.05, 6.3) and (0.05, 2.67)
    auto midRidgeSpline = simpleRidgeSpline(ridges, -0.05f, 6.3f, 0.05f, 2.67f);

    // ridges at (-0.05, 2.67) and (0.05, 6.3)
    auto invertedMidRidgeSpline = simpleRidgeSpline(ridges, -0.05f, 2.67f, 0.05f, 6.3f);

    std::vector<SplinePoint> points;

    // -0.6: sharedRidgeSpline
    points.push_back({-0.6f, sharedRidgeSpline, 0.0f});
    // -0.5: midRidgeSpline
    points.push_back({-0.5f, midRidgeSpline, 0.0f});
    // -0.35: sharedRidgeSpline
    points.push_back({-0.35f, sharedRidgeSpline, 0.0f});
    // -0.25: sharedRidgeSpline
    points.push_back({-0.25f, sharedRidgeSpline, 0.0f});
    // -0.1: invertedMidRidgeSpline
    points.push_back({-0.1f, invertedMidRidgeSpline, 0.0f});
    // 0.03: sharedRidgeSpline
    points.push_back({0.03f, sharedRidgeSpline, 0.0f});

    if (isNearCoast) {
        // 0.35: constant peakFactor
        points.push_back({0.35f, peakFactor, 0.0f});

        // 0.45, 0.55: weirdness sub-spline
        // coastSubRidge: ridges -> (0.0 -> peakFactor, 0.1 -> 0.625)
        auto coastSubRidge = simpleRidgeSpline(ridges, 0.0f, peakFactor, 0.1f, 0.625f);
        // coastSubWeirdness: weirdness -> (-0.9 -> peakFactor, -0.69 -> coastSubRidge)
        std::vector<SplinePoint> coastWeirdnessPoints;
        coastWeirdnessPoints.push_back({-0.9f, peakFactor, 0.0f});
        coastWeirdnessPoints.push_back({-0.69f, std::move(coastSubRidge), 0.0f});
        auto coastWeirdnessSpline = std::make_shared<CubicSpline>(weirdness, std::move(coastWeirdnessPoints));

        points.push_back({0.45f, coastWeirdnessSpline, 0.0f});
        points.push_back({0.55f, std::move(coastWeirdnessSpline), 0.0f});

        // 0.62: constant peakFactor
        points.push_back({0.62f, peakFactor, 0.0f});
    } else {
        // inland sub-splines
        // inlandRidgeSub 与 sharedRidgeSpline 参数相同，复用之

        // inlandWeirdness3: weirdness -> (-0.7 -> sharedRidgeSpline, -0.15 -> 1.37)
        std::vector<SplinePoint> inlandWeirdness3Points;
        inlandWeirdness3Points.push_back({-0.7f, sharedRidgeSpline, 0.0f});
        inlandWeirdness3Points.push_back({-0.15f, 1.37f, 0.0f});
        auto inlandWeirdness3 = std::make_shared<CubicSpline>(weirdness, std::move(inlandWeirdness3Points));

        // inlandWeirdness4: weirdness -> (0.45 -> sharedRidgeSpline, 0.7 -> 1.56)
        std::vector<SplinePoint> inlandWeirdness4Points;
        inlandWeirdness4Points.push_back({0.45f, sharedRidgeSpline, 0.0f});
        inlandWeirdness4Points.push_back({0.7f, 1.56f, 0.0f});
        auto inlandWeirdness4 = std::make_shared<CubicSpline>(weirdness, std::move(inlandWeirdness4Points));

        // 0.05: inlandWeirdness4
        points.push_back({0.05f, inlandWeirdness4, 0.0f});
        // 0.4: inlandWeirdness4
        points.push_back({0.4f, std::move(inlandWeirdness4), 0.0f});
        // 0.45: inlandWeirdness3
        points.push_back({0.45f, inlandWeirdness3, 0.0f});
        // 0.55: inlandWeirdness3
        points.push_back({0.55f, std::move(inlandWeirdness3), 0.0f});
        // 0.58: constant peakFactor
        points.push_back({0.58f, peakFactor, 0.0f});
    }

    return std::make_shared<CubicSpline>(std::move(erosion), std::move(points));
}

// ============================================================================
// buildErosionJaggednessSpline
// ============================================================================

std::shared_ptr<CubicSpline> TerrainProvider::buildErosionJaggednessSpline(std::shared_ptr<DensityFunction> erosion,
    std::shared_ptr<DensityFunction> ridges,
    std::shared_ptr<DensityFunction> weirdness,
    f32 jaggednessScale,
    f32 jaggednessMidScale,
    f32 jaggednessEndScale,
    f32 jaggednessTailScale)
{
    // peakRidgeJaggedness: at erosion=-1.0 and -0.78
    auto peakRidgeJagged = buildRidgeJaggednessSpline(ridges, weirdness, jaggednessScale, jaggednessEndScale);
    auto midRidgeJagged = buildRidgeJaggednessSpline(ridges, weirdness, jaggednessMidScale, jaggednessTailScale);

    std::vector<SplinePoint> points;
    points.push_back({-1.0f, std::move(peakRidgeJagged), 0.0f});
    points.push_back({-0.78f, midRidgeJagged, 0.0f}); // shared_ptr copy
    points.push_back({-0.5775f, std::move(midRidgeJagged), 0.0f});
    points.push_back({-0.375f, 0.0f, 0.0f});

    return std::make_shared<CubicSpline>(std::move(erosion), std::move(points));
}

// ============================================================================
// overworldOffset
// ============================================================================

std::unique_ptr<DensityFunction> TerrainProvider::overworldOffset(std::shared_ptr<DensityFunction> continents,
    std::shared_ptr<DensityFunction> erosion,
    std::shared_ptr<DensityFunction> ridges)
{
    // MC 1.21: overworldOffset 是 continentalness 轴的样条
    // 每个控制点值是 erosion 轴的子样条（包含 ridges 子样条）

    // cubicspline: buildErosionOffsetSpline(-0.15, 0.0, 0.0, 0.1, 0.0, -0.03, false, false)
    auto erosion0 = buildErosionOffsetSpline(erosion, ridges, -0.15f, 0.0f, 0.0f, 0.1f, 0.0f, -0.03f, false, false);

    // cubicspline1: buildErosionOffsetSpline(-0.1, 0.03, 0.1, 0.1, 0.01, -0.03, false, false)
    auto erosion1 = buildErosionOffsetSpline(erosion, ridges, -0.1f, 0.03f, 0.1f, 0.1f, 0.01f, -0.03f, false, false);

    // cubicspline2: buildErosionOffsetSpline(-0.1, 0.03, 0.1, 0.7, 0.01, -0.03, true, true)
    auto erosion2 = buildErosionOffsetSpline(erosion, ridges, -0.1f, 0.03f, 0.1f, 0.7f, 0.01f, -0.03f, true, true);

    // cubicspline3: buildErosionOffsetSpline(-0.05, 0.03, 0.1, 1.0, 0.01, 0.01, true, true)
    auto erosion3 = buildErosionOffsetSpline(erosion, ridges, -0.05f, 0.03f, 0.1f, 1.0f, 0.01f, 0.01f, true, true);
    auto erosion3copy = erosion3; // shared_ptr copy for 1.0f point

    // 构建 continentalness 轴样条
    std::vector<SplinePoint> points;
    points.push_back({-1.1f, 0.044f, 0.0f});
    points.push_back({-1.02f, -0.2222f, 0.0f});
    points.push_back({-0.51f, -0.2222f, 0.0f});
    points.push_back({-0.44f, -0.12f, 0.0f});
    points.push_back({-0.18f, -0.12f, 0.0f});
    points.push_back({-0.16f, std::move(erosion0), 0.0f});
    points.push_back({-0.15f, std::move(erosion1), 0.0f});
    points.push_back({-0.1f, std::move(erosion2), 0.0f});
    points.push_back({0.25f, std::move(erosion3), 0.0f});
    points.push_back({1.0f, std::move(erosion3copy), 0.0f});

    return std::make_unique<CubicSpline>(std::move(continents), std::move(points));
}

// ============================================================================
// overworldFactor
// ============================================================================

std::unique_ptr<DensityFunction> TerrainProvider::overworldFactor(std::shared_ptr<DensityFunction> continents,
    std::shared_ptr<DensityFunction> erosion,
    std::shared_ptr<DensityFunction> ridges,
    std::shared_ptr<DensityFunction> weirdness)
{
    // MC 1.21: overworldFactor 是 continentalness 轴的样条
    // 非放大版本（transform = IDENTITY）

    // -0.15: getErosionFactor(6.25, isNearCoast=true)
    auto factor0 = buildErosionFactorSpline(erosion, ridges, weirdness, 6.25f, true);

    // -0.1: getErosionFactor(5.47, isNearCoast=true)
    auto factor1 = buildErosionFactorSpline(erosion, ridges, weirdness, 5.47f, true);

    // 0.03: getErosionFactor(5.08, isNearCoast=true)
    auto factor2 = buildErosionFactorSpline(erosion, ridges, weirdness, 5.08f, true);

    // 0.06: getErosionFactor(4.69, isNearCoast=false)
    auto factor3 = buildErosionFactorSpline(erosion, ridges, weirdness, 4.69f, false);

    std::vector<SplinePoint> points;
    points.push_back({-0.19f, 3.95f, 0.0f});
    points.push_back({-0.15f, std::move(factor0), 0.0f});
    points.push_back({-0.1f, std::move(factor1), 0.0f});
    points.push_back({0.03f, std::move(factor2), 0.0f});
    points.push_back({0.06f, std::move(factor3), 0.0f});

    return std::make_unique<CubicSpline>(std::move(continents), std::move(points));
}

// ============================================================================
// overworldJaggedness
// ============================================================================

std::unique_ptr<DensityFunction> TerrainProvider::overworldJaggedness(std::shared_ptr<DensityFunction> continents,
    std::shared_ptr<DensityFunction> erosion,
    std::shared_ptr<DensityFunction> ridges,
    std::shared_ptr<DensityFunction> weirdness)
{
    // MC 1.21: overworldJaggedness 是 continentalness 轴的样条
    // 非放大版本（transform = IDENTITY）

    // 0.03: buildErosionJaggednessSpline(1.0, 0.5, 0.0, 0.0)
    auto jagged0 = buildErosionJaggednessSpline(erosion, ridges, weirdness, 1.0f, 0.5f, 0.0f, 0.0f);

    // 0.65: buildErosionJaggednessSpline(1.0, 1.0, 1.0, 0.0)
    auto jagged1 = buildErosionJaggednessSpline(erosion, ridges, weirdness, 1.0f, 1.0f, 1.0f, 0.0f);

    std::vector<SplinePoint> points;
    points.push_back({-0.11f, 0.0f, 0.0f});
    points.push_back({0.03f, std::move(jagged0), 0.0f});
    points.push_back({0.65f, std::move(jagged1), 0.0f});

    return std::make_unique<CubicSpline>(std::move(continents), std::move(points));
}

} // namespace mc::world::gen::density
