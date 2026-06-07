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
    , m_islandNoise(
          std::make_unique<noise::NormalNoise>(seed ^ 0x9E3779B97F4A7C15ULL, -4, std::vector<f64>{1.0, 1.0, 1.0, 1.0}))
{}

f64 EndIslands::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    MC_UNUSED(blockY);

    // MC 1.21: 采样网格为 8 格间距（不是 100）
    const i32 sx = blockX >> 3; // blockX / 8
    const i32 sz = blockZ >> 3; // blockZ / 8

    const f64 height = getHeightValue(sx, sz);
    return (height - 8.0) / 128.0;
}

f64 EndIslands::getHeightValue(i32 x, i32 z) const
{
    // MC 1.21 EndIslands.getHeightValue
    const i32 i = x >> 1;
    const i32 j = z >> 1;
    const i32 k = x & 1; // x % 2
    const i32 l = z & 1; // z % 2

    // 基础高度：根据到原点的距离
    f64 f = 100.0 - std::sqrt(static_cast<f64>(x * x + z * z)) * 8.0;
    f = std::clamp(f, -100.0, 80.0);

    // 检测周围的岛屿（-12 到 +12 范围内）
    for (i32 i1 = -12; i1 <= 12; ++i1) {
        for (i32 j1 = -12; j1 <= 12; ++j1) {
            const i64 k1 = static_cast<i64>(i) + i1;
            const i64 l1 = static_cast<i64>(j) + j1;

            // 跳过中心区域（主岛）和太远的区域
            if (k1 * k1 + l1 * l1 > 4096L) {
                continue;
            }

            // 检查此位置是否有岛屿（噪声阈值 < -0.9）
            const f64 noiseVal = m_islandNoise->getValue(static_cast<f64>(k1), 0.0, static_cast<f64>(l1));
            if (noiseVal < -0.9) {
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

std::unique_ptr<DensityFunction> spline(std::unique_ptr<DensityFunction> input, std::vector<SplinePoint> points)
{
    return std::make_unique<Spline>(std::move(input), std::move(points));
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

} // namespace factory

} // namespace mc::world::gen::density
