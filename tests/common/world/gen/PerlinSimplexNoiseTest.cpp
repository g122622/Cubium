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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/gen/noise/PerlinSimplexNoise.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::gen::noise;

// ============================================================================
// PerlinSimplexNoise 确定性测试
// ============================================================================

TEST(PerlinSimplexNoiseTest, SameSeedProducesIdenticalResults)
{
    // MC: PerlinSimplexNoise 使用 IRandom& 构造
    // 同样的随机源应产生相同的噪声
    math::Random rng1(12345);
    PerlinSimplexNoise noise1(rng1, {-2, -1, 0});

    math::Random rng2(12345);
    PerlinSimplexNoise noise2(rng2, {-2, -1, 0});

    for (int i = 0; i < 100; ++i) {
        const f64 x = static_cast<f64>(i) * 17.3;
        const f64 y = static_cast<f64>(i) * 31.7;
        EXPECT_NEAR(noise1.getValue(x, y, false), noise2.getValue(x, y, false), 1e-12)
            << "PerlinSimplexNoise determinism mismatch at sample " << i;
    }
}

TEST(PerlinSimplexNoiseTest, SameSeedWithOffsetProducesIdenticalResults)
{
    math::Random rng1(12345);
    PerlinSimplexNoise noise1(rng1, {-2, -1, 0});

    math::Random rng2(12345);
    PerlinSimplexNoise noise2(rng2, {-2, -1, 0});

    for (int i = 0; i < 50; ++i) {
        const f64 x = static_cast<f64>(i) * 11.1;
        const f64 y = static_cast<f64>(i) * 22.2;
        EXPECT_NEAR(noise1.getValue(x, y, true), noise2.getValue(x, y, true), 1e-12)
            << "PerlinSimplexNoise with offset determinism mismatch at sample " << i;
    }
}

TEST(PerlinSimplexNoiseTest, DifferentSeedsProduceDifferentResults)
{
    math::Random rng1(12345);
    PerlinSimplexNoise noise1(rng1, {-2, -1, 0});

    math::Random rng2(54321);
    PerlinSimplexNoise noise2(rng2, {-2, -1, 0});

    bool anyDifferent = false;
    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i) * 11.1;
        const f64 y = static_cast<f64>(i) * 22.2;
        if (std::abs(noise1.getValue(x, y, false) - noise2.getValue(x, y, false)) > 1e-10) {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent) << "Different seeds should produce different PerlinSimplexNoise values";
}

// ============================================================================
// PerlinSimplexNoise 倍频配置测试
// ============================================================================

TEST(PerlinSimplexNoiseTest, SingleOctaveZero)
{
    // 只使用 octave 0 (最高频)
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {0});

    const f64 value = noise.getValue(100.0, 200.0, false);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(PerlinSimplexNoiseTest, ThreeOctavesWithNegatives)
{
    // {-2, -1, 0}: 使用负倍频 (低频) 和 0 倍频 (高频)
    // MC: OverworldBiomeBuilder 气候噪声使用类似配置
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-2, -1, 0});

    const f64 value = noise.getValue(100.0, 200.0, false);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(PerlinSimplexNoiseTest, SingleNegativeOctave)
{
    // 只使用 -3 倍频 (低频)
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-3});

    const f64 value = noise.getValue(100.0, 200.0, false);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(PerlinSimplexNoiseTest, ManyOctaves)
{
    // 多倍频配置
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-6, -5, -4, -3, -2, -1, 0});

    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i) * 11.1;
        const f64 y = static_cast<f64>(i) * 22.2;
        const f64 value = noise.getValue(x, y, false);
        EXPECT_TRUE(std::isfinite(value)) << "Non-finite value at sample " << i;
    }
}

// ============================================================================
// PerlinSimplexNoise getValue 行为测试
// ============================================================================

