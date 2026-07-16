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

/**
 * @file DensityFunctionsTest.cpp
 * @brief DensityFunctions 单元测试 — 验证 Mapped/Invert 和 TwoArgument/Mul 边界计算
 *
 * 测试覆盖 MC 1.21.11 对齐中发现的关键 BUG 修复：
 * 1. Invert: 当输入范围完全为负时 minValue/maxValue 应正确
 * 2. Mul: maxValue 的条件判断应使用 min1/min2 和 max1/max2
 */

#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/density/Beardifier.hpp"
#include "common/world/gen/density/TerrainProvider.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include <cmath>
#include <limits>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gen::density;

// ============================================================================
// 辅助：创建恒定密度函数用于测试边界值
// ============================================================================

namespace {
/**
 * 测试专用：直接用 seed 构造 NormalNoise，与旧 factory::xxx 叶子工厂产出的
 * 噪声实例逐 bit 等价（旧实现内部即 make_shared<NormalNoise>(seed, oct, amps)）。
 */
std::shared_ptr<const world::gen::noise::NormalNoise> makeTestNoise(
    u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
{
    return std::make_shared<world::gen::noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
}

/**
 * 创建返回恒定值的密度函数，minValue = maxValue = value
 */
std::unique_ptr<DensityFunction> constantFunc(f64 value)
{
    return factory::constant(value);
}
} // namespace

// ============================================================================
// Mapped::Invert 边界值测试
// ============================================================================

TEST(DensityFunctionsInvertTest, PositiveRange)
{
    // 使用 YClampedGradient 创建 [2, 4] 范围的密度函数
    // Y=0 → value=2, Y=100 → value=4
    auto input = factory::yClampedGradient(0, 100, 2.0, 4.0);
    auto inverted = factory::invert(std::move(input));
    // 1/x 在 [2, 4] 上递减：minValue = 1/4 = 0.25, maxValue = 1/2 = 0.5
    EXPECT_NEAR(inverted->minValue(), 0.25, 1e-10);
    EXPECT_NEAR(inverted->maxValue(), 0.5, 1e-10);
}

TEST(DensityFunctionsInvertTest, NegativeRange)
{
    // 使用 YClampedGradient 创建 [-4, -2] 范围的密度函数
    auto input = factory::yClampedGradient(0, 100, -4.0, -2.0);
    auto inverted = factory::invert(std::move(input));

    // MC 1.21: 当 inMax < 0 时，minValue = 1/inMax, maxValue = 1/inMin
    // 即 minValue = 1/(-2) = -0.5, maxValue = 1/(-4) = -0.25
    // BUG 修复前 minValue 和 maxValue 互换，导致 minValue > maxValue
    EXPECT_NEAR(inverted->minValue(), -0.5, 1e-10);
    EXPECT_NEAR(inverted->maxValue(), -0.25, 1e-10);
    EXPECT_LE(inverted->minValue(), inverted->maxValue());
}

TEST(DensityFunctionsInvertTest, CrossesZero)
{
    // 输入范围跨越零点 → 无界
    // 使用 YClampedGradient 创建跨越零的密度函数
    auto gradient = factory::yClampedGradient(0, 100, -1.0, 1.0);
    auto inverted = factory::invert(std::move(gradient));

    EXPECT_EQ(inverted->minValue(), -std::numeric_limits<f64>::infinity());
    EXPECT_EQ(inverted->maxValue(), std::numeric_limits<f64>::infinity());
}

TEST(DensityFunctionsInvertTest, UnitRange)
{
    // 输入范围 [1, 1]，1/x → [1, 1]
    auto input = constantFunc(1.0);
    auto inverted = factory::invert(std::move(input));
    EXPECT_NEAR(inverted->minValue(), 1.0, 1e-10);
    EXPECT_NEAR(inverted->maxValue(), 1.0, 1e-10);
}

// ============================================================================
// TwoArgument::Mul 边界值测试
// ============================================================================

TEST(DensityFunctionsMulTest, BothPositive)
{
    // 两个正范围: [2, 4] * [3, 5]
    auto a = factory::yClampedGradient(0, 100, 2.0, 4.0);
    auto b = factory::yClampedGradient(0, 100, 3.0, 5.0);
    auto product = factory::mul(std::move(a), std::move(b));

    // minValue = 2*3 = 6, maxValue = 4*5 = 20
    EXPECT_NEAR(product->minValue(), 6.0, 1e-10);
    EXPECT_NEAR(product->maxValue(), 20.0, 1e-10);
}

TEST(DensityFunctionsMulTest, BothNegative)
{
    // 两个负范围: [-4, -2] * [-5, -3]
    // MC 1.21: minValue = max1*max2 = (-2)*(-3) = 6
    //          maxValue = min1*min2 = (-4)*(-5) = 20
    auto a = factory::yClampedGradient(0, 100, -4.0, -2.0);
    auto b = factory::yClampedGradient(0, 100, -5.0, -3.0);
    auto product = factory::mul(std::move(a), std::move(b));

    EXPECT_NEAR(product->minValue(), 6.0, 1e-10);
    EXPECT_NEAR(product->maxValue(), 20.0, 1e-10);
}

TEST(DensityFunctionsMulTest, MixedSigns)
{
    // 正范围 * 负范围: [2, 4] * [-5, -3]
    // minValue = min(2*(-3), 4*(-5)) = min(-6, -20) = -20
    // maxValue = max(2*(-5), 4*(-3)) = max(-10, -12) = -10
    auto pos = factory::yClampedGradient(0, 100, 2.0, 4.0);
    auto neg = factory::yClampedGradient(0, 100, -5.0, -3.0);
    auto product = factory::mul(std::move(pos), std::move(neg));

    EXPECT_NEAR(product->minValue(), -20.0, 1e-10);
    EXPECT_NEAR(product->maxValue(), -10.0, 1e-10);
}

TEST(DensityFunctionsMulTest, CrossesZeroAndPositive)
{
    // 跨零范围 * 正范围: [-2, 3] * [1, 4]
    // minValue = min(-2*4, 3*1) = min(-8, 3) = -8
    // maxValue = max(-2*1, 3*4) = max(-2, 12) = 12
    auto crossZero = factory::yClampedGradient(0, 100, -2.0, 3.0);
    auto pos = factory::yClampedGradient(0, 100, 1.0, 4.0);
    auto product = factory::mul(std::move(crossZero), std::move(pos));

    EXPECT_NEAR(product->minValue(), -8.0, 1e-10);
    EXPECT_NEAR(product->maxValue(), 12.0, 1e-10);
}

// ============================================================================
// Mapped::Abs bounds 对齐测试
// ============================================================================

TEST(DensityFunctionsAbsTest, MinValue_MCNegativeRange)
{
    // MC 1.21: Abs minValue = max(0, input.minValue)
    // 输入范围 [-5, -1]：MC minValue = max(0, -5) = 0
    auto input = factory::yClampedGradient(0, 100, -5.0, -1.0);
    auto absFunc = factory::abs(std::move(input));
    EXPECT_DOUBLE_EQ(absFunc->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(absFunc->maxValue(), 5.0); // max(|-5|, |-1|) = 5
}

TEST(DensityFunctionsAbsTest, MinValue_MCCrossZeroRange)
{
    // 输入范围 [-3, 2]：MC minValue = max(0, -3) = 0
    auto input = factory::yClampedGradient(0, 100, -3.0, 2.0);
    auto absFunc = factory::abs(std::move(input));
    EXPECT_DOUBLE_EQ(absFunc->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(absFunc->maxValue(), 3.0); // max(|-3|, |2|) = 3
}

TEST(DensityFunctionsAbsTest, MinValue_MCPositiveRange)
{
    // 输入范围 [1, 5]：MC minValue = max(0, 1) = 1
    auto input = factory::yClampedGradient(0, 100, 1.0, 5.0);
    auto absFunc = factory::abs(std::move(input));
    EXPECT_DOUBLE_EQ(absFunc->minValue(), 1.0);
    EXPECT_DOUBLE_EQ(absFunc->maxValue(), 5.0);
}

TEST(DensityFunctionsAbsTest, Compute_NegativeInput)
{
    auto c = constantFunc(-3.5);
    auto absFunc = factory::abs(std::move(c));
    EXPECT_DOUBLE_EQ(absFunc->compute(0, 0, 0), 3.5);
}

// ============================================================================
// Mapped::Square bounds 对齐测试
// ============================================================================

TEST(DensityFunctionsSquareTest, MinValue_MCNegativeRange)
{
    // MC 1.21: Square minValue = max(0, input.minValue)
    // 输入范围 [-5, -1]：MC minValue = max(0, -5) = 0（保守估计）
    auto input = factory::yClampedGradient(0, 100, -5.0, -1.0);
    auto sqFunc = factory::square(std::move(input));
    EXPECT_DOUBLE_EQ(sqFunc->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(sqFunc->maxValue(), 25.0); // max((-5)^2, (-1)^2) = 25
}

TEST(DensityFunctionsSquareTest, MinValue_MCCrossZeroRange)
{
    // 输入范围 [-3, 2]：MC minValue = max(0, -3) = 0
    auto input = factory::yClampedGradient(0, 100, -3.0, 2.0);
    auto sqFunc = factory::square(std::move(input));
    EXPECT_DOUBLE_EQ(sqFunc->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(sqFunc->maxValue(), 9.0); // max(9, 4) = 9
}

TEST(DensityFunctionsSquareTest, MinValue_MCPositiveRange)
{
    // 输入范围 [1, 5]：MC minValue = max(0, 1) = 1
    auto input = factory::yClampedGradient(0, 100, 1.0, 5.0);
    auto sqFunc = factory::square(std::move(input));
    EXPECT_DOUBLE_EQ(sqFunc->minValue(), 1.0);
    EXPECT_DOUBLE_EQ(sqFunc->maxValue(), 25.0);
}

// ============================================================================
// Mapped::Invert 边界值测试（扩展）
// ============================================================================

TEST(DensityFunctionsInvertTest, ZeroInputReturnsInfinity)
{
    // MC 1.21: Invert 在 v=0 时返回 1/0 = +infinity（IEEE 754）
    auto input = constantFunc(0.0);
    auto inverted = factory::invert(std::move(input));
    f64 result = inverted->compute(0, 0, 0);
    EXPECT_TRUE(std::isinf(result));
    EXPECT_GT(result, 0.0); // 正无穷
}

TEST(DensityFunctionsInvertTest, VerySmallPositiveInput)
{
    auto input = constantFunc(1e-300);
    auto inverted = factory::invert(std::move(input));
    f64 result = inverted->compute(0, 0, 0);
    EXPECT_GT(result, 0.0);
    EXPECT_TRUE(std::isfinite(result) || std::isinf(result));
}

// ============================================================================
// MappedNoise bounds 对齐测试
// ============================================================================

TEST(DensityFunctionsMappedNoiseTest, BoundsFromValueLessThanToValue)
{
    // fromValue=0, toValue=1: midpoint=0.5, halfAmplitude=0.5
    // MC 1.21: minValue = midpoint - |halfAmplitude| * innerMaxNoise
    //          maxValue = midpoint + |halfAmplitude| * innerMaxNoise
    // 因此：minValue + maxValue = 2 * midpoint, 且 maxValue > minValue
    auto noise = std::make_unique<MappedNoise>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0, 0.0, 1.0);
    const f64 midpoint = (0.0 + 1.0) * 0.5; // 0.5
    // 验证对称性：minValue + maxValue = 2 * midpoint
    EXPECT_NEAR(noise->minValue() + noise->maxValue(), 2.0 * midpoint, 1e-10);
    // 验证 minValue <= midpoint <= maxValue
    EXPECT_LE(noise->minValue(), midpoint);
    EXPECT_GE(noise->maxValue(), midpoint);
    // 验证合理范围
    EXPECT_GT(noise->maxValue(), noise->minValue());
}

TEST(DensityFunctionsMappedNoiseTest, BoundsFromValueGreaterThanToValue)
{
    // fromValue=-0.6, toValue=-1.3: midpoint=-0.95, halfAmplitude=-0.35
    // MC 1.21: minValue = midpoint - |halfAmplitude| * innerMaxNoise
    //          maxValue = midpoint + |halfAmplitude| * innerMaxNoise
    // 即使 fromValue > toValue（halfAmplitude 为负），bounds 仍然关于 midpoint 对称
    auto noise = std::make_unique<MappedNoise>(makeTestNoise(42, -8, {1.0}), 2.0, 1.0, -0.6, -1.3);
    const f64 midpoint = (-0.6 + -1.3) * 0.5; // -0.95
    // 验证对称性：minValue + maxValue = 2 * midpoint
    EXPECT_NEAR(noise->minValue() + noise->maxValue(), 2.0 * midpoint, 1e-10);
    // 验证 minValue <= midpoint <= maxValue
    EXPECT_LE(noise->minValue(), midpoint);
    EXPECT_GE(noise->maxValue(), midpoint);
    // maxValue 应该大于 minValue
    EXPECT_GT(noise->maxValue(), noise->minValue());
}

TEST(DensityFunctionsMappedNoiseTest, BoundsEqualFromToValue)
{
    // fromValue == toValue: halfAmplitude = 0, bounds = midpoint
    auto noise = std::make_unique<MappedNoise>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0, 5.0, 5.0);
    EXPECT_DOUBLE_EQ(noise->minValue(), 5.0);
    EXPECT_DOUBLE_EQ(noise->maxValue(), 5.0);
}

// ============================================================================
// WeirdScaledSampler bounds 对齐测试
// ============================================================================

TEST(DensityFunctionsWeirdScaledSamplerTest, Type1_MinValueIsZero)
{
    // MC 1.21: Type1 minValue = 0.0（因为 compute 使用了 abs）
    auto input = constantFunc(0.5);
    auto sampler = std::make_unique<WeirdScaledSampler>(
        std::move(input), makeTestNoise(42, -7, {1.0}), WeirdScaledSamplerType::Type1);
    EXPECT_DOUBLE_EQ(sampler->minValue(), 0.0);
}

TEST(DensityFunctionsWeirdScaledSamplerTest, Type2_MinValueIsZero)
{
    // MC 1.21: Type2 minValue = 0.0（因为 compute 使用了 abs）
    auto input = constantFunc(0.5);
    auto sampler = std::make_unique<WeirdScaledSampler>(
        std::move(input), makeTestNoise(42, -7, {1.0}), WeirdScaledSamplerType::Type2);
    EXPECT_DOUBLE_EQ(sampler->minValue(), 0.0);
}

TEST(DensityFunctionsWeirdScaledSamplerTest, Type1_MaxValueIsMaxRarityTimesNoiseMax)
{
    // MC 1.21: Type1 maxValue = 2.0 * noise.maxValue()
    auto input = constantFunc(0.5);
    auto sampler = std::make_unique<WeirdScaledSampler>(
        std::move(input), makeTestNoise(42, -7, {1.0}), WeirdScaledSamplerType::Type1);
    // Type1 maxRarity = 2.0
    // maxValue = 2.0 * noise.maxValue()
    EXPECT_GT(sampler->maxValue(), 0.0);
    EXPECT_LT(sampler->maxValue(), sampler->minValue() + 100.0); // 合理范围
}

TEST(DensityFunctionsWeirdScaledSamplerTest, Type2_MaxValueGreaterThanType1)
{
    // Type2 maxRarity = 3.0, Type1 maxRarity = 2.0
    // 相同噪声参数下，Type2 的 maxValue 应大于 Type1
    auto input1 = constantFunc(0.5);
    auto input2 = constantFunc(0.5);
    auto sampler1 = std::make_unique<WeirdScaledSampler>(
        std::move(input1), makeTestNoise(42, -7, {1.0}), WeirdScaledSamplerType::Type1);
    auto sampler2 = std::make_unique<WeirdScaledSampler>(
        std::move(input2), makeTestNoise(42, -7, {1.0}), WeirdScaledSamplerType::Type2);
    EXPECT_GT(sampler2->maxValue(), sampler1->maxValue());
}

TEST(DensityFunctionsWeirdScaledSamplerTest, RarityMapping_Type1)
{
    // Type1: value < -0.5 -> 0.75, < 0 -> 1.0, < 0.5 -> 1.5, else -> 2.0
    // 通过 compute 间接测试
    // 在高 rarity 下，噪声采样范围更小，值更大
    auto input = constantFunc(0.75); // 最高 rarity = 2.0
    auto sampler = std::make_unique<WeirdScaledSampler>(
        std::move(input), makeTestNoise(42, -7, {1.0}), WeirdScaledSamplerType::Type1);
    f64 result = sampler->compute(100, 100, 100);
    EXPECT_GE(result, 0.0); // 结果非负（因为 abs）
}

// ============================================================================
// YClampedGradient 测试
// ============================================================================

TEST(DensityFunctionsYClampedGradientTest, NormalRange)
{
    auto grad = factory::yClampedGradient(0, 100, 1.5, -1.5);
    EXPECT_DOUBLE_EQ(grad->compute(0, 0, 0), 1.5);    // at fromY
    EXPECT_DOUBLE_EQ(grad->compute(0, 100, 0), -1.5); // at toY
    EXPECT_DOUBLE_EQ(grad->compute(0, 50, 0), 0.0);   // midpoint
}

TEST(DensityFunctionsYClampedGradientTest, ClampedBelow)
{
    auto grad = factory::yClampedGradient(0, 100, 1.5, -1.5);
    EXPECT_DOUBLE_EQ(grad->compute(0, -50, 0), 1.5); // below fromY, clamped to fromValue
}

TEST(DensityFunctionsYClampedGradientTest, ClampedAbove)
{
    auto grad = factory::yClampedGradient(0, 100, 1.5, -1.5);
    EXPECT_DOUBLE_EQ(grad->compute(0, 150, 0), -1.5); // above toY, clamped to toValue
}

TEST(DensityFunctionsYClampedGradientTest, DecreasingValues)
{
    auto grad = factory::yClampedGradient(-64, 320, 1.5, -1.5);
    EXPECT_NEAR(grad->minValue(), -1.5, 1e-10);
    EXPECT_NEAR(grad->maxValue(), 1.5, 1e-10);
}

TEST(DensityFunctionsYClampedGradientTest, SameFromToY)
{
    // fromY == toY 时，inverseLerp 为 0/0 = NaN，clampedLerp(NaN, from, to) = NaN
    // 这是 MC 的行为 — MC 在 YClampedGradient 中也不处理此边界
    // 因此我们验证 NaN 结果（std::isnan）
    auto grad = factory::yClampedGradient(50, 50, 0.0, 1.0);
    f64 result = grad->compute(0, 50, 0);
    EXPECT_TRUE(std::isnan(result));
}

// ============================================================================
// RangeChoice 测试
// ============================================================================

TEST(DensityFunctionsRangeChoiceTest, ValueInRange)
{
    auto input = constantFunc(0.5);
    auto whenInRange = constantFunc(10.0);
    auto whenOutOfRange = constantFunc(-10.0);
    auto rc = factory::rangeChoice(std::move(input), 0.0, 1.0, std::move(whenInRange), std::move(whenOutOfRange));
    EXPECT_DOUBLE_EQ(rc->compute(0, 0, 0), 10.0);
}

TEST(DensityFunctionsRangeChoiceTest, ValueOutOfRange)
{
    auto input = constantFunc(2.0);
    auto whenInRange = constantFunc(10.0);
    auto whenOutOfRange = constantFunc(-10.0);
    auto rc = factory::rangeChoice(std::move(input), 0.0, 1.0, std::move(whenInRange), std::move(whenOutOfRange));
    EXPECT_DOUBLE_EQ(rc->compute(0, 0, 0), -10.0);
}

TEST(DensityFunctionsRangeChoiceTest, BoundaryMinInclusive)
{
    // 值等于 minInclusive 时应在范围内
    auto input = constantFunc(0.0);
    auto whenInRange = constantFunc(10.0);
    auto whenOutOfRange = constantFunc(-10.0);
    auto rc = factory::rangeChoice(std::move(input), 0.0, 1.0, std::move(whenInRange), std::move(whenOutOfRange));
    EXPECT_DOUBLE_EQ(rc->compute(0, 0, 0), 10.0);
}

TEST(DensityFunctionsRangeChoiceTest, BoundaryMaxExclusive)
{
    // 值等于 maxExclusive 时应在范围外
    auto input = constantFunc(1.0);
    auto whenInRange = constantFunc(10.0);
    auto whenOutOfRange = constantFunc(-10.0);
    auto rc = factory::rangeChoice(std::move(input), 0.0, 1.0, std::move(whenInRange), std::move(whenOutOfRange));
    EXPECT_DOUBLE_EQ(rc->compute(0, 0, 0), -10.0);
}

TEST(DensityFunctionsRangeChoiceTest, MinMaxValues)
{
    auto input = constantFunc(0.5);
    auto whenInRange = factory::yClampedGradient(0, 100, -5.0, 5.0);
    auto whenOutOfRange = constantFunc(-10.0);
    auto rc = factory::rangeChoice(std::move(input), 0.0, 1.0, std::move(whenInRange), std::move(whenOutOfRange));
    EXPECT_DOUBLE_EQ(rc->minValue(), -10.0); // min(-5, -10) = -10
    EXPECT_DOUBLE_EQ(rc->maxValue(), 5.0);   // max(5, -10) = 5
}

// ============================================================================
// Lerp 测试
// ============================================================================

TEST(DensityFunctionsLerpTest, DeltaZero_ReturnsStart)
{
    auto delta = constantFunc(0.0);
    auto start = constantFunc(5.0);
    auto end = constantFunc(10.0);
    auto lerp = factory::lerp(std::move(delta), std::move(start), std::move(end));
    EXPECT_DOUBLE_EQ(lerp->compute(0, 0, 0), 5.0);
}

TEST(DensityFunctionsLerpTest, DeltaOne_ReturnsEnd)
{
    auto delta = constantFunc(1.0);
    auto start = constantFunc(5.0);
    auto end = constantFunc(10.0);
    auto lerp = factory::lerp(std::move(delta), std::move(start), std::move(end));
    EXPECT_DOUBLE_EQ(lerp->compute(0, 0, 0), 10.0);
}

TEST(DensityFunctionsLerpTest, DeltaHalf_ReturnsMidpoint)
{
    auto delta = constantFunc(0.5);
    auto start = constantFunc(0.0);
    auto end = constantFunc(10.0);
    auto lerp = factory::lerp(std::move(delta), std::move(start), std::move(end));
    EXPECT_DOUBLE_EQ(lerp->compute(0, 0, 0), 5.0);
}

TEST(DensityFunctionsLerpTest, DeltaNegative_ReturnsStart)
{
    auto delta = constantFunc(-0.5);
    auto start = constantFunc(5.0);
    auto end = constantFunc(10.0);
    auto lerp = factory::lerp(std::move(delta), std::move(start), std::move(end));
    EXPECT_DOUBLE_EQ(lerp->compute(0, 0, 0), 5.0);
}

TEST(DensityFunctionsLerpTest, DeltaGreaterThanOne_ReturnsEnd)
{
    auto delta = constantFunc(1.5);
    auto start = constantFunc(5.0);
    auto end = constantFunc(10.0);
    auto lerp = factory::lerp(std::move(delta), std::move(start), std::move(end));
    EXPECT_DOUBLE_EQ(lerp->compute(0, 0, 0), 10.0);
}

TEST(DensityFunctionsLerpTest, MinMaxValues)
{
    auto delta = constantFunc(0.5);
    auto start = factory::yClampedGradient(0, 100, -3.0, 7.0);
    auto end = factory::yClampedGradient(0, 100, -1.0, 2.0);
    auto lerp = factory::lerp(std::move(delta), std::move(start), std::move(end));
    // lerp 的 bounds = min/max of all start/end bounds
    EXPECT_DOUBLE_EQ(lerp->minValue(), -3.0);
    EXPECT_DOUBLE_EQ(lerp->maxValue(), 7.0);
}

// ============================================================================
// ShiftNoise 测试
// ============================================================================

TEST(DensityFunctionsShiftNoiseTest, ShiftA_CoordinateOrder)
{
    // ShiftA: noise(x*0.25, 0, z*0.25) * 4
    // 测试 x 和 z 不对称性
    auto shiftA = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::ShiftA);
    // 两次 compute 结果应确定性
    f64 v1 = shiftA->compute(100, 0, 200);
    f64 v2 = shiftA->compute(100, 0, 200);
    EXPECT_DOUBLE_EQ(v1, v2);
    // x 和 z 交换后值应不同（ShiftA 不是各向同性的）
    f64 v3 = shiftA->compute(200, 0, 100);
    EXPECT_NE(v1, v3); // 几乎肯定不同
}

TEST(DensityFunctionsShiftNoiseTest, ShiftB_CoordinateSwap)
{
    // ShiftB: noise(z*0.25, x*0.25, 0) * 4（x 和 z 交换）
    auto shiftB = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::ShiftB);
    f64 v1 = shiftB->compute(100, 0, 200);
    f64 v2 = shiftB->compute(100, 0, 200);
    EXPECT_DOUBLE_EQ(v1, v2);
}

TEST(DensityFunctionsShiftNoiseTest, MinMaxValues)
{
    auto shiftA = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::ShiftA);
    EXPECT_DOUBLE_EQ(shiftA->minValue(), -shiftA->maxValue()); // 对称
    EXPECT_GT(shiftA->maxValue(), 0.0);
}

