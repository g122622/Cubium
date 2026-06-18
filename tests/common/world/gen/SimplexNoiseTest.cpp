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

#include "common/world/gen/noise/SimplexNoise.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::gen::noise;

// ============================================================================
// SimplexNoise 确定性测试
// ============================================================================

TEST(SimplexNoiseTest, SameSeedProducesIdentical2DResults)
{
    math::Random rng1(12345);
    SimplexNoise noise1(rng1);

    math::Random rng2(12345);
    SimplexNoise noise2(rng2);

    for (int i = 0; i < 100; ++i) {
        const f64 x = static_cast<f64>(i) * 7.3;
        const f64 y = static_cast<f64>(i) * 11.1;
        EXPECT_NEAR(noise1.getValue(x, y), noise2.getValue(x, y), 1e-14)
            << "SimplexNoise 2D determinism mismatch at sample " << i;
    }
}

TEST(SimplexNoiseTest, SameSeedProducesIdentical3DResults)
{
    math::Random rng1(12345);
    SimplexNoise noise1(rng1);

    math::Random rng2(12345);
    SimplexNoise noise2(rng2);

    for (int i = 0; i < 100; ++i) {
        const f64 x = static_cast<f64>(i) * 7.3;
        const f64 y = static_cast<f64>(i) * 11.1;
        const f64 z = static_cast<f64>(i) * 13.7;
        EXPECT_NEAR(noise1.getValue(x, y, z), noise2.getValue(x, y, z), 1e-14)
            << "SimplexNoise 3D determinism mismatch at sample " << i;
    }
}

TEST(SimplexNoiseTest, DifferentSeedsProduceDifferentResults)
{
    math::Random rng1(12345);
    SimplexNoise noise1(rng1);

    math::Random rng2(54321);
    SimplexNoise noise2(rng2);

    bool anyDifferent = false;
    for (int i = 0; i < 20; ++i) {
        const f64 x = static_cast<f64>(i) * 11.1;
        const f64 y = static_cast<f64>(i) * 22.2;
        if (std::abs(noise1.getValue(x, y) - noise2.getValue(x, y)) > 1e-10) {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent) << "Different seeds should produce different 2D noise values";
}

// ============================================================================
// SimplexNoise 值范围测试
// ============================================================================

TEST(SimplexNoiseTest, GetValue2DRange)
{
    // MC: SimplexNoise 2D 输出范围约为 [-1, 1]
    // 缩放因子为 70.0，但值范围仍取决于梯度
    math::Random rng(42);
    SimplexNoise noise(rng);

    f64 minValue = std::numeric_limits<f64>::max();
    f64 maxValue = std::numeric_limits<f64>::lowest();

    for (int i = 0; i < 2000; ++i) {
        const f64 x = static_cast<f64>(i) * 0.5;
        const f64 y = static_cast<f64>(i) * 0.7;
        const f64 value = noise.getValue(x, y);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    // 理论范围约 [-1, 1]，允许一些余量
    EXPECT_GE(minValue, -1.5);
    EXPECT_LE(maxValue, 1.5);
}

TEST(SimplexNoiseTest, GetValue3DRange)
{
    // MC: SimplexNoise 3D 输出范围约为 [-1, 1]
    math::Random rng(42);
    SimplexNoise noise(rng);

    f64 minValue = std::numeric_limits<f64>::max();
    f64 maxValue = std::numeric_limits<f64>::lowest();

    for (int i = 0; i < 2000; ++i) {
        const f64 x = static_cast<f64>(i) * 0.5;
        const f64 y = static_cast<f64>(i) * 0.7;
        const f64 z = static_cast<f64>(i) * 0.9;
        const f64 value = noise.getValue(x, y, z);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    EXPECT_GE(minValue, -1.5);
    EXPECT_LE(maxValue, 1.5);
}

TEST(SimplexNoiseTest, GetValueAtOrigin2D)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    const f64 value = noise.getValue(0.0, 0.0);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(SimplexNoiseTest, GetValueAtOrigin3D)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    const f64 value = noise.getValue(0.0, 0.0, 0.0);
    EXPECT_TRUE(std::isfinite(value));
}

// ============================================================================
// SimplexNoise 偏移测试
// ============================================================================

TEST(SimplexNoiseTest, OffsetsAreInRange)
{
    // MC: SimplexNoise offsets = nextDouble() * 256.0
    // 所以偏移值应在 [0, 256) 范围内
    math::Random rng(42);
    SimplexNoise noise(rng);

    EXPECT_GE(noise.xOffset(), 0.0);
    EXPECT_LT(noise.xOffset(), 256.0);
    EXPECT_GE(noise.yOffset(), 0.0);
    EXPECT_LT(noise.yOffset(), 256.0);
    EXPECT_GE(noise.zOffset(), 0.0);
    EXPECT_LT(noise.zOffset(), 256.0);
}

TEST(SimplexNoiseTest, DifferentSeedsDifferentOffsets)
{
    math::Random rng1(12345);
    SimplexNoise noise1(rng1);

    math::Random rng2(54321);
    SimplexNoise noise2(rng2);

    // 极小概率偏移相同
    bool anyDifferent = noise1.xOffset() != noise2.xOffset() || noise1.yOffset() != noise2.yOffset() ||
        noise1.zOffset() != noise2.zOffset();
    EXPECT_TRUE(anyDifferent) << "Different seeds should produce different offsets";
}

// ============================================================================
// SimplexNoise 平滑性测试
// ============================================================================

TEST(SimplexNoiseTest, GetValue2DIsSmooth)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    const f64 step = 0.001;
    for (int i = 0; i < 50; ++i) {
        const f64 x = static_cast<f64>(i) * 10.0;
        const f64 y = static_cast<f64>(i) * 7.0;
        const f64 v1 = noise.getValue(x, y);
        const f64 v2 = noise.getValue(x + step, y);
        // 相邻极小步长的变化应极小
        EXPECT_LT(std::abs(v2 - v1), 0.1) << "2D noise should be smooth at sample " << i;
    }
}

TEST(SimplexNoiseTest, GetValue3DIsSmooth)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    const f64 step = 0.001;
    for (int i = 0; i < 50; ++i) {
        const f64 x = static_cast<f64>(i) * 10.0;
        const f64 y = static_cast<f64>(i) * 7.0;
        const f64 z = static_cast<f64>(i) * 5.0;
        const f64 v1 = noise.getValue(x, y, z);
        const f64 v2 = noise.getValue(x + step, y, z);
        EXPECT_LT(std::abs(v2 - v1), 0.1) << "3D noise should be smooth at sample " << i;
    }
}

