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
 * @file EasingTypeTest.cpp
 * @brief 缓动类型（EasingType）单元测试
 *
 * 参考 MC 1.21.11 net.minecraft.util.Ease。
 *
 * 覆盖：
 * 1. CONSTANT / LINEAR 基础语义
 * 2. 全部具名缓动在端点 0/1 的收敛性（除刻意“过冲”的 back/elastic/bounce 外，
 *    均满足 apply(0)=0、apply(1)=1）
 * 3. 单调类缓动（quad/cubic/quart/quint/sine/circ/expo）在 [0,1] 上单调不减
 * 4. 具体数学值锚定（与 vanilla 公式逐一核对）
 */

#include "common/world/timeline/EasingType.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <array>

using namespace mc;
using namespace mc::world::timeline;

namespace {

/// 端点收敛：apply(0)=0 且 apply(1)=1
void ExpectConvergesAtEndpoints(const EasingType& easing, const char* name)
{
    EXPECT_NEAR(easing.apply(0.0f), 0.0f, 1e-4f) << name << " apply(0)";
    EXPECT_NEAR(easing.apply(1.0f), 1.0f, 1e-4f) << name << " apply(1)";
}

/// 在 [0,1] 上按固定步长采样，验证单调不减（允许极小数值误差）
void ExpectMonotonicNonDecreasing(const EasingType& easing, const char* name)
{
    f32 prev = easing.apply(0.0f);
    for (i32 i = 1; i <= 100; ++i) {
        const f32 f = static_cast<f32>(i) / 100.0f;
        const f32 cur = easing.apply(f);
        EXPECT_GE(cur + 1e-4f, prev) << name << " not monotonic at f=" << f;
        prev = cur;
    }
}

} // namespace

// ==================== 基础语义 ====================

TEST(EasingTypeTest, ConstantAlwaysZero)
{
    EXPECT_FLOAT_EQ(EasingType::CONSTANT.apply(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::CONSTANT.apply(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::CONSTANT.apply(1.0f), 0.0f);
}

TEST(EasingTypeTest, LinearIdentity)
{
    EXPECT_FLOAT_EQ(EasingType::LINEAR.apply(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::LINEAR.apply(0.25f), 0.25f);
    EXPECT_FLOAT_EQ(EasingType::LINEAR.apply(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(EasingType::LINEAR.apply(1.0f), 1.0f);
}

// ==================== 端点收敛 ====================

TEST(EasingTypeTest, AllEasingsConvergeAtEndpoints)
{
    // 除 back/elastic/bounce 族外，其余缓动在端点收敛到 0/1
    ExpectConvergesAtEndpoints(EasingType::IN_CUBIC, "IN_CUBIC");
    ExpectConvergesAtEndpoints(EasingType::IN_QUAD, "IN_QUAD");
    ExpectConvergesAtEndpoints(EasingType::IN_QUART, "IN_QUART");
    ExpectConvergesAtEndpoints(EasingType::IN_QUINT, "IN_QUINT");
    ExpectConvergesAtEndpoints(EasingType::IN_SINE, "IN_SINE");
    ExpectConvergesAtEndpoints(EasingType::IN_CIRC, "IN_CIRC");
    ExpectConvergesAtEndpoints(EasingType::IN_EXPO, "IN_EXPO");
    ExpectConvergesAtEndpoints(EasingType::IN_BACK, "IN_BACK");
    ExpectConvergesAtEndpoints(EasingType::IN_ELASTIC, "IN_ELASTIC");
    ExpectConvergesAtEndpoints(EasingType::IN_BOUNCE, "IN_BOUNCE");

    ExpectConvergesAtEndpoints(EasingType::OUT_CUBIC, "OUT_CUBIC");
    ExpectConvergesAtEndpoints(EasingType::OUT_QUAD, "OUT_QUAD");
    ExpectConvergesAtEndpoints(EasingType::OUT_QUART, "OUT_QUART");
    ExpectConvergesAtEndpoints(EasingType::OUT_QUINT, "OUT_QUINT");
    ExpectConvergesAtEndpoints(EasingType::OUT_SINE, "OUT_SINE");
    ExpectConvergesAtEndpoints(EasingType::OUT_CIRC, "OUT_CIRC");
    ExpectConvergesAtEndpoints(EasingType::OUT_EXPO, "OUT_EXPO");
    ExpectConvergesAtEndpoints(EasingType::OUT_BACK, "OUT_BACK");
    ExpectConvergesAtEndpoints(EasingType::OUT_ELASTIC, "OUT_ELASTIC");
    ExpectConvergesAtEndpoints(EasingType::OUT_BOUNCE, "OUT_BOUNCE");

    ExpectConvergesAtEndpoints(EasingType::IN_OUT_CUBIC, "IN_OUT_CUBIC");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_QUAD, "IN_OUT_QUAD");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_QUART, "IN_OUT_QUART");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_QUINT, "IN_OUT_QUINT");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_SINE, "IN_OUT_SINE");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_CIRC, "IN_OUT_CIRC");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_EXPO, "IN_OUT_EXPO");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_BACK, "IN_OUT_BACK");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_ELASTIC, "IN_OUT_ELASTIC");
    ExpectConvergesAtEndpoints(EasingType::IN_OUT_BOUNCE, "IN_OUT_BOUNCE");
}