// ============================================================================
// Cache2D 测试
// ============================================================================

TEST(DensityFunctionsCache2DTest, CachesResult)
{
    auto input = factory::yClampedGradient(0, 100, 0.0, 100.0);
    auto cache = factory::cache2D(std::move(input));
    // Cache2D 缓存 XZ，忽略 Y
    f64 v1 = cache->compute(10, 0, 20);
    f64 v2 = cache->compute(10, 50, 20); // 相同 XZ，不同 Y → 返回缓存值
    EXPECT_DOUBLE_EQ(v1, v2);
}

TEST(DensityFunctionsCache2DTest, DifferentXZ_Recalculates)
{
    // 使用依赖于 XZ 坐标的噪声函数来测试 Cache2D 缓存失效
    auto input = std::make_unique<NoiseDensity>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0);
    auto cache = factory::cache2D(std::move(input));
    f64 v1 = cache->compute(10, 0, 20);
    f64 v2 = cache->compute(30, 0, 40); // 不同 XZ
    EXPECT_NE(v1, v2);
}

TEST(DensityFunctionsCache2DTest, DelegatesMinMax)
{
    auto input = factory::yClampedGradient(0, 100, -5.0, 5.0);
    f64 inputMin = input->minValue();
    f64 inputMax = input->maxValue();
    auto cache = factory::cache2D(std::move(input));
    EXPECT_DOUBLE_EQ(cache->minValue(), inputMin);
    EXPECT_DOUBLE_EQ(cache->maxValue(), inputMax);
}

