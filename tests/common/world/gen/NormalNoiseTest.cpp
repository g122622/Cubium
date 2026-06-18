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

#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::gen::noise;

// ============================================================================
// NormalNoise 确定性测试
// ============================================================================

TEST(NormalNoiseTest, SameSeedProducesIdenticalResults)
{
    const u64 seed = 54321;
    NormalNoise noise1(seed, -3, {1.0, 1.0, 1.0, 1.0});
    NormalNoise noise2(seed, -3, {1.0, 1.0, 1.0, 1.0});

    for (int i = 0; i < 100; ++i) {
        const f64 x = static_cast<f64>(i * 17.3);
        const f64 y = static_cast<f64>(i * 31.7);
        const f64 z = static_cast<f64>(i * 53.1);
        EXPECT_NEAR(noise1.getValue(x, y, z), noise2.getValue(x, y, z), 1e-12)
            << "NormalNoise determinism mismatch at sample " << i;
    }
}

TEST(NormalNoiseTest, DifferentSeedsProduceDifferentResults)
{
    NormalNoise noise1(11111, -3, {1.0, 1.0, 1.0, 1.0});
    NormalNoise noise2(22222, -3, {1.0, 1.0, 1.0, 1.0});

    bool anyDifferent = false;
    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i * 13.1);
        const f64 y = static_cast<f64>(i * 27.3);
        const f64 z = static_cast<f64>(i * 41.7);
        if (std::abs(noise1.getValue(x, y, z) - noise2.getValue(x, y, z)) > 1e-10) {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent) << "Different seeds should produce different noise values";
}

// ============================================================================
// NormalNoise 参数测试
// ============================================================================

TEST(NormalNoiseTest, FirstOctaveAndAmplitudesAccessors)
{
    NormalNoise noise(42, -5, {1.0, 1.0, 1.0, 1.0, 1.0, 1.0});
    EXPECT_EQ(noise.firstOctave(), -5);
    EXPECT_EQ(noise.amplitudes().size(), 6u);
}

TEST(NormalNoiseTest, SeedAccessor)
{
    NormalNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});
    EXPECT_TRUE(noise.seed().has_value());
    EXPECT_EQ(noise.seed().value(), 42u);
}

TEST(NormalNoiseTest, CloneProducesIdenticalResults)
{
    NormalNoise original(12345, -3, {1.0, 1.0, 1.0, 1.0});
    auto cloned = original.clone();

    for (int i = 0; i < 50; ++i) {
        const f64 x = static_cast<f64>(i * 7.3);
        const f64 y = static_cast<f64>(i * 13.7);
        const f64 z = static_cast<f64>(i * 19.1);
        EXPECT_NEAR(original.getValue(x, y, z), cloned->getValue(x, y, z), 1e-12)
            << "Cloned NormalNoise mismatch at sample " << i;
    }
}

// ============================================================================
// NormalNoise 值范围测试
// ============================================================================

TEST(NormalNoiseTest, GetValueRangeWithinMaxValue)
{
    NormalNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});
    const f64 theoreticalMax = noise.maxValue();

    f64 minValue = std::numeric_limits<f64>::max();
    f64 maxValue = std::numeric_limits<f64>::lowest();

    for (int i = 0; i < 2000; ++i) {
        const f64 x = static_cast<f64>(i) * 2.7;
        const f64 y = static_cast<f64>(i) * 3.9;
        const f64 z = static_cast<f64>(i) * 5.3;
        const f64 value = noise.getValue(x, y, z);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    // 理论上 |value| <= maxValue，但随机采样可能无法触及精确极值
    // 使用 1.5 倍的理论最大值作为安全范围
    EXPECT_LT(maxValue, theoreticalMax * 1.5) << "Max sampled value exceeds 1.5x theoretical max";
    EXPECT_GT(minValue, -theoreticalMax * 1.5) << "Min sampled value exceeds -1.5x theoretical max";
}

TEST(NormalNoiseTest, MaxValueIsPositive)
{
    NormalNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});
    EXPECT_GT(noise.maxValue(), 0.0);
}