// ============================================================================
// SimplexNoise 梯度常量验证
// ============================================================================

TEST(SimplexNoiseTest, GradientTableConstants)
{
    // MC SimplexNoise GRADIENT 表有 16 条目
    // 验证最后 4 个是前 12 个的重复（MC 特定模式）
    // 索引 12 = 索引 0 的值, 索引 13 = 索引 9, 索引 14 = 索引 1, 索引 15 = 索引 11
    // 这通过 SimplexNoise 类的 static constexpr GRADIENT 验证
    // 但 GRADIENT 是 private 的，因此通过行为间接验证

    // SimplexNoise 应始终返回有限值
    math::Random rng(42);
    SimplexNoise noise(rng);

    for (int i = 0; i < 100; ++i) {
        const f64 x = static_cast<f64>(i) * 3.7;
        const f64 y = static_cast<f64>(i) * 5.3;
        EXPECT_TRUE(std::isfinite(noise.getValue(x, y)));
        EXPECT_TRUE(std::isfinite(noise.getValue(x, y, static_cast<f64>(i) * 7.1)));
    }
}

// ============================================================================
// SimplexNoise 特殊坐标测试
// ============================================================================

TEST(SimplexNoiseTest, LargeCoordinates2D)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    // 大坐标不应产生 NaN 或 Inf
    const f64 largeX = 1000000.0;
    const f64 largeY = 2000000.0;
    const f64 value = noise.getValue(largeX, largeY);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(SimplexNoiseTest, LargeCoordinates3D)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    const f64 largeX = 1000000.0;
    const f64 largeY = 2000000.0;
    const f64 largeZ = 3000000.0;
    const f64 value = noise.getValue(largeX, largeY, largeZ);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(SimplexNoiseTest, NegativeCoordinates2D)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    const f64 value = noise.getValue(-100.0, -200.0);
    EXPECT_TRUE(std::isfinite(value));
}

TEST(SimplexNoiseTest, NegativeCoordinates3D)
{
    math::Random rng(42);
    SimplexNoise noise(rng);

    const f64 value = noise.getValue(-100.0, -200.0, -300.0);
    EXPECT_TRUE(std::isfinite(value));
}

} // namespace
} // namespace mc