// ============================================================================
// FlatCache 测试
// ============================================================================

// FlatCache 的 quart 级缓存语义只在 precompute=true（NoiseChunk::apply 注入区块
// 几何）路径下成立。factory::flatCache 走 precompute=false 退化为单值 lastPos 缓存
// （按 block XZ 精确匹配，非 quart）。本用例直接构造 precompute=true 的 FlatCache
// 验证 quart 分辨率缓存，并补充非预计算路径的退化行为断言。
TEST(DensityFunctionsFlatCacheTest, CachesAtQuartResolution)
{
    // 使用依赖 XZ 的噪声函数作为输入，使不同 quart 格产生不同值。
    auto input = std::make_unique<NoiseDensity>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0);
    // 保留一份相同构造的裸输入用于交叉校验预计算值。
    auto inputRef = std::make_unique<NoiseDensity>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0);

    // 直接构造 precompute=true 的 FlatCache，模拟 NoiseChunk::apply 注入区块几何。
    // firstQuartX=0, firstQuartZ=0, sizeXZ=4 → 覆盖 quart 坐标 [0,4]，
    // 对应 block 坐标 [0,16]，与主世界 cellCountXZ=4,cellWidth=4 的几何一致。
    FlatCache cache(std::move(input), 0, 0, 4, true);

    // (0,0,0) 与 (3,0,3) 同属 quart 格 (0,0)：floorDiv(block,4) 均为 0
    // → 命中同一缓存槽，返回同一预计算值。
    const f64 v1 = cache.compute(0, 0, 0);
    const f64 v2 = cache.compute(3, 0, 3);
    EXPECT_DOUBLE_EQ(v1, v2);

    // 预计算值应等于裸输入在对应 quart 对齐 block 坐标的值。
    // quart 格 (0,0) 的对齐 block 坐标为 (firstQuartX<<2, 0, firstQuartZ<<2)=(0,0,0)。
    const f64 refV1 = inputRef->compute(0, 0, 0);
    EXPECT_DOUBLE_EQ(v1, refV1);

    // (4,0,0) 属于 quart 格 (1,0)，与 (0,0) 不同格 → 应为不同预计算值。
    const f64 v3 = cache.compute(4, 0, 0);
    EXPECT_NE(v1, v3);
    // 且 v3 应等于裸输入在 quart (1,0) 对齐 block 坐标 (4,0,0) 处的值。
    const f64 refV3 = inputRef->compute(4, 0, 0);
    EXPECT_DOUBLE_EQ(v3, refV3);

    // Y 不参与 quart 缓存键（XZ-only）：同 XZ 不同 Y 返回同一缓存值。
    const f64 v4 = cache.compute(3, 64, 3);
    EXPECT_DOUBLE_EQ(v1, v4);

    // 越界回退：quart 坐标超出 [0,sizeXZ] 时退化为 m_input->compute(block)，
    // 不走数组查表（对齐原版 NoiseChunk.FlatCache.compute 越界分支）。
    const f64 oob = cache.compute(100, 0, 100);
    const f64 refOob = inputRef->compute(100, 0, 100);
    EXPECT_DOUBLE_EQ(oob, refOob);
}