TEST(PerlinSimplexNoiseTest, UseOffsetChangesOutput)
{
    // useOffset=true 时添加每个 SimplexNoise 的偏移，结果应不同
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-2, -1, 0});

    bool anyDifferent = false;
    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i) * 13.1;
        const f64 y = static_cast<f64>(i) * 27.3;
        const f64 withOffset = noise.getValue(x, y, true);
        const f64 withoutOffset = noise.getValue(x, y, false);
        if (std::abs(withOffset - withoutOffset) > 1e-10) {
            anyDifferent = true;
            break;
        }
    }
    // 并非所有位置都不同，但大部分应不同
    EXPECT_TRUE(anyDifferent) << "useOffset should change output for most positions";
}

TEST(PerlinSimplexNoiseTest, GetValueAtOrigin)
{
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-2, -1, 0});

    const f64 value = noise.getValue(0.0, 0.0, false);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(PerlinSimplexNoiseTest, GetValueRangeReasonable)
{
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-2, -1, 0});

    f64 minValue = std::numeric_limits<f64>::max();
    f64 maxValue = std::numeric_limits<f64>::lowest();

    for (int i = 0; i < 1000; ++i) {
        const f64 x = static_cast<f64>(i) * 0.5;
        const f64 y = static_cast<f64>(i) * 0.7;
        const f64 value = noise.getValue(x, y, false);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    // 值应在合理范围内
    EXPECT_LT(maxValue - minValue, 10.0) << "PerlinSimplexNoise range too wide";
    EXPECT_GT(maxValue - minValue, 0.0) << "PerlinSimplexNoise range too narrow";
}

// ============================================================================
// PerlinSimplexNoise firstNoise 测试
// ============================================================================

TEST(PerlinSimplexNoiseTest, FirstNoiseNotNull)
{
    // PerlinSimplexNoise 应始终有一个非空的 firstNoise
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-2, -1, 0});

    const SimplexNoise* first = noise.firstNoise();
    ASSERT_NE(first, nullptr);
}

TEST(PerlinSimplexNoiseTest, FirstNoiseHasValidOffsets)
{
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-2, -1, 0});

    const SimplexNoise* first = noise.firstNoise();
    ASSERT_NE(first, nullptr);

    // 偏移应在 [0, 256) 范围内
    EXPECT_GE(first->xOffset(), 0.0);
    EXPECT_LT(first->xOffset(), 256.0);
    EXPECT_GE(first->yOffset(), 0.0);
    EXPECT_LT(first->yOffset(), 256.0);
}

// ============================================================================
// PerlinSimplexNoise JavaLegacyRandom 种子派生测试
// ============================================================================

TEST(PerlinSimplexNoiseTest, NegativeOctaveSeedDerivation)
{
    // 当存在负倍频时，PerlinSimplexNoise 使用 firstNoise 的 3D 评估值派生种子
    // 这个值与 JavaLegacyRandom 一起用于创建负倍频层的 SimplexNoise
    // 验证使用负倍频时噪声仍然正确

    math::Random rng1(42);
    PerlinSimplexNoise noise1(rng1, {-3, -2, -1, 0});

    math::Random rng2(42);
    PerlinSimplexNoise noise2(rng2, {-3, -2, -1, 0});

    // 确定性测试
    for (int i = 0; i < 30; ++i) {
        const f64 x = static_cast<f64>(i) * 7.7;
        const f64 y = static_cast<f64>(i) * 13.3;
        EXPECT_NEAR(noise1.getValue(x, y, false), noise2.getValue(x, y, false), 1e-12)
            << "Negative octave seed derivation determinism mismatch at sample " << i;
    }
}

// ============================================================================
// PerlinSimplexNoise 平滑性测试
// ============================================================================

TEST(PerlinSimplexNoiseTest, GetValueIsSmooth)
{
    math::Random rng(42);
    PerlinSimplexNoise noise(rng, {-2, -1, 0});

    const f64 step = 0.01;
    for (int i = 0; i < 50; ++i) {
        const f64 x = static_cast<f64>(i) * 10.0;
        const f64 y = static_cast<f64>(i) * 7.0;
        const f64 v1 = noise.getValue(x, y, false);
        const f64 v2 = noise.getValue(x + step, y, false);
        // 相邻步长变化应很小
        EXPECT_LT(std::abs(v2 - v1), 0.5) << "PerlinSimplexNoise should be smooth at sample " << i;
    }
}

} // namespace
} // namespace mc
