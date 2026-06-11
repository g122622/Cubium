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

#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::world::gen::density {

// ============================================================================
// EndIslands 实现
// ============================================================================

EndIslands::EndIslands(u64 seed)
    : m_seed(seed)
{
    // MC 1.21.11: LegacyRandomSource(seed).consumeCount(17292) 后创建 SimplexNoise
    // consumeCount 等价于调用 nextLong() 17292 次来推进随机状态
    math::Random rng(seed);
    for (i32 i = 0; i < 17292; ++i) {
        static_cast<void>(rng.nextLong());
    }
    m_islandNoise = std::make_unique<noise::SimplexNoise>(rng);
}

f64 EndIslands::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    MC_UNUSED(blockY);

    // MC 1.21: 采样网格为 8 格间距
    const i32 sx = blockX >> 3; // blockX / 8
    const i32 sz = blockZ >> 3; // blockZ / 8

    const f64 height = getHeightValue(sx, sz);
    return (height - 8.0) / 128.0;
}

f64 EndIslands::getHeightValue(i32 x, i32 z) const
{
    // MC 1.21.11 EndIslandDensityFunction.getHeightValue
    const i32 i = x >> 1; // x / 2
    const i32 j = z >> 1; // z / 2
    const i32 k = x & 1;  // x % 2
    const i32 l = z & 1;  // z % 2

    // 基础高度：根据到原点的距离
    f64 f = 100.0 - std::sqrt(static_cast<f64>(x * x + z * z)) * 8.0;
    f = std::clamp(f, -100.0, 80.0);

    // 检测周围的岛屿（-12 到 +12 范围内）
    for (i32 i1 = -12; i1 <= 12; ++i1) {
        for (i32 j1 = -12; j1 <= 12; ++j1) {
            const i64 k1 = static_cast<i64>(i) + i1;
            const i64 l1 = static_cast<i64>(j) + j1;

            // MC 1.21.11: 只在外岛区域（距中心 > 64 区块半径）且噪声 < -0.9 时处理
            // 原代码错误地将此条件反转（continue 跳过了外岛区域）
            if (k1 * k1 + l1 * l1 > 4096L &&
                m_islandNoise->getValue(static_cast<f64>(k1), static_cast<f64>(l1)) < -0.9) {
                // 计算此岛屿对此位置的高度贡献
                const f64 f1 =
                    std::fmod(std::abs(static_cast<f64>(k1)) * 3439.0 + std::abs(static_cast<f64>(l1)) * 147.0, 13.0) +
                    9.0;
                const f64 f2 = static_cast<f64>(k) - i1 * 2;
                const f64 f3 = static_cast<f64>(l) - j1 * 2;
                f64 f4 = 100.0 - std::sqrt(f2 * f2 + f3 * f3) * f1;
                f4 = std::clamp(f4, -100.0, 80.0);
                f = std::max(f, f4);
            }
        }
    }

    return f;
}

// ============================================================================
// CubicSpline 实现
// ============================================================================

CubicSpline::CubicSpline(std::shared_ptr<DensityFunction> input, std::vector<SplinePoint> points)
    : m_input(std::move(input))
    , m_points(std::move(points))
{
    MC_ASSERT_RELEASE(!m_points.empty());
    MC_ASSERT_RELEASE(m_input != nullptr);
    computeBounds();
}

f64 CubicSpline::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    const f64 coordinate = m_input->compute(blockX, blockY, blockZ);
    return apply(coordinate, blockX, blockY, blockZ);
}