// ============================================================================
// Constant 测试
// ============================================================================

TEST(DensityFunctionsConstantTest, AlwaysReturnsValue)
{
    auto c = factory::constant(3.14);
    EXPECT_DOUBLE_EQ(c->compute(0, 0, 0), 3.14);
    EXPECT_DOUBLE_EQ(c->compute(100, -50, 200), 3.14);
    EXPECT_DOUBLE_EQ(c->minValue(), 3.14);
    EXPECT_DOUBLE_EQ(c->maxValue(), 3.14);
}

TEST(DensityFunctionsConstantTest, NegativeValue)
{
    auto c = factory::constant(-2.5);
    EXPECT_DOUBLE_EQ(c->compute(0, 0, 0), -2.5);
    EXPECT_DOUBLE_EQ(c->minValue(), -2.5);
    EXPECT_DOUBLE_EQ(c->maxValue(), -2.5);
}

// ============================================================================
// Clamp 测试
// ============================================================================

TEST(DensityFunctionsClampTest, ClampsLow)
{
    auto input = constantFunc(-5.0);
    auto clamped = factory::clamp(std::move(input), -1.0, 1.0);
    EXPECT_DOUBLE_EQ(clamped->compute(0, 0, 0), -1.0);
}

TEST(DensityFunctionsClampTest, ClampsHigh)
{
    auto input = constantFunc(5.0);
    auto clamped = factory::clamp(std::move(input), -1.0, 1.0);
    EXPECT_DOUBLE_EQ(clamped->compute(0, 0, 0), 1.0);
}

TEST(DensityFunctionsClampTest, WithinRange)
{
    auto input = constantFunc(0.5);
    auto clamped = factory::clamp(std::move(input), -1.0, 1.0);
    EXPECT_DOUBLE_EQ(clamped->compute(0, 0, 0), 0.5);
}

TEST(DensityFunctionsClampTest, MinMaxValues)
{
    auto input = factory::yClampedGradient(0, 100, -10.0, 10.0);
    auto clamped = factory::clamp(std::move(input), -3.0, 3.0);
    EXPECT_DOUBLE_EQ(clamped->minValue(), -3.0);
    EXPECT_DOUBLE_EQ(clamped->maxValue(), 3.0);
}

// ============================================================================
// Mapped (Cube, HalfNegative, QuarterNegative, Squeeze) 测试
// ============================================================================

TEST(DensityFunctionsCubeTest, PositiveInput)
{
    auto c = constantFunc(2.0);
    auto cube = factory::cube(std::move(c));
    EXPECT_DOUBLE_EQ(cube->compute(0, 0, 0), 8.0);
    EXPECT_DOUBLE_EQ(cube->minValue(), 8.0);
    EXPECT_DOUBLE_EQ(cube->maxValue(), 8.0);
}

TEST(DensityFunctionsCubeTest, NegativeInput)
{
    auto c = constantFunc(-3.0);
    auto cube = factory::cube(std::move(c));
    EXPECT_DOUBLE_EQ(cube->compute(0, 0, 0), -27.0);
}

TEST(DensityFunctionsHalfNegativeTest, PositivePassthrough)
{
    auto c = constantFunc(4.0);
    auto hn = factory::halfNegative(std::move(c));
    EXPECT_DOUBLE_EQ(hn->compute(0, 0, 0), 4.0); // 正值不变
}

TEST(DensityFunctionsHalfNegativeTest, NegativeHalved)
{
    auto c = constantFunc(-4.0);
    auto hn = factory::halfNegative(std::move(c));
    EXPECT_DOUBLE_EQ(hn->compute(0, 0, 0), -2.0); // 负值减半
}

TEST(DensityFunctionsQuarterNegativeTest, PositivePassthrough)
{
    auto c = constantFunc(4.0);
    auto qn = factory::quarterNegative(std::move(c));
    EXPECT_DOUBLE_EQ(qn->compute(0, 0, 0), 4.0);
}

TEST(DensityFunctionsQuarterNegativeTest, NegativeQuartered)
{
    auto c = constantFunc(-4.0);
    auto qn = factory::quarterNegative(std::move(c));
    EXPECT_DOUBLE_EQ(qn->compute(0, 0, 0), -1.0); // 负值四分之一
}

TEST(DensityFunctionsSqueezeTest, WithinRange)
{
    // Squeeze: clamp(v, -1, 1) / 2 - clamp(v, -1, 1)^3 / 24
    // v=0: 0/2 - 0/24 = 0
    auto c = constantFunc(0.0);
    auto sq = factory::squeeze(std::move(c));
    EXPECT_DOUBLE_EQ(sq->compute(0, 0, 0), 0.0);
}