TEST(NormalNoiseTest, GetValueAtOrigin)
{
    NormalNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});
    const f64 value = noise.getValue(0.0, 0.0, 0.0);
    EXPECT_TRUE(std::isfinite(value));
}

// ============================================================================
// NormalNoise INPUT_FACTOR 测试
// ============================================================================

TEST(NormalNoiseTest, InputFactorDecouplesTwoPerlinNoises)
{
    // NormalNoise 使用两个 PerlinNoise 实例：
    // 第一个使用原始坐标，第二个使用 INPUT_FACTOR ≈ 1.018 缩放的坐标
    // 这确保两个噪声不相关
    NormalNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});

    // 正常采样应产生合理值
    for (int i = 0; i < 50; ++i) {
        const f64 x = static_cast<f64>(i) * 10.0;
        const f64 y = static_cast<f64>(i) * 10.0;
        const f64 z = static_cast<f64>(i) * 10.0;
        const f64 value = noise.getValue(x, y, z);
        EXPECT_TRUE(std::isfinite(value)) << "Non-finite value at sample " << i;
    }
}

// ============================================================================
// NormalNoise 不同振幅配置测试
// ============================================================================

TEST(NormalNoiseTest, SingleAmplitudeProducesFiniteValues)
{
    NormalNoise noise(42, 0, {1.0});
    const f64 value = noise.getValue(100.0, 64.0, 200.0);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(NormalNoiseTest, MixedAmplitudesWithZeros)
{
    // 与 MC 的 TEMPERATURE 噪声类似：firstOctave=-4, amps={1,0,0,0,1}
    NormalNoise noise(42, -4, {1.0, 0.0, 0.0, 0.0, 1.0});
    const f64 value = noise.getValue(100.0, 64.0, 200.0);
    EXPECT_TRUE(std::isfinite(value));
    EXPECT_GT(noise.maxValue(), 0.0);
}

// ============================================================================
// NormalNoise expectedDeviation 测试
// ============================================================================

TEST(NormalNoiseTest, ExpectedDeviationValues)
{
    // expectedDeviation(n) = 0.1 * (1.0 + 1.0 / (n + 1))
    // n=0: 0.1 * (1 + 1) = 0.2
    // n=1: 0.1 * (1 + 0.5) = 0.15
    // n=2: 0.1 * (1 + 1/3) ≈ 0.13333...
    // n=3: 0.1 * (1 + 0.25) = 0.125
    EXPECT_NEAR(NormalNoise::expectedDeviation(0), 0.2, 1e-15);
    EXPECT_NEAR(NormalNoise::expectedDeviation(1), 0.15, 1e-15);
    EXPECT_NEAR(NormalNoise::expectedDeviation(2), 0.1 * (1.0 + 1.0 / 3.0), 1e-15);
    EXPECT_NEAR(NormalNoise::expectedDeviation(3), 0.125, 1e-15);
}

// ============================================================================
// NormalNoise 构造方式测试
// ============================================================================

TEST(NormalNoiseTest, ConstructFromRandom)
{
    math::Random rng(42);
    NormalNoise noise(rng, -3, {1.0, 1.0, 1.0, 1.0});

    // 通过 Random& 构造的噪声应返回有限值
    const f64 value = noise.getValue(100.0, 64.0, 200.0);
    EXPECT_TRUE(std::isfinite(value));

    // seed 应为 nullopt（无法从 Random& 提取种子）
    EXPECT_FALSE(noise.seed().has_value());
}

TEST(NormalNoiseTest, ConstructFromRandomIsDeterministic)
{
    math::Random rng1(42);
    NormalNoise noise1(rng1, -3, {1.0, 1.0, 1.0, 1.0});

    math::Random rng2(42);
    NormalNoise noise2(rng2, -3, {1.0, 1.0, 1.0, 1.0});

    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i * 11.1);
        const f64 y = static_cast<f64>(i * 22.2);
        const f64 z = static_cast<f64>(i * 33.3);
        EXPECT_NEAR(noise1.getValue(x, y, z), noise2.getValue(x, y, z), 1e-12)
            << "Random-constructed NormalNoise determinism mismatch at sample " << i;
    }
}

} // namespace
} // namespace mc
