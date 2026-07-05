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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file ElytraSpeedValueTest.cpp
 * @brief elytra::computeSpeedValue 纯逻辑单元测试
 *
 * 验证 MC 1.21.11 HumanoidMobRenderer.extractHumanoidRenderState 中 speedValue 的填充公式：
 *
 *   speedValue = 1.0F;
 *   if (isFallFlying) {
 *       speedValue = (float)deltaMovement.lengthSqr();
 *       speedValue /= 0.2F;
 *       speedValue = speedValue * (speedValue * speedValue);  // 立方
 *   }
 *   if (speedValue < 1.0F) speedValue = 1.0F;
 *
 * 该自由函数同时被 GPU 管线路径（EntityRendererManager::_applyBipedElytraState）
 * 与 CPU 路径（PlayerRenderer::setModelVisibilities）调用。本测试直接验证公式分支，
 * 无需链接 Vulkan/EntityRendererManager。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/base/ElytraSpeedValue.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::client::renderer::entity::model::elytra;

namespace mc::client::renderer::entity::model {
namespace {

// ========== 非飞行时返回 1.0 ==========

TEST(ElytraSpeedValueTest, NotFallFlying_ReturnsOne)
{
    // 即使速度长度平方很大，非飞行时也应返回 1.0
    EXPECT_FLOAT_EQ(computeSpeedValue(false, 0.0f), 1.0f);
    EXPECT_FLOAT_EQ(computeSpeedValue(false, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(computeSpeedValue(false, 100.0f), 1.0f);
    EXPECT_FLOAT_EQ(computeSpeedValue(false, 10000.0f), 1.0f);
}

// ========== 飞行时返回 (lengthSq / 0.2)^3 ==========

TEST(ElytraSpeedValueTest, FallFlying_VelocityZero_ClampedToOne)
{
    // 速度为 0 时：(0 / 0.2)^3 = 0，钳制到 1.0
    EXPECT_FLOAT_EQ(computeSpeedValue(true, 0.0f), 1.0f);
}

TEST(ElytraSpeedValueTest, FallFlying_VelocitySmall_ClampedToOne)
{
    // 速度长度平方 = 0.1：(0.1 / 0.2)^3 = 0.5^3 = 0.125，钳制到 1.0
    EXPECT_FLOAT_EQ(computeSpeedValue(true, 0.1f), 1.0f);
}

TEST(ElytraSpeedValueTest, FallFlying_VelocityBoundary_0_2_ReturnsOne)
{
    // 速度长度平方 = 0.2：(0.2 / 0.2)^3 = 1.0，刚好达到下限
    EXPECT_FLOAT_EQ(computeSpeedValue(true, 0.2f), 1.0f);
}

TEST(ElytraSpeedValueTest, FallFlying_VelocityModerate_ProducesLargeValue)
{
    // 速度长度平方 = 1.0：(1.0 / 0.2)^3 = 5^3 = 125
    EXPECT_NEAR(computeSpeedValue(true, 1.0f), 125.0f, 1e-3f);
}

TEST(ElytraSpeedValueTest, FallFlying_VelocityLarge_ProducesHugeValue)
{
    // 速度长度平方 = 4.0：(4.0 / 0.2)^3 = 20^3 = 8000
    EXPECT_NEAR(computeSpeedValue(true, 4.0f), 8000.0f, 1.0f);
}

// ========== 立方公式验证 ==========

TEST(ElytraSpeedValueTest, FallFlying_FormulaIsCubeOfLengthSqOverDivisor)
{
    // 验证 speedValue = (lengthSq / 0.2)^3
    for (const f32 lengthSq : {0.3f, 0.5f, 0.8f, 1.5f, 2.0f, 3.0f}) {
        const f32 expected = std::pow(lengthSq / SPEED_DIVISOR, 3.0f);
        EXPECT_NEAR(computeSpeedValue(true, lengthSq), expected, 1e-4f) << "lengthSq=" << lengthSq;
    }
}

// ========== 钳制不变量：永远 >= 1.0 ==========

TEST(ElytraSpeedValueTest, AllStates_ReturnsAtLeastOne)
{
    // 不论输入如何，返回值永远 >= 1.0
    for (const bool isFallFlying : {false, true}) {
        for (const f32 lengthSq : {0.0f, 0.01f, 0.1f, 0.19f, 0.2f, 0.21f, 1.0f, 10.0f}) {
            const f32 result = computeSpeedValue(isFallFlying, lengthSq);
            EXPECT_GE(result, 1.0f) << "isFallFlying=" << isFallFlying << " lengthSq=" << lengthSq;
        }
    }
}

// ========== SPEED_DIVISOR 常量值验证 ==========

TEST(ElytraSpeedValueTest, SpeedDivisorConstant_IsZeroPointTwo)
{
    // MC 1.21.11 中 speedValue /= 0.2F 的常量值
    EXPECT_FLOAT_EQ(SPEED_DIVISOR, 0.2f);
}

} // namespace
} // namespace mc::client::renderer::entity::model