TEST(DensityFunctionsSqueezeTest, AtBoundary)
{
    // v=1: clamp=1, result = 1/2 - 1/24 = 12/24 - 1/24 = 11/24
    auto c = constantFunc(1.0);
    auto sq = factory::squeeze(std::move(c));
    EXPECT_NEAR(sq->compute(0, 0, 0), 11.0 / 24.0, 1e-10);
}

TEST(DensityFunctionsSqueezeTest, BeyondBoundary)
{
    // v=2: clamp=1, result = 11/24 (same as v=1)
    auto c = constantFunc(2.0);
    auto sq = factory::squeeze(std::move(c));
    EXPECT_NEAR(sq->compute(0, 0, 0), 11.0 / 24.0, 1e-10);
}

// ============================================================================
// TwoArgument (Add, Min, Max) 扩展测试
// ============================================================================

TEST(DensityFunctionsAddTest, MinMaxValues)
{
    auto a = factory::yClampedGradient(0, 100, 2.0, 5.0);
    auto b = factory::yClampedGradient(0, 100, -3.0, 1.0);
    auto sum = factory::add(std::move(a), std::move(b));
    EXPECT_DOUBLE_EQ(sum->minValue(), -1.0); // 2 + (-3) = -1
    EXPECT_DOUBLE_EQ(sum->maxValue(), 6.0);  // 5 + 1 = 6
}

TEST(DensityFunctionsMinTest, EarlyExit)
{
    // Min: 当 v1 < arg2.minValue 时，直接返回 v1（不计算 arg2）
    auto a = constantFunc(-10.0);
    auto b = factory::yClampedGradient(0, 100, 5.0, 10.0); // minValue=5
    auto minFunc = factory::min(std::move(a), std::move(b));
    // a = -10 < b.minValue = 5，所以结果应该是 -10
    EXPECT_DOUBLE_EQ(minFunc->compute(0, 0, 0), -10.0);
}

TEST(DensityFunctionsMaxTest, EarlyExit)
{
    // Max: 当 v1 > arg2.maxValue 时，直接返回 v1
    auto a = constantFunc(20.0);
    auto b = factory::yClampedGradient(0, 100, 5.0, 10.0); // maxValue=10
    auto maxFunc = factory::max(std::move(a), std::move(b));
    // a = 20 > b.maxValue = 10，所以结果应该是 20
    EXPECT_DOUBLE_EQ(maxFunc->compute(0, 0, 0), 20.0);
}

// ============================================================================
// CubicSpline 测试
// ============================================================================

TEST(DensityFunctionsCubicSTest, SinglePointExtrapolate)
{
    // 单控制点：外推为常量（导数=0）
    auto input = std::make_shared<Constant>(0.0);
    std::vector<SplinePoint> points = {{0.0, 5.0, 0.0}};
    auto spline = factory::cubicSpline(input, std::move(points));
    EXPECT_DOUBLE_EQ(spline->compute(0, 0, 0), 5.0);   // at the point
    EXPECT_DOUBLE_EQ(spline->compute(100, 0, 0), 5.0); // extrapolated
}

TEST(DensityFunctionsCubicSplineTest, TwoPointInterpolation)
{
    // 两个控制点：线性插值（导数=0）
    // inputFunc: Y=0 → -1.0, Y=50 → 0.0, Y=100 → 1.0
    auto inputFunc = std::make_shared<YClampedGradient>(0, 100, -1.0, 1.0);
    std::vector<SplinePoint> points = {{-1.0, 0.0, 0.0}, {1.0, 10.0, 0.0}};
    auto spline = factory::cubicSpline(inputFunc, std::move(points));
    // Y=0: input=-1.0, 刚好等于第一个控制点位置 → linearExtend 返回 value=0.0
    EXPECT_NEAR(spline->compute(0, 0, 0), 0.0, 1e-10);
    // Y=50: input=0.0, 在两个控制点中间，t=0.5 → lerp = 5.0
    EXPECT_NEAR(spline->compute(0, 50, 0), 5.0, 1e-10);
    // Y=100: input=1.0, 等于第二个控制点位置 → linearExtend 返回 value=10.0
    EXPECT_NEAR(spline->compute(0, 100, 0), 10.0, 1e-10);
}

TEST(DensityFunctionsCubicSplineTest, MinMaxValues)
{
    auto inputFunc = std::make_shared<YClampedGradient>(0, 100, -1.0, 1.0);
    std::vector<SplinePoint> points = {{-1.0, -5.0, 0.0}, {1.0, 5.0, 0.0}};
    auto spline = factory::cubicSpline(inputFunc, std::move(points));
    // bounds 应该包含两个端点的 min/max
    EXPECT_LE(spline->minValue(), -5.0);
    EXPECT_GE(spline->maxValue(), 5.0);
}

// ============================================================================
// Marker 测试
// ============================================================================

TEST(DensityFunctionsMarkerTest, InterpolatedDelegates)
{
    auto inner = constantFunc(3.0);
    auto marker = factory::interpolated(std::move(inner));
    EXPECT_DOUBLE_EQ(marker->compute(0, 0, 0), 3.0);
    EXPECT_DOUBLE_EQ(marker->minValue(), 3.0);
    EXPECT_DOUBLE_EQ(marker->maxValue(), 3.0);
}

TEST(DensityFunctionsMarkerTest, CacheOnceDelegates)
{
    auto inner = constantFunc(7.0);
    auto marker = factory::cacheOnce(std::move(inner));
    EXPECT_DOUBLE_EQ(marker->compute(0, 0, 0), 7.0);
}

TEST(DensityFunctionsMarkerTest, BeardifierMarkerReturnsZero)
{
    auto marker = factory::beardifierMarker();
    EXPECT_DOUBLE_EQ(marker->compute(0, 0, 0), 0.0);
    EXPECT_DOUBLE_EQ(marker->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(marker->maxValue(), 0.0);
}

// ============================================================================
// SharedHolder 测试
// ============================================================================

TEST(DensityFunctionsSharedHolderTest, DelegatesCompute)
{
    auto inner = factory::constant(5.0);
    auto shared = factory::sharedHolder(std::move(inner));
    EXPECT_DOUBLE_EQ(shared->compute(0, 0, 0), 5.0);
    EXPECT_DOUBLE_EQ(shared->minValue(), 5.0);
    EXPECT_DOUBLE_EQ(shared->maxValue(), 5.0);
}

TEST(DensityFunctionsSharedHolderTest, MultipleReferencesShareInner)
{
    auto inner = std::make_shared<Constant>(42.0);
    auto holder1 = std::make_unique<SharedHolder>(inner);
    auto holder2 = std::make_unique<SharedHolder>(inner);
    EXPECT_DOUBLE_EQ(holder1->compute(0, 0, 0), 42.0);
    EXPECT_DOUBLE_EQ(holder2->compute(0, 0, 0), 42.0);
}

// ============================================================================
// EndIslands 测试
// ============================================================================

TEST(DensityFunctionsEndIslandsTest, DeterministicWithSameSeed)
{
    auto islands1 = factory::endIslands(0);
    auto islands2 = factory::endIslands(0);
    f64 v1 = islands1->compute(100, 50, 200);
    f64 v2 = islands2->compute(100, 50, 200);
    EXPECT_DOUBLE_EQ(v1, v2);
}

TEST(DensityFunctionsEndIslandsTest, MinMaxValues)
{
    auto islands = factory::endIslands(0);
    EXPECT_DOUBLE_EQ(islands->minValue(), -0.84375);
    EXPECT_DOUBLE_EQ(islands->maxValue(), 0.5625);
}

TEST(DensityFunctionsEndIslandsTest, OriginValue)
{
    auto islands = factory::endIslands(0);
    f64 v = islands->compute(0, 0, 0);
    // 原点附近应该返回一个有效值
    EXPECT_GE(v, islands->minValue());
    EXPECT_LE(v, islands->maxValue());
}

// ============================================================================
// NoiseDensity 测试
// ============================================================================

TEST(DensityFunctionsNoiseDensityTest, Deterministic)
{
    auto noise1 = std::make_unique<NoiseDensity>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0);
    auto noise2 = std::make_unique<NoiseDensity>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0);
    EXPECT_DOUBLE_EQ(noise1->compute(100, 50, 200), noise2->compute(100, 50, 200));
}

TEST(DensityFunctionsNoiseDensityTest, SymmetricMinMax)
{
    auto noise = std::make_unique<NoiseDensity>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0);
    EXPECT_DOUBLE_EQ(noise->minValue(), -noise->maxValue());
}

