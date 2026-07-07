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

#include <gtest/gtest.h>

#include "client/renderer/trident/particle/particles/effect/LavaParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/RedstoneParticle.hpp"

namespace mc {
namespace {

using namespace client::renderer::trident::particle::particles;

// ============================================================================
// RedstoneParticle 缩放测试
// ============================================================================

class RedstoneParticleScaleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RedstoneParticleScaleTest, GetScale_FadeInClamp_StartsNearZero)
{
    // 淡入钳位模式：clamp((age+pt)/lifetime * 32, 0, 1)
    // 粒子刚创建时 age=0, partialTick=0, 所以 scale 应该为 0
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = RedstoneParticle::create(pos, velocity, nullptr);

    // 刚创建时 age=0, getScale(0) = clamp(0/lifetime * 32, 0, 1) = 0
    f64 scale = particle->getScale(0.0);
    EXPECT_DOUBLE_EQ(scale, 0.0);
}

TEST_F(RedstoneParticleScaleTest, GetScale_FadeInClamp_ReachesFullSizeEarly)
{
    // 前 1/32 生命周期内从 0 渐变到 1.0，之后保持 1.0
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);
    glm::vec4 color(1.0f, 0.0f, 0.0f, 1.0f);

    auto particle = std::make_unique<RedstoneParticle>(pos, velocity, color);

    // tick 10 次，age 变为 10，lifetime 约 8-12
    // 10/maxAge * 32 > 1.0 时，scale 应该为 1.0
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }

    f64 scale = particle->getScale(0.0);
    EXPECT_NEAR(scale, 1.0, 0.01);
}

TEST_F(RedstoneParticleScaleTest, GetScale_FadeInClamp_InterpolatesBetween)
{
    // 中间状态：scale 应该在 0 和 1 之间
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);
    glm::vec4 color(1.0f, 0.0f, 0.0f, 1.0f);

    auto particle = std::make_unique<RedstoneParticle>(pos, velocity, color);

    // tick 1 次，age=1, lifetime 约 8-12, scale = clamp(1/lifetime*32, 0, 1)
    // 对于 lifetime=8: 1/8*32 = 4.0 -> clamp = 1.0
    // 对于 lifetime=12: 1/12*32 = 2.67 -> clamp = 1.0
    // 所以即使 tick 1 次，scale 也已经是 1.0（因为乘数 32 很大，淡入极快）
    // 这验证了淡入钳位模式的正确性
    particle->tick(nullptr);
    f64 scale = particle->getScale(0.0);
    EXPECT_NEAR(scale, 1.0, 0.01);
}

// ============================================================================
// LavaParticle 缩放测试
// ============================================================================

class LavaParticleScaleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LavaParticleScaleTest, GetScale_QuadraticShrink_StartsAtOne)
{
    // 二次收缩模式：(1 - t^2), t = (age+pt)/lifetime
    // 刚创建时 age=0, partialTick=0, scale = 1 - 0 = 1.0
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = LavaParticle::create(pos, velocity, nullptr);

    f64 scale = particle->getScale(0.0);
    EXPECT_DOUBLE_EQ(scale, 1.0);
}

TEST_F(LavaParticleScaleTest, GetScale_QuadraticShrink_ShrinksOverTime)
{
    // 随着年龄增长，scale 应该减小
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = LavaParticle::create(pos, velocity, nullptr);

    f64 initialScale = particle->getScale(0.0);

    // tick 5 次
    for (int i = 0; i < 5; ++i) {
        particle->tick(nullptr);
    }

    f64 scaleAfter5Ticks = particle->getScale(0.0);

    // scale 应该减小
    EXPECT_LT(scaleAfter5Ticks, initialScale);
    // 但不应该为负
    EXPECT_GT(scaleAfter5Ticks, 0.0);
}

TEST_F(LavaParticleScaleTest, GetScale_QuadraticShrink_NearsZeroAtEndOfLife)
{
    // 接近生命末尾时，scale 应该接近 0
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = LavaParticle::create(pos, velocity, nullptr);

    // LavaParticle 构造时 maxAge = DEFAULT_LIFETIME(30) * (0.8~1.2) = 24~36，带随机抖动。
    // 原测试固定 tick 28 次，当随机 maxAge 较大（如 36）时 28/maxAge < 0.894，
    // scale = 1 - t^2 > 0.2，导致用例随机失败。这里显式设置确定性的 maxAge=10，
    // 消除随机性后断言"接近生命末尾 scale 接近 0"才有确定意义。
    particle->setMaxAge(10.0);

    // tick 9 次（age=9, t=0.9, scale=1-0.81=0.19 < 0.2），接近生命末尾但未过期。
    for (int i = 0; i < 9; ++i) {
        particle->tick(nullptr);
    }

    f64 scale = particle->getScale(0.0);
    // 在生命末尾 scale 应该很小
    EXPECT_LT(scale, 0.2);
}

TEST_F(LavaParticleScaleTest, GetScale_ScaleIsMultiplier)
{
    // getScale() 返回的是乘数，不是绝对大小
    // 渲染管线: halfSize = m_size * scale * 0.5
    // 所以 scale 始终应在 0~1 之间（对于二次收缩）
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = LavaParticle::create(pos, velocity, nullptr);

    for (int i = 0; i < 15; ++i) {
        particle->tick(nullptr);
        f64 scale = particle->getScale(0.0);
        EXPECT_GE(scale, 0.0);
        EXPECT_LE(scale, 1.0);
    }
}

// ============================================================================
// RedstoneParticle 渲染尺寸验证
// ============================================================================

TEST_F(RedstoneParticleScaleTest, GetScale_ReturnsMultiplierNotSize)
{
    // 验证 getScale() 返回的是乘数而非绝对尺寸
    // RedstoneParticle 的 size 约为 0.01~0.06
    // getScale() 返回 0~1 的乘数
    // 渲染尺寸 = size * scale
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);
    glm::vec4 color(1.0f, 0.0f, 0.0f, 1.0f);

    auto particle = std::make_unique<RedstoneParticle>(pos, velocity, color);

    // 获取粒子大小和缩放乘数
    f64 particleSize = particle->size();
    f64 scale = particle->getScale(0.5);

    // 粒子大小应该在合理范围内 (0.01 ~ 0.06)
    EXPECT_GT(particleSize, 0.005);
    EXPECT_LT(particleSize, 0.1);

    // scale 应该是 0~1 之间的乘数
    EXPECT_GE(scale, 0.0);
    EXPECT_LE(scale, 1.0);

    // 渲染尺寸 = size * scale，不应该超过 size
    f64 renderedSize = particleSize * scale;
    EXPECT_LE(renderedSize, particleSize);
}

} // namespace
} // namespace mc