f64 CubicSpline::apply(f64 coordinate, i32 blockX, i32 blockY, i32 blockZ) const
{
    const size_t n = m_points.size();

    // 在第一个点之前 - 线性外推
    if (coordinate <= m_points[0].location) {
        const f64 value0 = resolvePointValue(0, blockX, blockY, blockZ);
        return linearExtend(coordinate, m_points[0].location, value0, m_points[0].derivative);
    }

    // 在最后一个点之后 - 线性外推
    if (coordinate >= m_points[n - 1].location) {
        const f64 valueN = resolvePointValue(n - 1, blockX, blockY, blockZ);
        return linearExtend(coordinate, m_points[n - 1].location, valueN, m_points[n - 1].derivative);
    }

    // 二分查找区间
    size_t lo = 0;
    size_t hi = n - 2;
    while (lo < hi) {
        const size_t mid = (lo + hi + 1) / 2;
        if (m_points[mid].location <= coordinate) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }

    // 三次 Hermite 插值（MC 1.21 CubicSpline.Multipoint.apply）
    const f64 loc0 = m_points[lo].location;
    const f64 loc1 = m_points[lo + 1].location;
    const f64 val0 = resolvePointValue(lo, blockX, blockY, blockZ);
    const f64 val1 = resolvePointValue(lo + 1, blockX, blockY, blockZ);
    const f64 der0 = m_points[lo].derivative;
    const f64 der1 = m_points[lo + 1].derivative;

    const f64 width = loc1 - loc0;
    const f64 t = (coordinate - loc0) / width;

    // MC 1.21: f4 = der0 * width - (val1 - val0), f5 = -der1 * width + (val1 - val0)
    const f64 f4 = der0 * width - (val1 - val0);
    const f64 f5 = -der1 * width + (val1 - val0);

    // lerp(t, val0, val1) + t * (1 - t) * lerp(t, f4, f5)
    return val0 + t * (val1 - val0) + t * (1.0 - t) * (f4 * (1.0 - t) + f5 * t);
}

f64 CubicSpline::resolvePointValue(size_t index, i32 blockX, i32 blockY, i32 blockZ) const
{
    const auto& point = m_points[index];
    if (std::holds_alternative<f64>(point.value)) {
        return std::get<f64>(point.value);
    }
    // 嵌套样条：递归计算
    return std::get<std::shared_ptr<CubicSpline>>(point.value)->compute(blockX, blockY, blockZ);
}

std::unique_ptr<DensityFunction> CubicSpline::mapAll(Visitor& visitor) const
{
    auto newInput = m_input->mapAll(visitor);
    auto sharedInput = std::shared_ptr<DensityFunction>(std::move(newInput));

    // 递归 mapAll 子样条
    std::vector<SplinePoint> newPoints;
    newPoints.reserve(m_points.size());
    for (const auto& point : m_points) {
        SplinePoint newPoint;
        newPoint.location = point.location;
        newPoint.derivative = point.derivative;
        if (std::holds_alternative<f64>(point.value)) {
            newPoint.value = std::get<f64>(point.value);
        } else {
            auto& childSpline = std::get<std::shared_ptr<CubicSpline>>(point.value);
            auto mappedChild = childSpline->mapAll(visitor);
            newPoint.value = std::shared_ptr<CubicSpline>(dynamic_cast<CubicSpline*>(mappedChild.release()));
        }
        newPoints.push_back(std::move(newPoint));
    }

    return visitor.apply(std::make_unique<CubicSpline>(std::move(sharedInput), std::move(newPoints)));
}

f64 CubicSpline::linearExtend(f64 coordinate, f64 location, f64 value, f64 derivative)
{
    if (derivative == 0.0) {
        return value;
    }
    return value + derivative * (coordinate - location);
}

void CubicSpline::computeBounds()
{
    // 递归计算所有控制点的值范围
    m_minValue = std::numeric_limits<f64>::max();
    m_maxValue = std::numeric_limits<f64>::lowest();

    for (const auto& point : m_points) {
        if (std::holds_alternative<f64>(point.value)) {
            m_minValue = std::min(m_minValue, std::get<f64>(point.value));
            m_maxValue = std::max(m_maxValue, std::get<f64>(point.value));
        } else {
            const auto& child = std::get<std::shared_ptr<CubicSpline>>(point.value);
            m_minValue = std::min(m_minValue, child->minValue());
            m_maxValue = std::max(m_maxValue, child->maxValue());
        }
    }

    // 考虑导数导致的外推超出范围
    const f64 inputMin = m_input->minValue();
    const f64 inputMax = m_input->maxValue();
    const f64 valAtMin = apply(inputMin, 0, 0, 0);
    const f64 valAtMax = apply(inputMax, 0, 0, 0);
    m_minValue = std::min({m_minValue, valAtMin, valAtMax});
    m_maxValue = std::max({m_maxValue, valAtMin, valAtMax});
}

// ============================================================================
// 工厂函数实现
// ============================================================================