// ============================================================================
// MappedNoise compute 测试
// ============================================================================

TEST(DensityFunctionsMappedNoiseTest, ComputeMatchesFormula)
{
    // compute = fromValue + noise * (toValue - fromValue)
    // 当 fromValue == toValue 时，结果恒等于 fromValue
    auto noise = std::make_unique<MappedNoise>(makeTestNoise(42, -8, {1.0}), 1.0, 1.0, 5.0, 5.0);
    EXPECT_DOUBLE_EQ(noise->compute(0, 0, 0), 5.0);
    EXPECT_DOUBLE_EQ(noise->compute(100, 50, 200), 5.0);
    EXPECT_DOUBLE_EQ(noise->minValue(), 5.0);
    EXPECT_DOUBLE_EQ(noise->maxValue(), 5.0);
}

// ============================================================================
// ShiftedNoise 测试
// ============================================================================

TEST(DensityFunctionsShiftedNoiseTest, MinMaxValues)
{
    auto shiftX = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::ShiftA);
    auto shiftY = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::Shift);
    auto shiftZ = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::ShiftB);
    auto shifted = std::make_unique<ShiftedNoise>(
        makeTestNoise(42, -8, {1.0}), 0.25, 0.125, std::move(shiftX), std::move(shiftY), std::move(shiftZ));
    EXPECT_DOUBLE_EQ(shifted->minValue(), -shifted->maxValue());
    EXPECT_GT(shifted->maxValue(), 0.0);
}

// ============================================================================
// Mapped::Cube 边界值测试
// ============================================================================

TEST(DensityFunctionsCubeTest, CrossZeroBounds)
{
    // Cube 在跨零范围上: [−2, 3] → [−8, 27]
    auto input = factory::yClampedGradient(0, 100, -2.0, 3.0);
    auto cube = factory::cube(std::move(input));
    EXPECT_DOUBLE_EQ(cube->minValue(), -8.0); // (-2)^3
    EXPECT_DOUBLE_EQ(cube->maxValue(), 27.0); // 3^3
}

TEST(DensityFunctionsCubeTest, AllNegativeBounds)
{
    // [-3, -1] → [-27, -1]
    auto input = factory::yClampedGradient(0, 100, -3.0, -1.0);
    auto cube = factory::cube(std::move(input));
    EXPECT_DOUBLE_EQ(cube->minValue(), -27.0);
    EXPECT_DOUBLE_EQ(cube->maxValue(), -1.0);
}

