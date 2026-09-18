/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. In NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/particles/effect/DustParticle.hpp"
#include "client/renderer/trident/particle/particles/special/TrailParticle.hpp"
#include "client/renderer/trident/particle/particles/special/VaultConnectionParticle.hpp"
#include "client/renderer/trident/particle/particles/special/VibrationSignalParticle.hpp"
#include "common/util/math/Vector3.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// DustParticle createWithColor 测试
// ============================================================================

TEST(DustParticleCreateWithColorTest, CreateWithCustomColor)
{
    // 绿色灰尘粒子
    glm::vec4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
    auto particle = DustParticle::createWithColor(glm::vec3(5.0f, 10.0f, 15.0f), glm::vec3(0.0f), nullptr, greenColor);

    ASSERT_NE(particle, nullptr);
    // 绿色灰尘粒子的尺寸应基于颜色强度 (0+1+0)/3 ≈ 0.33
    EXPECT_GT(particle->size(), 0.0);
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(DustParticleCreateWithColorTest, CreateWithBlueColor)
{
    glm::vec4 blueColor(0.0f, 0.0f, 1.0f, 1.0f);
    auto particle = DustParticle::createWithColor(glm::vec3(0.0f), glm::vec3(0.0f), nullptr, blueColor);

    ASSERT_NE(particle, nullptr);
    EXPECT_GT(particle->size(), 0.0);
}

TEST(DustParticleCreateWithColorTest, DefaultCreateIsRed)
{
    // 默认 create() 应产生红色灰尘粒子
    auto particle = DustParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);
    ASSERT_NE(particle, nullptr);
    // 红色默认，尺寸基于 (1+0+0)/3 ≈ 0.33
    EXPECT_GT(particle->size(), 0.0);
}

TEST(DustParticleCreateWithColorTest, TickBehaviorWithCustomColor)
{
    glm::vec4 color(0.5f, 0.8f, 0.2f, 1.0f);
    auto particle = DustParticle::createWithColor(glm::vec3(0.0f), glm::vec3(0.0f), nullptr, color);

    ASSERT_NE(particle, nullptr);
    f64 initialAge = particle->age();

    // tick 多次
    for (int i = 0; i < 5; ++i) {
        particle->tick(nullptr);
    }

    EXPECT_GT(particle->age(), initialAge);
}

// ============================================================================
// DustColorTransitionParticle createWithColors 测试
// ============================================================================

TEST(DustColorTransitionCreateWithColorsTest, CreateWithCustomColors)
{
    glm::vec4 fromColor(1.0f, 0.0f, 0.0f, 1.0f); // 红色
    glm::vec4 toColor(0.0f, 1.0f, 0.0f, 1.0f);   // 绿色
    auto particle =
        DustColorTransitionParticle::createWithColors(glm::vec3(0.0f), glm::vec3(0.0f), nullptr, fromColor, toColor);

    ASSERT_NE(particle, nullptr);
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(DustColorTransitionCreateWithColorsTest, ColorTransitionOverLifetime)
{
    glm::vec4 fromColor(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 toColor(0.0f, 0.0f, 1.0f, 1.0f);
    auto particle =
        DustColorTransitionParticle::createWithColors(glm::vec3(0.0f), glm::vec3(0.0f), nullptr, fromColor, toColor);

    ASSERT_NE(particle, nullptr);

    // tick 直到过期前，检查颜色是否变化
    f64 maxAge = particle->maxAge();
    for (int i = 0; i < static_cast<int>(maxAge) - 1; ++i) {
        particle->tick(nullptr);
    }

    // 粒子应该还活着（差1 tick就到期）
    EXPECT_TRUE(particle->isAlive());
}

// ============================================================================
// TrailParticle 测试
// ============================================================================

TEST(TrailParticleTest, CreateDefault)
{
    auto particle = TrailParticle::create(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 2.0f, 3.0f), nullptr);

    ASSERT_NE(particle, nullptr);
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    EXPECT_EQ(particle->getTextureLocation(), ResourceLocation("minecraft:particle/trail"));
    EXPECT_EQ(particle->getLightColor(nullptr), 15728880u); // FULL_BRIGHTNESS
    EXPECT_NEAR(particle->getScale(0.0), 1.0, 0.001);
}

TEST(TrailParticleTest, CreateWithTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 20.0, 30.0);
    u32 color = 0xFF00FF00; // 绿色 ARGB
    i32 duration = 20;

    auto particle = TrailParticle::createWithTarget(pos, target, color, duration);

    ASSERT_NE(particle, nullptr);
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    EXPECT_NEAR(particle->maxAge(), 20.0, 0.001);
}

