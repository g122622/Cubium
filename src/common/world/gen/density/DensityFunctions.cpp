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
    : m_islandNoise(std::make_unique<noise::NormalNoise>(seed, -4, std::vector<f64>{1.0, 1.0, 1.0, 1.0}))
{}

f64 EndIslands::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    MC_UNUSED(blockY);

    // 末地岛屿使用 100 格间距的采样
    const i32 samplingScale = 100;
    const i32 sx = blockX / samplingScale;
    const i32 sz = blockZ / samplingScale;

    // 计算岛屿高度
    const f64 height = getHeight(sx, sz);

    // 采样岛屿噪声
    const f64 noise = m_islandNoise->getValue(static_cast<f64>(blockX) / 100.0, 0.0, static_cast<f64>(blockZ) / 100.0);

    // 如果在岛屿区域内，返回正值
    if (height > 0.0) {
        return height + noise * 0.2;
    }

    return noise;
}

f64 EndIslands::getHeight(i32 x, i32 z) const
{
    // MC 1.21 末地岛屿高度计算
    // 简化实现：基于噪声值计算岛屿是否存在
    const f64 d = static_cast<f64>(x * x + z * z);
    if (d < 1.0) {
        return 1.0;
    }
    return 0.0;
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

} // namespace factory

} // namespace mc::world::gen::density
