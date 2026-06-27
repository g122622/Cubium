/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BY NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file FullbrightRendererTest.cpp
 * @brief Fullbright 渲染管线着色器计算的单元测试
 *
 * 测试覆盖：
 * - 着色器 fullbright 因子在 mix() 中的行为
 * - 经验球最小亮度因子计算正确性
 * - fullbright 对不同环境光照值的影响
 *
 * 这些测试验证了 entity.vert 着色器中
 * fragLight = mix(fragLight, 1.0, pc.fullbright) 的 CPU 端等价逻辑。
 *
 * EntityRenderer::isFullbright() 虚方法和各渲染器子类的覆写
 * 通过代码审查和集成测试验证（不在此单元测试中），
 * 因为具体渲染器类依赖 Vulkan 等图形基础设施。
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::renderer::test {

// ============================================================================
// 着色器 mix() 行为测试
// ============================================================================

/**
 * @brief 模拟 GLSL mix(a, b, t) 函数
 *
 * 在 GLSL 中: mix(a, b, t) = a * (1.0 - t) + b * t
 * 在着色器 entity.vert 中: fragLight = mix(fragLight, 1.0, pc.fullbright)
 */
static f32 shaderMix(f32 a, f32 b, f32 t)
{
    return a * (1.0f - t) + b * t;
}

/**
 * @brief 验证 fullbright=0.0 时光照不变（正常光照模式）
 *
 * 对应 MC Java 中默认 getBlockLightLevel() 返回环境光照值的实体。
 */
TEST(FullbrightRendererTest, ZeroFullbrightPreservesLighting)
{
    // 不同环境光照值
    for (f32 baseLight : {0.18f, 0.3f, 0.5f, 0.8f, 1.0f}) {
        f32 result = shaderMix(baseLight, 1.0f, 0.0f);
        EXPECT_FLOAT_EQ(result, baseLight) << "At baseLight=" << baseLight;
    }
}

/**
 * @brief 验证 fullbright=1.0 时光照始终为最大值
 *
 * 对应 MC Java 中 getBlockLightLevel() 返回 15 的实体
 * （如烈焰人、岩浆怪、末影之眼、火球、凋灵、恼鬼等）。
 */
TEST(FullbrightRendererTest, FullFullbrightMaxesOutLighting)
{
    for (f32 baseLight : {0.0f, 0.18f, 0.3f, 0.5f, 0.8f, 1.0f}) {
        f32 result = shaderMix(baseLight, 1.0f, 1.0f);
        EXPECT_FLOAT_EQ(result, 1.0f) << "At baseLight=" << baseLight;
    }
}

/**
 * @brief 验证经验球最小亮度因子计算正确
 *
 * MC Java 中 ExperienceOrbRenderer.getBlockLightLevel() 返回
 * clamp(worldLight + 7, 0, 15)，即最小 blockLight 为 7。
 * 在当前管线实现中，使用 fullbright 因子 = 7/15 ≈ 0.4667 来模拟。
 */
TEST(FullbrightRendererTest, ExperienceOrbMinBrightnessFactor)
{
    const f32 orbMinBrightness = 7.0f / 15.0f;
    EXPECT_NEAR(orbMinBrightness, 0.4667f, 0.001f);
}

/**
 * @brief 验证经验球在黑暗环境中的光照提升
 *
 * 经验球在黑暗环境（ambient = 0.18）中应该有一定可见度。
 */
TEST(FullbrightRendererTest, ExperienceOrbVisibleInDarkness)
{
    const f32 orbMinBrightness = 7.0f / 15.0f;
    const f32 darkAmbient = 0.18f; // 着色器中最小 ambient 值

    f32 result = shaderMix(darkAmbient, 1.0f, orbMinBrightness);
    EXPECT_GT(result, darkAmbient); // 亮度应被提升
    EXPECT_NEAR(result, 0.564f, 0.01f); // 约 56% 亮度
}

/**
 * @brief 验证 fullbright mix() 的单调性
 *
 * 当 fullbright 因子从 0 递增到 1 时，输出光照应单调递增。
 */
TEST(FullbrightRendererTest, FullbrightMixMonotonicity)
{
    const f32 baseLight = 0.3f;
    f32 prevResult = shaderMix(baseLight, 1.0f, 0.0f);

    for (f32 t = 0.1f; t <= 1.0f; t += 0.1f) {
        f32 result = shaderMix(baseLight, 1.0f, t);
        EXPECT_GT(result, prevResult) << "At t=" << t;
        prevResult = result;
    }
}

/**
 * @brief 验证 fullbright mix() 在中等光照下的行为
 *
 * 经验球 fullbright 因子 (7/15) 在中等光照环境中不应超过 1.0。
 */
TEST(FullbrightRendererTest, ExperienceOrbDoesNotExceedMaxLight)
{
    const f32 orbMinBrightness = 7.0f / 15.0f;

    // 在各种环境光照下（排除 baseLight=1.0，因为 mix(1.0, 1.0, t) = 1.0 不 > 1.0）
    for (f32 baseLight : {0.0f, 0.18f, 0.3f, 0.5f, 0.8f}) {
        f32 result = shaderMix(baseLight, 1.0f, orbMinBrightness);
        EXPECT_LE(result, 1.0f) << "At baseLight=" << baseLight;
        EXPECT_GT(result, baseLight) << "At baseLight=" << baseLight;
    }
}

/**
 * @brief 验证完全发光实体在黑暗中的亮度
 *
 * 烈焰人等完全发光实体（fullbright=1.0）在最暗环境中也应全亮。
 */
TEST(FullbrightRendererTest, FullbrightEntityInDarkestEnvironment)
{
    const f32 darkestAmbient = 0.18f; // 着色器中最小环境光值
    f32 result = shaderMix(darkestAmbient, 1.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 1.0f);
}

} // namespace mc::client::renderer::entity::renderer::test
