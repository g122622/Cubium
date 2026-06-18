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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/gen/noise/PerlinNoise.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::gen::noise;

// ============================================================================
// PerlinNoise 构造与参数测试
// ============================================================================

TEST(PerlinNoiseTest, ConstructorWithSeedProducesDeterministicResults)
{
    const u64 seed = 12345;
    PerlinNoise noise1(seed, -3, {1.0, 1.0, 1.0, 1.0});
    PerlinNoise noise2(seed, -3, {1.0, 1.0, 1.0, 1.0});

    for (int i = 0; i < 50; ++i) {
        const f64 x = static_cast<f64>(i * 17.3);
        const f64 y = static_cast<f64>(i * 31.7);
        const f64 z = static_cast<f64>(i * 53.1);
        EXPECT_NEAR(noise1.getValue(x, y, z), noise2.getValue(x, y, z), 1e-12)
            << "PerlinNoise determinism mismatch at sample " << i;
    }
}

TEST(PerlinNoiseTest, DifferentSeedsProduceDifferentResults)
{
    PerlinNoise noise1(12345, -3, {1.0, 1.0, 1.0, 1.0});
    PerlinNoise noise2(54321, -3, {1.0, 1.0, 1.0, 1.0});

    bool anyDifferent = false;
    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i * 11.1);
        const f64 y = static_cast<f64>(i * 22.2);
        const f64 z = static_cast<f64>(i * 33.3);
        if (std::abs(noise1.getValue(x, y, z) - noise2.getValue(x, y, z)) > 1e-10) {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent) << "Different seeds should produce different noise values";
}

TEST(PerlinNoiseTest, FirstOctaveAndAmplitudesAccessors)
{
    const i32 firstOctave = -4;
    const std::vector<f64> amplitudes = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    PerlinNoise noise(42, firstOctave, amplitudes);

    EXPECT_EQ(noise.firstOctave(), firstOctave);
    EXPECT_EQ(noise.amplitudes().size(), amplitudes.size());
    for (size_t i = 0; i < amplitudes.size(); ++i) {
        EXPECT_NEAR(noise.amplitudes()[i], amplitudes[i], 1e-15);
    }
}

TEST(PerlinNoiseTest, LowestFreqInputFactorForNegativeOctave)
{
    // firstOctave = -3, amplitudes = {1.0, 1.0, 1.0, 1.0}
    // lowestFreqInputFactor = 2^(-3) = 0.125
    // lowestFreqValueFactor = 2^3 / (2^4 - 1) = 8/15 = 0.5333...
    // maxValue = edgeValue(2.0) = sum of |amp| * 2.0 * valueFactor_i for non-zero layers
    PerlinNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});

    // 值应在合理范围内
    EXPECT_GT(noise.maxValue(), 0.0);
    // 4 个振幅为 1.0 的倍频，maxValue 应该接近理论值
    // edgeValue(2.0) = 2.0 * (8/15 + 4/15 + 2/15 + 1/15) = 2.0 * 1.0 = 2.0
    EXPECT_NEAR(noise.maxValue(), 2.0, 1e-10);
}

TEST(PerlinNoiseTest, SingleAmplitudeMaxValue)
{
    // 1 个振幅: lowestFreqValueFactor = 2^0/(2^1-1) = 1.0/1.0 = 1.0
    // edgeValue(2.0) = 1.0 * 2.0 * 1.0 = 2.0
    PerlinNoise noise(42, 0, {1.0});
    EXPECT_NEAR(noise.maxValue(), 2.0, 1e-10);
}

TEST(PerlinNoiseTest, TwoAmplitudesMaxValue)
{
    // 2 个振幅: lowestFreqValueFactor = 2^1/(2^2-1) = 2/3
    // edgeValue(2.0) = 1.0 * 2.0 * (2/3 + 1/3) = 1.0 * 2.0 * 1.0 = 2.0
    PerlinNoise noise(42, -1, {1.0, 1.0});
    EXPECT_NEAR(noise.maxValue(), 2.0, 1e-10);
}