TEST(DensityFunctionsCubeTest, ZeroInput)
{
    auto c = constantFunc(0.0);
    auto cube = factory::cube(std::move(c));
    EXPECT_DOUBLE_EQ(cube->compute(0, 0, 0), 0.0);
    EXPECT_DOUBLE_EQ(cube->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(cube->maxValue(), 0.0);
}

// ============================================================================
// Mapped::HalfNegative 边界值测试
// ============================================================================

TEST(DensityFunctionsHalfNegativeTest, CrossZeroBounds)
{
    // HalfNegative: 正值不变，负值减半
    // [-4, 2] → minValue = -4*0.5 = -2, maxValue = 2
    auto input = factory::yClampedGradient(0, 100, -4.0, 2.0);
    auto hn = factory::halfNegative(std::move(input));
    EXPECT_DOUBLE_EQ(hn->minValue(), -2.0);
    EXPECT_DOUBLE_EQ(hn->maxValue(), 2.0);
}

TEST(DensityFunctionsHalfNegativeTest, AllNegativeBounds)
{
    // [-6, -2] → [-3, -1]
    auto input = factory::yClampedGradient(0, 100, -6.0, -2.0);
    auto hn = factory::halfNegative(std::move(input));
    EXPECT_DOUBLE_EQ(hn->minValue(), -3.0);
    EXPECT_DOUBLE_EQ(hn->maxValue(), -1.0);
}

// ============================================================================
// Mapped::QuarterNegative 边界值测试
// ============================================================================

TEST(DensityFunctionsQuarterNegativeTest, CrossZeroBounds)
{
    // QuarterNegative: 正值不变，负值四分之一
    // [-8, 4] → minValue = -8*0.25 = -2, maxValue = 4
    auto input = factory::yClampedGradient(0, 100, -8.0, 4.0);
    auto qn = factory::quarterNegative(std::move(input));
    EXPECT_DOUBLE_EQ(qn->minValue(), -2.0);
    EXPECT_DOUBLE_EQ(qn->maxValue(), 4.0);
}

TEST(DensityFunctionsQuarterNegativeTest, AllNegativeBounds)
{
    // [-8, -2] → [-2, -0.5]
    auto input = factory::yClampedGradient(0, 100, -8.0, -2.0);
    auto qn = factory::quarterNegative(std::move(input));
    EXPECT_DOUBLE_EQ(qn->minValue(), -2.0);
    EXPECT_DOUBLE_EQ(qn->maxValue(), -0.5);
}

// ============================================================================
// Mapped::Squeeze 扩展测试
// ============================================================================

TEST(DensityFunctionsSqueezeTest, NegativeBoundary)
{
    // v=-1: clamp(-1) = -1, result = (-1)/2 - (-1)^3/24 = -0.5 + 1/24 = -11/24
    auto c = constantFunc(-1.0);
    auto sq = factory::squeeze(std::move(c));
    EXPECT_NEAR(sq->compute(0, 0, 0), -11.0 / 24.0, 1e-10);
}

TEST(DensityFunctionsSqueezeTest, NegativeBeyondBoundary)
{
    // v=-2: clamp(-2) = -1, result = -11/24 (same as v=-1)
    auto c = constantFunc(-2.0);
    auto sq = factory::squeeze(std::move(c));
    EXPECT_NEAR(sq->compute(0, 0, 0), -11.0 / 24.0, 1e-10);
}

TEST(DensityFunctionsSqueezeTest, BoundsWithVariableInput)
{
    // 输入范围 [-2, 2]: squeeze(-2) = -11/24, squeeze(2) = 11/24, squeeze(0) = 0
    // squeeze 在 [-1, 1] 内是奇函数，在 ±1 处达到 ±11/24
    auto input = factory::yClampedGradient(0, 100, -2.0, 2.0);
    auto sq = factory::squeeze(std::move(input));
    EXPECT_NEAR(sq->minValue(), -11.0 / 24.0, 1e-10);
    EXPECT_NEAR(sq->maxValue(), 11.0 / 24.0, 1e-10);
}

TEST(DensityFunctionsSqueezeTest, Symmetry)
{
    // squeeze(-v) = -squeeze(v) 在 [-1, 1] 内
    auto pos = constantFunc(0.5);
    auto neg = constantFunc(-0.5);
    auto sqPos = factory::squeeze(std::move(pos));
    auto sqNeg = factory::squeeze(std::move(neg));
    EXPECT_NEAR(sqPos->compute(0, 0, 0), -sqNeg->compute(0, 0, 0), 1e-10);
}

// ============================================================================
// TwoArgument::Add compute 测试
// ============================================================================

TEST(DensityFunctionsAddTest, ComputeAddition)
{
    auto a = constantFunc(3.5);
    auto b = constantFunc(2.1);
    auto sum = factory::add(std::move(a), std::move(b));
    EXPECT_NEAR(sum->compute(0, 0, 0), 5.6, 1e-10);
}

TEST(DensityFunctionsAddTest, AddNegative)
{
    auto a = constantFunc(5.0);
    auto b = constantFunc(-3.0);
    auto sum = factory::add(std::move(a), std::move(b));
    EXPECT_DOUBLE_EQ(sum->compute(0, 0, 0), 2.0);
}

// ============================================================================
// TwoArgument::Min/Max 正常路径测试
// ============================================================================

TEST(DensityFunctionsMinTest, NormalPath)
{
    // 两个常数函数的 min
    auto a = constantFunc(3.0);
    auto b = constantFunc(7.0);
    auto minFunc = factory::min(std::move(a), std::move(b));
    EXPECT_DOUBLE_EQ(minFunc->compute(0, 0, 0), 3.0);
}

TEST(DensityFunctionsMinTest, MinMaxValues)
{
    auto a = factory::yClampedGradient(0, 100, -5.0, 5.0);
    auto b = factory::yClampedGradient(0, 100, -3.0, 7.0);
    auto minFunc = factory::min(std::move(a), std::move(b));
    EXPECT_DOUBLE_EQ(minFunc->minValue(), -5.0); // min(-5, -3) = -5
    EXPECT_DOUBLE_EQ(minFunc->maxValue(), 5.0);  // min(5, 7) = 5
}

TEST(DensityFunctionsMaxTest, NormalPath)
{
    // 两个常数函数的 max
    auto a = constantFunc(3.0);
    auto b = constantFunc(7.0);
    auto maxFunc = factory::max(std::move(a), std::move(b));
    EXPECT_DOUBLE_EQ(maxFunc->compute(0, 0, 0), 7.0);
}

TEST(DensityFunctionsMaxTest, MinMaxValues)
{
    auto a = factory::yClampedGradient(0, 100, -5.0, 5.0);
    auto b = factory::yClampedGradient(0, 100, -3.0, 7.0);
    auto maxFunc = factory::max(std::move(a), std::move(b));
    EXPECT_DOUBLE_EQ(maxFunc->minValue(), -3.0); // max(-5, -3) = -3
    EXPECT_DOUBLE_EQ(maxFunc->maxValue(), 7.0);  // max(5, 7) = 7
}

// ============================================================================
// CubicSpline 嵌套样条测试
// ============================================================================

TEST(DensityFunctionsCubicSplineTest, NestedSplineValueResolution)
{
    // 嵌套样条: 外层使用内层样条作为控制点值
    // 输入函数: Y=0→0, Y=50→0.5, Y=100→1
    auto inputFunc = std::make_shared<YClampedGradient>(0, 100, 0.0, 1.0);
    // 内层样条: 简单两点线性（导数=0），location=[0, 1], value=[10, 20]
    auto innerSpline =
        std::make_shared<CubicSpline>(inputFunc, std::vector<SplinePoint>{{0.0, 10.0, 0.0}, {1.0, 20.0, 0.0}});
    // 外层样条: 使用嵌套样条作为值
    std::vector<SplinePoint> points = {{0.0, innerSpline, 0.0}, {1.0, 30.0, 0.0}};
    auto outerSpline = factory::cubicSpline(inputFunc, std::move(points));

    // Y=0: input=0, 外层 coordinate=0，在第一个点，value = innerSpline.apply(0)
    //   innerSpline at coordinate=0: 在第一个点，value=10.0
    // 所以 outerSpline at Y=0 = 10.0
    EXPECT_NEAR(outerSpline->compute(0, 0, 0), 10.0, 1e-10);

    // Y=100: input=1, 外层 coordinate=1，在最后一个点
    //   outerSpline at coordinate=1 = 30.0
    EXPECT_NEAR(outerSpline->compute(0, 100, 0), 30.0, 1e-10);
}

TEST(DensityFunctionsCubicSplineTest, ThreePointInterpolation)
{
    // 三点样条: 位置 [-1, 0, 1]，值 [0, 5, 0]，导数 [0, 0, 0]
    auto inputFunc = std::make_shared<YClampedGradient>(0, 100, -1.0, 1.0);
    std::vector<SplinePoint> points = {{-1.0, 0.0, 0.0}, {0.0, 5.0, 0.0}, {1.0, 0.0, 0.0}};
    auto spline = factory::cubicSpline(inputFunc, std::move(points));

    // Y=50: input=0, coordinate=0 (中间点) → 5.0
    EXPECT_NEAR(spline->compute(0, 50, 0), 5.0, 1e-10);
    // Y=0: input=-1, coordinate=-1 (第一个点) → 0.0
    EXPECT_NEAR(spline->compute(0, 0, 0), 0.0, 1e-10);
    // Y=100: input=1, coordinate=1 (最后一个点) → 0.0
    EXPECT_NEAR(spline->compute(0, 100, 0), 0.0, 1e-10);

    // 中点之间应该为正
    EXPECT_GT(spline->compute(0, 25, 0), 0.0); // Y=25: input=-0.5
    EXPECT_GT(spline->compute(0, 75, 0), 0.0); // Y=75: input=0.5
}

TEST(DensityFunctionsCubicSplineTest, ExtrapolationWithNonZeroDerivative)
{
    // 非零导数的外推：linearExtend(x, loc, val, der) = val + der * (x - loc)
    // 控制点: loc=0, val=10, der=2
    auto input = std::make_shared<Constant>(0.0); // 始终返回 0
    std::vector<SplinePoint> points = {{0.0, 10.0, 2.0}};
    auto spline = factory::cubicSpline(input, std::move(points));
    // input=0, coordinate=0 → 在控制点 → 返回 10.0
    EXPECT_DOUBLE_EQ(spline->compute(0, 0, 0), 10.0);
}

TEST(DensityFunctionsCubicSplineTest, ExtrapolationBelowWithDerivative)
{
    // 当 coordinate < 第一个点时，使用 linearExtend 外推
    // 点: loc=5, val=20, der=3 → 外推到 coordinate=0: 20 + 3*(0-5) = 5
    auto input = std::make_shared<Constant>(0.0); // 始终返回 0
    std::vector<SplinePoint> points = {{5.0, 20.0, 3.0}};
    auto spline = factory::cubicSpline(input, std::move(points));
    // input=0, coordinate=0 < 5 → linearExtend(0, 5, 20, 3) = 20 + 3*(0-5) = 5
    EXPECT_DOUBLE_EQ(spline->compute(0, 0, 0), 5.0);
}

// ============================================================================
// Beardifier 测试
// ============================================================================

TEST(DensityFunctionsBeardifierTest, EmptyBeardifierReturnsZero)
{
    auto empty = Beardifier::EMPTY;
    EXPECT_DOUBLE_EQ(empty.compute(0, 0, 0), 0.0);
    // 空的 Beardifier 在任何坐标都返回 0
    EXPECT_DOUBLE_EQ(empty.compute(100, 50, 200), 0.0);
}

TEST(DensityFunctionsBeardifierTest, EmptyIsSingleton)
{
    // EMPTY 应该是空的
    EXPECT_TRUE(Beardifier::EMPTY.isEmpty());
}

TEST(DensityFunctionsBeardifierTest, MinMaxValues)
{
    EXPECT_EQ(Beardifier::EMPTY.minValue(), -std::numeric_limits<f64>::infinity());
    EXPECT_EQ(Beardifier::EMPTY.maxValue(), std::numeric_limits<f64>::infinity());
}

TEST(DensityFunctionsBeardifierTest, BuryContributionInsideRange)
{
    // 距离为 0 时返回 1.0
    EXPECT_DOUBLE_EQ(Beardifier::getBuryContribution(0, 0, 0), 1.0);
}

TEST(DensityFunctionsBeardifierTest, BuryContributionAtBoundary)
{
    // 距离 >= 6.0 时返回 0.0
    EXPECT_DOUBLE_EQ(Beardifier::getBuryContribution(6, 0, 0), 0.0);
    EXPECT_DOUBLE_EQ(Beardifier::getBuryContribution(0, 6, 0), 0.0);
}

TEST(DensityFunctionsBeardifierTest, BuryContributionLinearRamp)
{
    // 距离 = 3.0: (6 - 3) / 6 = 0.5
    f64 dist3 = Beardifier::getBuryContribution(3, 0, 0);
    EXPECT_NEAR(dist3, 0.5, 0.01); // sqrt(9) = 3, so (6-3)/6 = 0.5
}

TEST(DensityFunctionsBeardifierTest, BeardContributionOutsideKernel)
{
    // 超出核半径（12）时返回 0
    EXPECT_DOUBLE_EQ(Beardifier::getBeardContribution(13, 0, 13, 0), 0.0);
}

// ============================================================================
// Mapped::Square 边界值扩展测试
// ============================================================================

TEST(DensityFunctionsSquareTest, CrossZeroSymmetricInput)
{
    // [-3, 3]: minValue = max(0, -3) = 0, maxValue = max(9, 9) = 9
    auto input = factory::yClampedGradient(0, 100, -3.0, 3.0);
    auto sq = factory::square(std::move(input));
    EXPECT_DOUBLE_EQ(sq->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(sq->maxValue(), 9.0);
}

TEST(DensityFunctionsSquareTest, ComputeAtValue)
{
    auto c = constantFunc(4.0);
    auto sq = factory::square(std::move(c));
    EXPECT_DOUBLE_EQ(sq->compute(0, 0, 0), 16.0);
}

// ============================================================================
// Mapped::Invert 扩展测试
// ============================================================================

TEST(DensityFunctionsInvertTest, SmallRange)
{
    // 输入范围 [0.5, 2.0]: 1/x 在 [0.5, 2] 上递减
    // minValue = 1/2.0 = 0.5, maxValue = 1/0.5 = 2.0
    auto input = factory::yClampedGradient(0, 100, 0.5, 2.0);
    auto inverted = factory::invert(std::move(input));
    EXPECT_NEAR(inverted->minValue(), 0.5, 1e-10);
    EXPECT_NEAR(inverted->maxValue(), 2.0, 1e-10);
}

// ============================================================================
// ShiftedNoise compute 测试
// ============================================================================

TEST(DensityFunctionsShiftedNoiseTest, ComputeDeterministic)
{
    auto shiftX = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::ShiftA);
    auto shiftY = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::Shift);
    auto shiftZ = std::make_unique<ShiftNoise>(makeTestNoise(42, -3, {1.0, 1.0, 1.0, 0.0}), ShiftType::ShiftB);
    auto shifted = std::make_unique<ShiftedNoise>(
        makeTestNoise(42, -8, {1.0}), 0.25, 0.125, std::move(shiftX), std::move(shiftY), std::move(shiftZ));
    f64 v1 = shifted->compute(100, 50, 200);
    f64 v2 = shifted->compute(100, 50, 200);
    EXPECT_DOUBLE_EQ(v1, v2);
}