namespace factory {

std::unique_ptr<DensityFunction> constant(f64 value)
{
    return std::make_unique<Constant>(value);
}

std::unique_ptr<DensityFunction> yClampedGradient(i32 fromY, i32 toY, f64 fromValue, f64 toValue)
{
    return std::make_unique<YClampedGradient>(fromY, toY, fromValue, toValue);
}

std::unique_ptr<DensityFunction> clamp(std::unique_ptr<DensityFunction> input, f64 minValue, f64 maxValue)
{
    return std::make_unique<Clamp>(std::move(input), minValue, maxValue);
}

std::unique_ptr<DensityFunction> abs(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Mapped>(std::move(input), MappedType::Abs);
}

std::unique_ptr<DensityFunction> square(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Mapped>(std::move(input), MappedType::Square);
}

std::unique_ptr<DensityFunction> cube(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Mapped>(std::move(input), MappedType::Cube);
}

std::unique_ptr<DensityFunction> halfNegative(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Mapped>(std::move(input), MappedType::HalfNegative);
}

std::unique_ptr<DensityFunction> quarterNegative(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Mapped>(std::move(input), MappedType::QuarterNegative);
}

std::unique_ptr<DensityFunction> squeeze(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Mapped>(std::move(input), MappedType::Squeeze);
}

std::unique_ptr<DensityFunction> invert(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Mapped>(std::move(input), MappedType::Invert);
}

std::unique_ptr<DensityFunction> add(std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2)
{
    return std::make_unique<TwoArgument>(std::move(arg1), std::move(arg2), TwoArgumentType::Add);
}

std::unique_ptr<DensityFunction> mul(std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2)
{
    return std::make_unique<TwoArgument>(std::move(arg1), std::move(arg2), TwoArgumentType::Mul);
}

std::unique_ptr<DensityFunction> min(std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2)
{
    return std::make_unique<TwoArgument>(std::move(arg1), std::move(arg2), TwoArgumentType::Min);
}

std::unique_ptr<DensityFunction> max(std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2)
{
    return std::make_unique<TwoArgument>(std::move(arg1), std::move(arg2), TwoArgumentType::Max);
}

std::unique_ptr<DensityFunction> noise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes, f64 xzScale, f64 yScale)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    return std::make_unique<NoiseDensity>(std::move(normalNoise), xzScale, yScale);
}

std::unique_ptr<DensityFunction> shiftedNoise(u64 seed,
    i32 firstOctave,
    std::vector<f64> amplitudes,
    f64 xzScale,
    f64 yScale,
    std::unique_ptr<DensityFunction> shiftX,
    std::unique_ptr<DensityFunction> shiftY,
    std::unique_ptr<DensityFunction> shiftZ)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    return std::make_unique<ShiftedNoise>(
        std::move(normalNoise), xzScale, yScale, std::move(shiftX), std::move(shiftY), std::move(shiftZ));
}

std::unique_ptr<DensityFunction> shiftedNoise2d(std::unique_ptr<DensityFunction> shiftX,
    std::unique_ptr<DensityFunction> shiftZ,
    f64 xzScale,
    u64 seed,
    i32 firstOctave,
    std::vector<f64> amplitudes)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    auto zero = factory::constant(0.0);
    return std::make_unique<ShiftedNoise>(
        std::move(normalNoise), xzScale, 0.0, std::move(shiftX), std::move(zero), std::move(shiftZ));
}

std::unique_ptr<DensityFunction> lerp(std::unique_ptr<DensityFunction> delta,
    std::unique_ptr<DensityFunction> start,
    std::unique_ptr<DensityFunction> end)
{
    return std::make_unique<Lerp>(std::move(delta), std::move(start), std::move(end));
}

std::unique_ptr<DensityFunction> shiftA(u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    return std::make_unique<ShiftNoise>(std::move(normalNoise), ShiftType::ShiftA);
}

std::unique_ptr<DensityFunction> shiftB(u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    return std::make_unique<ShiftNoise>(std::move(normalNoise), ShiftType::ShiftB);
}

std::unique_ptr<DensityFunction> shift(u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    return std::make_unique<ShiftNoise>(std::move(normalNoise), ShiftType::Shift);
}

std::unique_ptr<DensityFunction> rangeChoice(std::unique_ptr<DensityFunction> input,
    f64 minInclusive,
    f64 maxExclusive,
    std::unique_ptr<DensityFunction> whenInRange,
    std::unique_ptr<DensityFunction> whenOutOfRange)
{
    return std::make_unique<RangeChoice>(
        std::move(input), minInclusive, maxExclusive, std::move(whenInRange), std::move(whenOutOfRange));
}