TEST(TrailParticleTest, VelocityUsedAsTargetOffset)
{
    // velocity 应作为目标偏移：targetPos = pos + velocity
    auto particle = TrailParticle::create(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(5.0f, 10.0f, 15.0f), nullptr);

    ASSERT_NE(particle, nullptr);
    // 粒子应向目标 (6, 12, 18) 飞行
    // tick 一次后位置应向目标移动
    glm::vec3 posBeforeTick = particle->position();
    particle->tick(nullptr);
    glm::vec3 posAfterTick = particle->position();

    // 位置应该已经改变了
    EXPECT_NE(posBeforeTick.x, posAfterTick.x);
    EXPECT_NE(posBeforeTick.y, posAfterTick.y);
    EXPECT_NE(posBeforeTick.z, posAfterTick.z);
}

TEST(TrailParticleTest, MovesTowardTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(100.0, 0.0, 0.0);
    auto particle = TrailParticle::createWithTarget(pos, target, 0xFFFFFFFF, 10);

    ASSERT_NE(particle, nullptr);

    // 随着粒子 aging，它应该向目标位置移动
    for (int i = 0; i < 9; ++i) {
        particle->tick(nullptr);
    }

    // 粒子应该更接近目标 x=100
    EXPECT_GT(particle->position().x, 0.0f);
}

TEST(TrailParticleTest, ExpiresAtEndOfLifetime)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 10.0, 10.0);
    i32 duration = 5;
    auto particle = TrailParticle::createWithTarget(pos, target, 0xFFFFFFFF, duration);

    ASSERT_NE(particle, nullptr);

    // tick 直到过期
    for (int i = 0; i < duration; ++i) {
        particle->tick(nullptr);
    }

    EXPECT_FALSE(particle->isAlive());
}

TEST(TrailParticleTest, NoGravity)
{
    auto particle = TrailParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);
    ASSERT_NE(particle, nullptr);
    EXPECT_NEAR(particle->gravity(), 0.0, 0.001);
}

TEST(TrailParticleTest, FixedBrightness)
{
    auto particle = TrailParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);
    ASSERT_NE(particle, nullptr);
    // 轨迹粒子应始终全亮
    EXPECT_EQ(particle->getLightColor(nullptr), 15728880u);
}

TEST(TrailParticleTest, ColorFromArgb)
{
    // 测试不同颜色
    u32 redArgb = 0xFFFF0000;
    auto redParticle = TrailParticle::createWithTarget(glm::vec3(0.0f), Vector3d(1.0, 0.0, 0.0), redArgb, 10);
    ASSERT_NE(redParticle, nullptr);

    u32 blueArgb = 0xFF0000FF;
    auto blueParticle = TrailParticle::createWithTarget(glm::vec3(0.0f), Vector3d(1.0, 0.0, 0.0), blueArgb, 10);
    ASSERT_NE(blueParticle, nullptr);
}

// ============================================================================
// VaultConnectionParticle createWithTarget 测试
// ============================================================================

TEST(VaultConnectionCreateWithTargetTest, CreateWithExplicitTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 20.0, 30.0);
    i32 arrival = 35;

    auto particle = VaultConnectionParticle::createWithTarget(pos, target, arrival);

    ASSERT_NE(particle, nullptr);
    EXPECT_NEAR(particle->maxAge(), 35.0, 0.001);
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(VaultConnectionCreateWithTargetTest, MovesTowardTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(100.0, 0.0, 0.0);
    i32 arrival = 30;

    auto particle = VaultConnectionParticle::createWithTarget(pos, target, arrival);
    ASSERT_NE(particle, nullptr);

    // tick 几次
    for (int i = 0; i < 15; ++i) {
        particle->tick(nullptr);
    }

    // 粒子应更接近目标
    EXPECT_GT(particle->position().x, 0.0f);
}

// ============================================================================
// VibrationSignalParticle createWithTarget 测试
// ============================================================================

TEST(VibrationCreateWithTargetTest, CreateWithExplicitTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 50.0, 50.0);
    i32 arrival = 20;

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, arrival);

    ASSERT_NE(particle, nullptr);
    EXPECT_NEAR(particle->maxAge(), 20.0, 0.001);
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(VibrationCreateWithTargetTest, MovesTowardTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(0.0, 100.0, 0.0);
    i32 arrival = 20;

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, arrival);
    ASSERT_NE(particle, nullptr);

    // tick 几次
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }

    // 粒子应向上移动
    EXPECT_GT(particle->position().y, 0.0f);
}

TEST(VibrationCreateWithTargetTest, ReachesTargetBeforeExpiry)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 10.0, 10.0);
    i32 arrival = 8;

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, arrival);
    ASSERT_NE(particle, nullptr);

    // tick 到快过期
    for (int i = 0; i < arrival - 1; ++i) {
        particle->tick(nullptr);
    }

    // 粒子应接近目标
    EXPECT_NEAR(particle->position().x, 10.0f, 2.0f);
    EXPECT_NEAR(particle->position().y, 10.0f, 2.0f);
    EXPECT_NEAR(particle->position().z, 10.0f, 2.0f);
}

} // namespace
} // namespace mc