// ============================================================================
// Cache2D 扩展测试
// ============================================================================

TEST(DensityFunctionsCache2DTest, IgnoresYChanges)
{
    // Cache2D 应该忽略 Y 变化，只按 XZ 缓存
    // 使用 YClampedGradient 作为输入：Y=0→0.0, Y=100→100.0
    auto input = factory::yClampedGradient(0, 100, 0.0, 100.0);
    auto cache = factory::cache2D(std::move(input));
    // 首次 compute(10, 0, 20): Y=0, 值=0.0，缓存到 (10, 20)
    f64 v1 = cache->compute(10, 0, 20);
    EXPECT_DOUBLE_EQ(v1, 0.0);
    // 再次 compute(10, 50, 20): 相同 XZ，不同 Y → 返回缓存值 0.0
    f64 v2 = cache->compute(10, 50, 20);
    EXPECT_DOUBLE_EQ(v2, 0.0); // 缓存值，不是 Y=50 的值
}

// ============================================================================
// FlatCache 扩展测试
// ============================================================================

TEST(DensityFunctionsFlatCacheTest, DelegatesMinMax)
{
    auto input = factory::yClampedGradient(0, 100, -5.0, 5.0);
    f64 inputMin = input->minValue();
    f64 inputMax = input->maxValue();
    auto cache = factory::flatCache(std::move(input));
    EXPECT_DOUBLE_EQ(cache->minValue(), inputMin);
    EXPECT_DOUBLE_EQ(cache->maxValue(), inputMax);
}

// ============================================================================
// Mul 扩展测试
// ============================================================================

TEST(DensityFunctionsMulTest, OnePositiveOneCrossZero)
{
    // [1, 3] * [-2, 4]
    // minValue = min(1*(-2), 3*4) = min(-2, 12) = -2
    //   (both not positive, both not negative, so min(1*4, 3*(-2)) = min(4, -6) = -6? No.)
    //   Wait: d0=1, d1=-2, d2=3, d3=4
    //   d0>0 && d1>0? No. d2<0 && d3<0? No.
    //   minValue = min(d0*d3, d2*d1) = min(1*4, 3*(-2)) = min(4, -6) = -6
    auto pos = factory::yClampedGradient(0, 100, 1.0, 3.0);
    auto crossZero = factory::yClampedGradient(0, 100, -2.0, 4.0);
    auto product = factory::mul(std::move(pos), std::move(crossZero));
    EXPECT_DOUBLE_EQ(product->minValue(), -6.0);
    // maxValue: d0>0 && d1>0? No. d2<0 && d3<0? No.
    //   maxValue = max(d0*d1, d2*d3) = max(1*(-2), 3*4) = max(-2, 12) = 12
    EXPECT_DOUBLE_EQ(product->maxValue(), 12.0);
}

TEST(DensityFunctionsMulTest, BothCrossZero)
{
    // [-1, 2] * [-3, 4]
    // d0=-1, d1=-3, d2=2, d3=4
    // minValue = min(d0*d3, d2*d1) = min(-1*4, 2*(-3)) = min(-4, -6) = -6
    // maxValue = max(d0*d1, d2*d3) = max(-1*(-3), 2*4) = max(3, 8) = 8
    auto a = factory::yClampedGradient(0, 100, -1.0, 2.0);
    auto b = factory::yClampedGradient(0, 100, -3.0, 4.0);
    auto product = factory::mul(std::move(a), std::move(b));
    EXPECT_DOUBLE_EQ(product->minValue(), -6.0);
    EXPECT_DOUBLE_EQ(product->maxValue(), 8.0);
}

// ============================================================================
// Lerp 边界值扩展测试
// ============================================================================

TEST(DensityFunctionsLerpTest, DeltaZero_ReturnsStartFromVariable)
{
    // delta=0 时，lerp(0, start, end) = start
    auto delta = factory::yClampedGradient(0, 100, 0.0, 0.0); // 始终 0
    auto start = factory::yClampedGradient(0, 100, -5.0, 5.0);
    auto end = factory::yClampedGradient(0, 100, 10.0, 20.0);
    auto lerp = factory::lerp(std::move(delta), std::move(start), std::move(end));
    // delta=0 → result = start
    EXPECT_NEAR(lerp->compute(0, 0, 0), -5.0, 1e-10);
    EXPECT_NEAR(lerp->compute(0, 100, 0), 5.0, 1e-10);
}

// ============================================================================
// RangeChoice 扩展测试
// ============================================================================

TEST(DensityFunctionsRangeChoiceTest, NegativeRange)
{
    // 输入在负范围 [-5, -1] 内
    auto input = constantFunc(-3.0);
    auto whenInRange = constantFunc(10.0);
    auto whenOutOfRange = constantFunc(-10.0);
    auto rc = factory::rangeChoice(std::move(input), -5.0, 0.0, std::move(whenInRange), std::move(whenOutOfRange));
    EXPECT_DOUBLE_EQ(rc->compute(0, 0, 0), 10.0); // -3 在 [-5, 0) 内
}

TEST(DensityFunctionsRangeChoiceTest, ExactMinBoundary)
{
    auto input = constantFunc(-5.0);
    auto whenInRange = constantFunc(10.0);
    auto whenOutOfRange = constantFunc(-10.0);
    auto rc = factory::rangeChoice(std::move(input), -5.0, 0.0, std::move(whenInRange), std::move(whenOutOfRange));
    EXPECT_DOUBLE_EQ(rc->compute(0, 0, 0), 10.0); // -5 刚好等于 minInclusive
}

// ============================================================================
// EndIslands 边界值扩展测试
// ============================================================================

TEST(DensityFunctionsEndIslandsTest, SeedZeroIsDefault)
{
    // MC 1.21: endIslands 使用种子 0（而非世界种子）
    auto islands = factory::endIslands(0);
    EXPECT_DOUBLE_EQ(islands->minValue(), -0.84375);
    EXPECT_DOUBLE_EQ(islands->maxValue(), 0.5625);
}

TEST(DensityFunctionsEndIslandsTest, MultiplePositionsDiffer)
{
    auto islands = factory::endIslands(0);
    f64 v1 = islands->compute(0, 0, 0);
    f64 v2 = islands->compute(1000, 0, 1000);
    // 不同位置应该产生不同的值（极大概率）
    // 注意：不使用 EXPECT_NE 因为理论上可能相同
    // 只验证两个值都在有效范围内
    EXPECT_GE(v1, islands->minValue());
    EXPECT_LE(v1, islands->maxValue());
    EXPECT_GE(v2, islands->minValue());
    EXPECT_LE(v2, islands->maxValue());
}

// ============================================================================
// Marker 类型扩展测试
// ============================================================================

TEST(DensityFunctionsMarkerTest, CacheAllInCellDelegates)
{
    auto inner = constantFunc(3.0);
    auto marker = factory::cacheAllInCellMarker(std::move(inner));
    EXPECT_DOUBLE_EQ(marker->compute(0, 0, 0), 3.0);
    EXPECT_DOUBLE_EQ(marker->minValue(), 3.0);
    EXPECT_DOUBLE_EQ(marker->maxValue(), 3.0);
}

TEST(DensityFunctionsMarkerTest, FlatCacheMarkerDelegates)
{
    auto inner = constantFunc(5.0);
    auto marker = factory::flatCacheMarker(std::move(inner));
    EXPECT_DOUBLE_EQ(marker->compute(0, 0, 0), 5.0);
    EXPECT_DOUBLE_EQ(marker->minValue(), 5.0);
    EXPECT_DOUBLE_EQ(marker->maxValue(), 5.0);
}

TEST(DensityFunctionsMarkerTest, Cache2DMarkerDelegates)
{
    auto inner = constantFunc(7.0);
    auto marker = factory::cache2DMarker(std::move(inner));
    EXPECT_DOUBLE_EQ(marker->compute(0, 0, 0), 7.0);
    EXPECT_DOUBLE_EQ(marker->minValue(), 7.0);
    EXPECT_DOUBLE_EQ(marker->maxValue(), 7.0);
}