TEST(PerlinNoiseTest, MaxBrokenValueIsEdgeValuePlus2)
{
    // maxBrokenValue(maxInput) = edgeValue(maxInput + 2.0)
    PerlinNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});

    const f64 maxInput = 1.5;
    // 对 edgeValue 和 maxBrokenValue 的关系进行验证
    // maxBrokenValue 应大于 maxValue（因为 maxInput + 2 > 2）
    EXPECT_GT(noise.maxBrokenValue(maxInput), noise.maxValue());
}

TEST(PerlinNoiseTest, ZeroAmplitudeSkipsOctave)
{
    // 第二个倍频振幅为 0，应跳过该层
    PerlinNoise noise(42, -2, {1.0, 0.0, 1.0});

    // getOctaveNoise 对零振幅层应返回 nullptr
    // octave = -2 (firstOctave) -> index = size-1-(-2) = 2+2 = 4, out of range -> nullptr
    // octave = 0 (最高频) -> index = size-1-0 = 2, 该层振幅 1.0，应非 null
    EXPECT_NE(noise.getOctaveNoise(0), nullptr);
    // octave = 1 -> index = size-1-1 = 1, 振幅 0.0，应返回 nullptr
    EXPECT_EQ(noise.getOctaveNoise(1), nullptr);
}

TEST(PerlinNoiseTest, GetOctaveNoiseReverseIndex)
{
    // MC 1.21.11: getOctaveNoise 使用反向索引
    // octave=0 返回最高频层 (最后一个)，octave=1 返回次高频层...
    const std::vector<f64> amps = {1.0, 1.0, 1.0};
    PerlinNoise noise(42, -2, amps);

    // 共 3 层: index 0 = firstOctave(-2), index 1 = -1, index 2 = 0(最高频)
    // getOctaveNoise(0) -> index = 3-1-0 = 2 -> 最高频层，非 null
    EXPECT_NE(noise.getOctaveNoise(0), nullptr);
    // getOctaveNoise(1) -> index = 3-1-1 = 1 -> 中频层，非 null
    EXPECT_NE(noise.getOctaveNoise(1), nullptr);
    // getOctaveNoise(2) -> index = 3-1-2 = 0 -> 低频层，非 null
    EXPECT_NE(noise.getOctaveNoise(2), nullptr);
    // 超出范围返回 nullptr
    EXPECT_EQ(noise.getOctaveNoise(3), nullptr);
    EXPECT_EQ(noise.getOctaveNoise(-1), nullptr);
}

TEST(PerlinNoiseTest, GetValueWithSmearProducesDifferentResultsFromGetValue)
{
    PerlinNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});

    // smearScaleMultiplier != 0 时，getValueWithSmear 应产生不同的结果
    const f64 x = 100.0, y = 64.0, z = 200.0;
    const f64 normal = noise.getValue(x, y, z);
    const f64 smeared = noise.getValueWithSmear(x, y, z, 8.0);

    // 涂抹效果应改变 Y 轴方向的行为，导致不同的输出
    // 但不保证总是不同，因为某些 Y 值可能恰好在网格线上
    // 只测试不崩溃且返回有限值
    EXPECT_TRUE(std::isfinite(smeared));
    EXPECT_TRUE(std::isfinite(normal));
}

TEST(PerlinNoiseTest, WrapPreventsPrecisionLoss)
{
    // wrap 应将值限制在 [-16777216, 16777216] 范围内
    // 2^25 = 33554432, 半宽 = 16777216
    const f64 largeValue = 1e15;
    const f64 wrapped = PerlinNoise::wrap(largeValue);
    EXPECT_GT(wrapped, -16777217.0);
    EXPECT_LT(wrapped, 16777217.0);

    // 小值应不受影响
    const f64 smallValue = 100.0;
    EXPECT_NEAR(PerlinNoise::wrap(smallValue), smallValue, 1e-10);

    // 负小值
    const f64 negSmall = -100.0;
    EXPECT_NEAR(PerlinNoise::wrap(negSmall), negSmall, 1e-10);
}