// ==================== 单调性 ====================

TEST(EasingTypeTest, InEasingsAreMonotonic)
{
    ExpectMonotonicNonDecreasing(EasingType::IN_CUBIC, "IN_CUBIC");
    ExpectMonotonicNonDecreasing(EasingType::IN_QUAD, "IN_QUAD");
    ExpectMonotonicNonDecreasing(EasingType::IN_QUART, "IN_QUART");
    ExpectMonotonicNonDecreasing(EasingType::IN_QUINT, "IN_QUINT");
    ExpectMonotonicNonDecreasing(EasingType::IN_SINE, "IN_SINE");
    ExpectMonotonicNonDecreasing(EasingType::IN_CIRC, "IN_CIRC");
    ExpectMonotonicNonDecreasing(EasingType::IN_EXPO, "IN_EXPO");
}

TEST(EasingTypeTest, OutEasingsAreMonotonic)
{
    ExpectMonotonicNonDecreasing(EasingType::OUT_CUBIC, "OUT_CUBIC");
    ExpectMonotonicNonDecreasing(EasingType::OUT_QUAD, "OUT_QUAD");
    ExpectMonotonicNonDecreasing(EasingType::OUT_QUART, "OUT_QUART");
    ExpectMonotonicNonDecreasing(EasingType::OUT_QUINT, "OUT_QUINT");
    ExpectMonotonicNonDecreasing(EasingType::OUT_SINE, "OUT_SINE");
    ExpectMonotonicNonDecreasing(EasingType::OUT_CIRC, "OUT_CIRC");
    ExpectMonotonicNonDecreasing(EasingType::OUT_EXPO, "OUT_EXPO");
}

TEST(EasingTypeTest, InOutEasingsAreMonotonic)
{
    ExpectMonotonicNonDecreasing(EasingType::IN_OUT_CUBIC, "IN_OUT_CUBIC");
    ExpectMonotonicNonDecreasing(EasingType::IN_OUT_QUAD, "IN_OUT_QUAD");
    ExpectMonotonicNonDecreasing(EasingType::IN_OUT_QUART, "IN_OUT_QUART");
    ExpectMonotonicNonDecreasing(EasingType::IN_OUT_QUINT, "IN_OUT_QUINT");
    ExpectMonotonicNonDecreasing(EasingType::IN_OUT_SINE, "IN_OUT_SINE");
    ExpectMonotonicNonDecreasing(EasingType::IN_OUT_CIRC, "IN_OUT_CIRC");
    ExpectMonotonicNonDecreasing(EasingType::IN_OUT_EXPO, "IN_OUT_EXPO");
}

// ==================== 具体数值锚定 ====================

TEST(EasingTypeTest, InQuadEqualsSquare)
{
    EXPECT_FLOAT_EQ(EasingType::IN_QUAD.apply(0.5f), 0.25f);
    EXPECT_FLOAT_EQ(EasingType::IN_QUAD.apply(0.25f), 0.0625f);
}

TEST(EasingTypeTest, InCubicEqualsCube)
{
    EXPECT_FLOAT_EQ(EasingType::IN_CUBIC.apply(0.5f), 0.125f);
}

TEST(EasingTypeTest, InQuartAndInQuint)
{
    EXPECT_FLOAT_EQ(EasingType::IN_QUART.apply(0.5f), 0.0625f); // 0.5^4
    EXPECT_FLOAT_EQ(EasingType::IN_QUINT.apply(0.5f), 0.03125f); // 0.5^5
}

TEST(EasingTypeTest, OutQuadComplement)
{
    // outQuad(0.5) = 1 - (1-0.5)^2 = 0.75
    EXPECT_FLOAT_EQ(EasingType::OUT_QUAD.apply(0.5f), 0.75f);
}

TEST(EasingTypeTest, OutCubicComplement)
{
    // outCubic(0.5) = 1 - (1-0.5)^3 = 0.875
    EXPECT_FLOAT_EQ(EasingType::OUT_CUBIC.apply(0.5f), 0.875f);
}

TEST(EasingTypeTest, InSineHalfPi)
{
    // inSine(0.5) = 1 - cos(pi/4) ≈ 1 - 0.70710678 = 0.29289322
    EXPECT_NEAR(EasingType::IN_SINE.apply(0.5f), 0.29289322f, 1e-5f);
}