std::unique_ptr<DensityFunction> cubicSpline(std::shared_ptr<DensityFunction> input, std::vector<SplinePoint> points)
{
    return std::make_unique<CubicSpline>(std::move(input), std::move(points));
}

std::unique_ptr<DensityFunction> spline(std::shared_ptr<DensityFunction> input, std::vector<FlatSplinePoint> flatPoints)
{
    // 将扁平样条点转换为嵌套样条点（value 全部为 f64 常量）
    std::vector<SplinePoint> points;
    points.reserve(flatPoints.size());
    for (const auto& fp : flatPoints) {
        SplinePoint sp;
        sp.location = fp.location;
        sp.value = fp.value;
        sp.derivative = fp.derivative;
        points.push_back(std::move(sp));
    }
    return std::make_unique<CubicSpline>(std::move(input), std::move(points));
}

std::unique_ptr<DensityFunction> spline(std::unique_ptr<DensityFunction> input, std::vector<FlatSplinePoint> flatPoints)
{
    auto sharedInput = std::shared_ptr<DensityFunction>(std::move(input));
    return spline(std::move(sharedInput), std::move(flatPoints));
}

std::unique_ptr<DensityFunction> cache2D(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<Cache2D>(std::move(input));
}

std::unique_ptr<DensityFunction> flatCache(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<FlatCache>(std::move(input));
}

std::unique_ptr<DensityFunction> cacheAllInCell(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<CacheAllInCell>(std::move(input));
}

std::unique_ptr<DensityFunction> weirdScaledSampler(std::unique_ptr<DensityFunction> input,
    u64 seed,
    i32 firstOctave,
    std::vector<f64> amplitudes,
    WeirdScaledSamplerType type)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    return std::make_unique<WeirdScaledSampler>(std::move(input), std::move(normalNoise), type);
}

std::unique_ptr<DensityFunction> endIslands(u64 seed)
{
    return std::make_unique<EndIslands>(seed);
}

std::unique_ptr<DensityFunction> blendedNoise(
    u64 seed, f64 xzScale, f64 yScale, f64 xzFactor, f64 yFactor, f64 smearScaleMultiplier)
{
    return std::make_unique<BlendedNoise>(seed, xzScale, yScale, xzFactor, yFactor, smearScaleMultiplier);
}

std::unique_ptr<DensityFunction> mappedNoise(
    u64 seed, i32 firstOctave, std::vector<f64> amplitudes, f64 xzScale, f64 yScale, f64 fromValue, f64 toValue)
{
    auto normalNoise = std::make_unique<noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
    return std::make_unique<MappedNoise>(std::move(normalNoise), xzScale, yScale, fromValue, toValue);
}

std::unique_ptr<DensityFunction> interpolated(std::unique_ptr<DensityFunction> wrapped)
{
    return std::make_unique<Marker>(MarkerType::Interpolated, std::move(wrapped));
}

std::unique_ptr<DensityFunction> cacheOnce(std::unique_ptr<DensityFunction> wrapped)
{
    return std::make_unique<Marker>(MarkerType::CacheOnce, std::move(wrapped));
}

std::unique_ptr<DensityFunction> cacheAllInCellMarker(std::unique_ptr<DensityFunction> wrapped)
{
    return std::make_unique<Marker>(MarkerType::CacheAllInCell, std::move(wrapped));
}

std::unique_ptr<DensityFunction> flatCacheMarker(std::unique_ptr<DensityFunction> wrapped)
{
    return std::make_unique<Marker>(MarkerType::FlatCache, std::move(wrapped));
}

std::unique_ptr<DensityFunction> cache2DMarker(std::unique_ptr<DensityFunction> wrapped)
{
    return std::make_unique<Marker>(MarkerType::Cache2D, std::move(wrapped));
}

std::unique_ptr<DensityFunction> sharedHolder(std::unique_ptr<DensityFunction> input)
{
    return std::make_unique<SharedHolder>(std::shared_ptr<DensityFunction>(std::move(input)));
}

std::unique_ptr<DensityFunction> sharedHolder(std::shared_ptr<DensityFunction> shared)
{
    return std::make_unique<SharedHolder>(std::move(shared));
}

} // namespace factory

} // namespace mc::world::gen::density