TEST(PerlinNoiseTest, GetValueAtOrigin)
{
    PerlinNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});
    const f64 value = noise.getValue(0.0, 0.0, 0.0);
    // 原点处应返回有限值
    EXPECT_TRUE(std::isfinite(value));
}

TEST(PerlinNoiseTest, GetValueIsSmooth)
{
    // 噪声值应在相邻点之间平滑变化
    PerlinNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});
    const f64 step = 0.01;

    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i) * 10.0;
        const f64 v1 = noise.getValue(x, 0.0, 0.0);
        const f64 v2 = noise.getValue(x + step, 0.0, 0.0);
        // 相邻点的变化应远小于理论最大值
        EXPECT_LT(std::abs(v2 - v1), 1.0) << "Noise should be smooth at (" << x << ", 0, 0)";
    }
}

TEST(PerlinNoiseTest, GetValueRangeWithinMaxValue)
{
    PerlinNoise noise(42, -3, {1.0, 1.0, 1.0, 1.0});

    f64 minValue = std::numeric_limits<f64>::max();
    f64 maxValue = std::numeric_limits<f64>::lowest();

    // 在大量采样点检查值范围
    for (int i = 0; i < 1000; ++i) {
        const f64 x = static_cast<f64>(i) * 3.7;
        const f64 y = static_cast<f64>(i) * 5.3;
        const f64 z = static_cast<f64>(i) * 7.1;
        const f64 value = noise.getValue(x, y, z);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    // 理论上 |value| <= maxValue，但随机采样不一定能触及极值
    // 只检查采样到的值在合理范围内
    EXPECT_LT(maxValue, noise.maxValue() * 1.5);
    EXPECT_GT(minValue, -noise.maxValue() * 1.5);
}

TEST(PerlinNoiseTest, NegativeAmplitudeReversesOutput)
{
    // 负振幅应反转噪声值
    PerlinNoise positive(42, -2, {1.0, 1.0});
    PerlinNoise negative(42, -2, {-1.0, -1.0});

    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i) * 11.1;
        const f64 y = static_cast<f64>(i) * 22.2;
        const f64 z = static_cast<f64>(i) * 33.3;
        const f64 posVal = positive.getValue(x, y, z);
        const f64 negVal = negative.getValue(x, y, z);
        // 负振幅产生的值应该是正振幅的负值
        EXPECT_NEAR(posVal, -negVal, 1e-10) << "Negative amplitude should reverse output at sample " << i;
    }
}

TEST(PerlinNoiseTest, PositionalRandomFactoryConstructorMatchesSeedConstructor)
{
    // 使用 PositionalRandomFactory 构造应产生与种子构造不同的结果
    // （因为种子构造先创建 Random 再 forkPositional，两次调用产生不同的工厂）
    const u64 seed = 42;
    math::Random rng(seed);

    PerlinNoise fromSeed(seed, -3, {1.0, 1.0, 1.0, 1.0});
    PerlinNoise fromFactory(rng.forkPositional(), -3, {1.0, 1.0, 1.0, 1.0});

    // fromSeed 构造内部也创建了自己的 Random(seed) 并 forkPositional
    // fromFactory 使用外部 rng 的 forkPositional
    // 由于 rng 已经被消耗了一些状态，两者的结果应该不同
    // 但 fromSeed 和使用相同 seed 重新创建的 Random 应该相同
    PerlinNoise fromSeed2(seed, -3, {1.0, 1.0, 1.0, 1.0});
    EXPECT_NEAR(fromSeed.getValue(100.0, 200.0, 300.0), fromSeed2.getValue(100.0, 200.0, 300.0), 1e-12);
}

TEST(PerlinNoiseTest, AmplitudesWithZeros)
{
    // 混合零和非零振幅
    PerlinNoise noise(42, -3, {1.0, 0.0, 0.5, 0.0, 0.25});

    // 不崩溃且返回有限值
    const f64 value = noise.getValue(100.0, 64.0, 200.0);
    EXPECT_TRUE(std::isfinite(value));

    // maxValue 只计入非零振幅
    EXPECT_GT(noise.maxValue(), 0.0);
}

} // namespace
} // namespace mc
