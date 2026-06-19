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
#include "common/world/gen/density/CaveDensityFunctions.hpp"
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
// CaveDensityFunctions 噪声参数测试
// ============================================================================

TEST(CaveDensityFunctionsTest, NoiseParametersMatchMC1211)
{
    // 验证关键噪声参数与 MC 1.21.11 对齐

    // SPAGHETTI_2D_ELEVATION: firstOctave=-8, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_2D_ELEVATION_OCTAVE, -8);
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_2D_ELEVATION_AMPS[0], 1.0);

    // SPAGHETTI_2D_THICKNESS: firstOctave=-11, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_2D_THICKNESS_OCTAVE, -11);
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_2D_THICKNESS_AMPS[0], 1.0);

    // SPAGHETTI_3D_RARITY: firstOctave=-11, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_3D_RARITY_OCTAVE, -11);

    // SPAGHETTI_3D_THICKNESS: firstOctave=-8, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_3D_THICKNESS_OCTAVE, -8);

    // SPAGHETTI_3D_1/2: firstOctave=-7, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_3D_1_OCTAVE, -7);
    EXPECT_EQ(CaveDensityFunctions::SPAGHETTI_3D_2_OCTAVE, -7);

    // CAVE_ENTRANCE: firstOctave=-7, amplitudes={0.4, 0.5, 1.0}
    EXPECT_EQ(CaveDensityFunctions::CAVE_ENTRANCE_OCTAVE, -7);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_ENTRANCE_AMPS[0], 0.4, 1e-10);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_ENTRANCE_AMPS[1], 0.5, 1e-10);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_ENTRANCE_AMPS[2], 1.0, 1e-10);

    // CAVE_LAYER: firstOctave=-8, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::CAVE_LAYER_OCTAVE, -8);
    EXPECT_EQ(CaveDensityFunctions::CAVE_LAYER_AMPS[0], 1.0);

    // CAVE_CHEESE: firstOctave=-8, amplitudes={0.5, 1.0, 2.0, 1.0, 2.0, 1.0, 0.0, 2.0, 0.0}
    EXPECT_EQ(CaveDensityFunctions::CAVE_CHEESE_OCTAVE, -8);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_CHEESE_AMPS[0], 0.5, 1e-10);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_CHEESE_AMPS[1], 1.0, 1e-10);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_CHEESE_AMPS[2], 2.0, 1e-10);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_CHEESE_AMPS[6], 0.0, 1e-10);
    EXPECT_NEAR(CaveDensityFunctions::CAVE_CHEESE_AMPS[7], 2.0, 1e-10);

    // PILLAR: firstOctave=-7, amplitudes={1.0, 1.0}
    EXPECT_EQ(CaveDensityFunctions::PILLAR_OCTAVE, -7);
    EXPECT_EQ(CaveDensityFunctions::PILLAR_AMPS[0], 1.0);
    EXPECT_EQ(CaveDensityFunctions::PILLAR_AMPS[1], 1.0);

    // JAGGED: firstOctave=-16, amplitudes={1.0}×17
    EXPECT_EQ(CaveDensityFunctions::JAGGED_OCTAVE, -16);
    EXPECT_EQ(std::size(CaveDensityFunctions::JAGGED_AMPS), 17u);
    for (size_t i = 0; i < 17; ++i) {
        EXPECT_NEAR(CaveDensityFunctions::JAGGED_AMPS[i], 1.0, 1e-10) << "JAGGED_AMPS[" << i << "] should be 1.0";
    }

    // ORE_VEININESS: firstOctave=-8, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::ORE_VEININESS_OCTAVE, -8);
    EXPECT_EQ(CaveDensityFunctions::ORE_VEININESS_AMPS[0], 1.0);

    // ORE_VEIN_A/B: firstOctave=-7, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::ORE_VEIN_A_OCTAVE, -7);
    EXPECT_EQ(CaveDensityFunctions::ORE_VEIN_B_OCTAVE, -7);

    // ORE_GAP: firstOctave=-5, amplitudes={1.0}
    EXPECT_EQ(CaveDensityFunctions::ORE_GAP_OCTAVE, -5);
    EXPECT_EQ(CaveDensityFunctions::ORE_GAP_AMPS[0], 1.0);
}