TEST(EasingTypeTest, OutSineHalfPi)
{
    // outSine(0.5) = sin(pi/4) ≈ 0.70710678
    EXPECT_NEAR(EasingType::OUT_SINE.apply(0.5f), 0.70710678f, 1e-5f);
}

TEST(EasingTypeTest, InOutSineSymmetric)
{
    // inOutSine(0.5) = -(cos(pi/2) - 1)/2 = 0.5
    EXPECT_NEAR(EasingType::IN_OUT_SINE.apply(0.5f), 0.5f, 1e-5f);
}

TEST(EasingTypeTest, InOutQuadMidpoint)
{
    // inOutQuad 在 0.5 处为 0.5
    EXPECT_NEAR(EasingType::IN_OUT_QUAD.apply(0.5f), 0.5f, 1e-5f);
}

TEST(EasingTypeTest, OutBounceKnownValues)
{
    // vanilla outBounce 分段公式在阈值处的确定性取值
    // outBounce(0) = 0，outBounce(1) = 7.5625*(1-0.95454544)^2 + 0.984375 = 1
    EXPECT_NEAR(EasingType::OUT_BOUNCE.apply(0.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(EasingType::OUT_BOUNCE.apply(1.0f), 1.0f, 1e-5f);
    // 第一个平台段末端附近应接近 1（弹跳特性）：outBounce 在 [0,1] 上不超过略大于 1
    EXPECT_GT(EasingType::OUT_BOUNCE.apply(0.5f), 0.5f);
}

TEST(EasingTypeTest, InElasticEndpointsExact)
{
    // vanilla inElastic 在 0/1 处短路返回精确值
    EXPECT_FLOAT_EQ(EasingType::IN_ELASTIC.apply(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::IN_ELASTIC.apply(1.0f), 1.0f);
}

TEST(EasingTypeTest, OutElasticEndpointsExact)
{
    EXPECT_FLOAT_EQ(EasingType::OUT_ELASTIC.apply(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::OUT_ELASTIC.apply(1.0f), 1.0f);
}

TEST(EasingTypeTest, InExpoEndpoints)
{
    EXPECT_FLOAT_EQ(EasingType::IN_EXPO.apply(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::IN_EXPO.apply(1.0f), 1.0f);
}

TEST(EasingTypeTest, OutExpoEndpoints)
{
    EXPECT_FLOAT_EQ(EasingType::OUT_EXPO.apply(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::OUT_EXPO.apply(1.0f), 1.0f);
}

TEST(EasingTypeTest, InOutExpoEndpoints)
{
    EXPECT_FLOAT_EQ(EasingType::IN_OUT_EXPO.apply(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingType::IN_OUT_EXPO.apply(1.0f), 1.0f);
    EXPECT_NEAR(EasingType::IN_OUT_EXPO.apply(0.5f), 0.5f, 1e-4f);
}

TEST(EasingTypeTest, InCircEndpoints)
{
    EXPECT_NEAR(EasingType::IN_CIRC.apply(0.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(EasingType::IN_CIRC.apply(1.0f), 1.0f, 1e-5f);
}

TEST(EasingTypeTest, OutCircEndpoints)
{
    EXPECT_NEAR(EasingType::OUT_CIRC.apply(0.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(EasingType::OUT_CIRC.apply(1.0f), 1.0f, 1e-5f);
}

TEST(EasingTypeTest, InOutCircEndpoints)
{
    EXPECT_NEAR(EasingType::IN_OUT_CIRC.apply(0.0f), 0.0f, 1e-4f);
    EXPECT_NEAR(EasingType::IN_OUT_CIRC.apply(1.0f), 1.0f, 1e-4f);
}

TEST(EasingTypeTest, InBackOvershootsBeforeOne)
{
    // inBack 在 1 处回到 1，但中段先下探到负值（回拉特性）
    f32 minValue = 0.0f;
    for (i32 i = 1; i < 100; ++i) {
        minValue = std::min(minValue, EasingType::IN_BACK.apply(static_cast<f32>(i) / 100.0f));
    }
    EXPECT_LT(minValue, 0.0f) << "inBack should undershoot below 0";
}

TEST(EasingTypeTest, OutBackOvershootsAboveOne)
{
    // outBack 在 1 处回到 1，但中段先冲过 1（过冲特性）
    f32 maxValue = 1.0f;
    for (i32 i = 1; i < 100; ++i) {
        maxValue = std::max(maxValue, EasingType::OUT_BACK.apply(static_cast<f32>(i) / 100.0f));
    }
    EXPECT_GT(maxValue, 1.0f) << "outBack should overshoot above 1";
}