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

#include "client/renderer/trident/particle/particles/effect/SpellParticle.hpp"
#include "common/core/Types.hpp"
#include <glm/glm.hpp>

namespace mc {
namespace {

using namespace client::renderer::trident::particle::particles;

// ============================================================================
// EntityEffectParticle 颜色提取测试
// ============================================================================

class EntityEffectParticleColorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EntityEffectParticleColorTest, Create_ZeroVelocity_FallsBackToPurple)
{
    // 零向量速度应回退到默认紫色 (0.5, 0.0, 0.5)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    // 构造函数中有随机颜色偏移，所以只检查颜色在合理范围内
    // 默认紫色 R=0.5, G=0.0, B=0.5，构造函数添加 ±20% 随机偏移
    // 但 EntityEffectParticle 构造函数直接使用传入颜色，无随机偏移
    // create() 中 fallback 为 (0.5, 0.0, 0.5, 0.5)，alpha 在 tick 中会变
    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 0.5f);
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    EXPECT_FLOAT_EQ(color.b, 0.5f);
    EXPECT_FLOAT_EQ(color.a, 0.5f);
}

TEST_F(EntityEffectParticleColorTest, Create_CustomColor_ExtractsFromVelocity)
{
    // 凋灵无敌阶段粒子颜色：紫色 (0.7, 0.7, 0.9)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.7f, 0.7f, 0.9f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 0.7f);
    EXPECT_FLOAT_EQ(color.g, 0.7f);
    EXPECT_FLOAT_EQ(color.b, 0.9f);
    EXPECT_FLOAT_EQ(color.a, 0.5f); // EntityEffectParticle alpha 固定为 0.5
}

TEST_F(EntityEffectParticleColorTest, Create_WitherChargedColor_YellowGreen)
{
    // 凋灵充能状态粒子颜色：黄绿色 (0.7, 0.7, 0.5)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.7f, 0.7f, 0.5f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 0.7f);
    EXPECT_FLOAT_EQ(color.g, 0.7f);
    EXPECT_FLOAT_EQ(color.b, 0.5f);
}

TEST_F(EntityEffectParticleColorTest, Create_RavagerStunColor_Gray)
{
    // 劫掠兽眩晕粒子颜色：灰色 (0.498, 0.514, 0.573)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.498f, 0.514f, 0.573f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 0.498f);
    EXPECT_FLOAT_EQ(color.g, 0.514f);
    EXPECT_FLOAT_EQ(color.b, 0.573f);
}

TEST_F(EntityEffectParticleColorTest, Create_RedColor)
{
    // 红色粒子 (1.0, 0.0, 0.0)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(1.0f, 0.0f, 0.0f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    EXPECT_FLOAT_EQ(color.b, 0.0f);
}

TEST_F(EntityEffectParticleColorTest, Create_BlueColor)
{
    // 蓝色粒子 (0.0, 0.0, 1.0) - 注意 G 也是 0，但 R 是 0，所以不是零向量
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 1.0f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 0.0f);
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    EXPECT_FLOAT_EQ(color.b, 1.0f);
}

TEST_F(EntityEffectParticleColorTest, Create_GreenOnly_NotZeroFallback)
{
    // 仅绿色通道有值 (0.0, 1.0, 0.0)，不应该回退到紫色
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 1.0f, 0.0f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 0.0f);
    EXPECT_FLOAT_EQ(color.g, 1.0f);
    EXPECT_FLOAT_EQ(color.b, 0.0f);
}

TEST_F(EntityEffectParticleColorTest, DirectConstruction_UsesProvidedColor)
{
    // 直接构造使用提供的颜色，不经过 velocity 提取
    glm::vec3 pos(1.0f, 2.0f, 3.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);
    glm::vec4 color(0.8f, 0.2f, 0.6f, 0.5f);

    EntityEffectParticle particle(pos, velocity, color);

    EXPECT_FLOAT_EQ(particle.color().r, 0.8f);
    EXPECT_FLOAT_EQ(particle.color().g, 0.2f);
    EXPECT_FLOAT_EQ(particle.color().b, 0.6f);
    EXPECT_FLOAT_EQ(particle.color().a, 0.5f);
}

TEST_F(EntityEffectParticleColorTest, Create_PositionPreserved)
{
    // 验证位置正确传递
    glm::vec3 pos(10.0f, 64.0f, -20.0f);
    glm::vec3 velocity(0.5f, 0.3f, 0.8f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_FLOAT_EQ(particle->position().x, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 64.0f);
    EXPECT_FLOAT_EQ(particle->position().z, -20.0f);
}

// ============================================================================
// SpellParticle 颜色提取测试
// ============================================================================

class SpellParticleColorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SpellParticleColorTest, Create_ZeroVelocity_FallsBackToPurple)
{
    // 零向量速度应回退到默认紫色 (0.5, 0.0, 1.0)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = SpellParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    // SpellParticle 构造函数会添加随机偏移 (0.8~1.0 倍)，
    // 所以颜色值会在 fallback 的基础上有所偏移
    const glm::vec4& color = particle->color();
    // R: 0.5 * (0.8~1.0) = 0.4~0.5
    EXPECT_GE(color.r, 0.35f);
    EXPECT_LE(color.r, 0.55f);
    // G: 0.0 * (0.8~1.0) = 0.0
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    // B: 1.0 * (0.8~1.0) = 0.8~1.0
    EXPECT_GE(color.b, 0.75f);
    EXPECT_LE(color.b, 1.05f);
    // alpha: 构造函数设置为 0.8
    EXPECT_FLOAT_EQ(color.a, 0.8f);
}

TEST_F(SpellParticleColorTest, Create_CustomColor_ExtractsFromVelocity)
{
    // 自定义颜色 (0.8, 0.2, 0.4)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.8f, 0.2f, 0.4f);

    auto particle = SpellParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    // SpellParticle 构造函数添加随机偏移 (0.8~1.0 倍)
    const glm::vec4& color = particle->color();
    // R: 0.8 * (0.8~1.0) = 0.64~0.8
    EXPECT_GE(color.r, 0.60f);
    EXPECT_LE(color.r, 0.85f);
    // G: 0.2 * (0.8~1.0) = 0.16~0.2
    EXPECT_GE(color.g, 0.14f);
    EXPECT_LE(color.g, 0.22f);
    // B: 0.4 * (0.8~1.0) = 0.32~0.4
    EXPECT_GE(color.b, 0.30f);
    EXPECT_LE(color.b, 0.42f);
}

TEST_F(SpellParticleColorTest, DirectConstruction_UsesProvidedColor)
{
    // 直接构造使用提供的颜色（带随机偏移）
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);
    glm::vec4 color(0.9f, 0.1f, 0.5f, 1.0f);

    SpellParticle particle(pos, velocity, color);

    // SpellParticle 构造函数会应用随机偏移
    // R: 0.9 * (0.8~1.0) = 0.72~0.9
    EXPECT_GE(particle.color().r, 0.68f);
    EXPECT_LE(particle.color().r, 0.95f);
    // G: 0.1 * (0.8~1.0) = 0.08~0.1
    EXPECT_GE(particle.color().g, 0.07f);
    EXPECT_LE(particle.color().g, 0.12f);
    // B: 0.5 * (0.8~1.0) = 0.4~0.5
    EXPECT_GE(particle.color().b, 0.38f);
    EXPECT_LE(particle.color().b, 0.52f);
    // alpha 固定为 0.8
    EXPECT_FLOAT_EQ(particle.color().a, 0.8f);
}

// ============================================================================
// InstantSpellParticle 颜色提取测试
// ============================================================================

class InstantSpellParticleColorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(InstantSpellParticleColorTest, Create_ZeroVelocity_FallsBackToWhite)
{
    // 零向量速度应回退到默认白色 (1.0, 1.0, 1.0)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = InstantSpellParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    // InstantSpellParticle 构造函数会将颜色乘以 1.2 倍（更亮）
    // fallback: R=1.0*1.2=1.2, G=1.0*1.2=1.2, B=1.0*1.2=1.2
    // 但 OpenGL 会自动钳制到 [0,1]，这里只验证颜色偏亮
    const glm::vec4& color = particle->color();
    EXPECT_GE(color.r, 1.0f); // 白色回退，乘以 1.2 后至少为 1.0
    EXPECT_GE(color.g, 1.0f);
    EXPECT_GE(color.b, 1.0f);
    // alpha: 构造函数设置为 1.0
    EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST_F(InstantSpellParticleColorTest, Create_CustomColor_ExtractsFromVelocity)
{
    // 自定义颜色 (0.6, 0.4, 0.8)
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.6f, 0.4f, 0.8f);

    auto particle = InstantSpellParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    // InstantSpellParticle 构造函数将颜色乘以 1.2
    // R: 0.6 * 1.2 = 0.72
    // G: 0.4 * 1.2 = 0.48
    // B: 0.8 * 1.2 = 0.96
    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 0.72f);
    EXPECT_FLOAT_EQ(color.g, 0.48f);
    EXPECT_FLOAT_EQ(color.b, 0.96f);
    EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST_F(InstantSpellParticleColorTest, Create_RedColor)
{
    // 红色 (1.0, 0.0, 0.0) - R 非零所以不会回退到白色
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(1.0f, 0.0f, 0.0f);

    auto particle = InstantSpellParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    // R: 1.0 * 1.2 = 1.2
    // G: 0.0 * 1.2 = 0.0
    // B: 0.0 * 1.2 = 0.0
    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 1.2f);
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    EXPECT_FLOAT_EQ(color.b, 0.0f);
}

// ============================================================================
// 颜色传递一致性测试
// ============================================================================

class ParticleColorConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ParticleColorConsistencyTest, EntityEffectParticle_CreateAndDirectConstruction_MatchForNonZeroVelocity)
{
    // 使用非零 velocity 的 create() 应该等效于直接构造传入提取的颜色
    glm::vec3 pos(5.0f, 10.0f, 15.0f);
    glm::vec3 velocity(0.3f, 0.6f, 0.9f);

    auto created = EntityEffectParticle::create(pos, velocity, nullptr);
    EntityEffectParticle direct(pos, velocity, glm::vec4(0.3f, 0.6f, 0.9f, 0.5f));

    // 两者颜色应该相同（EntityEffectParticle 构造函数不做随机偏移）
    EXPECT_FLOAT_EQ(created->color().r, direct.color().r);
    EXPECT_FLOAT_EQ(created->color().g, direct.color().g);
    EXPECT_FLOAT_EQ(created->color().b, direct.color().b);
    EXPECT_FLOAT_EQ(created->color().a, direct.color().a);
}

TEST_F(ParticleColorConsistencyTest, EntityEffectParticle_TickFadeOut)
{
    // EntityEffectParticle 在 tick 中 alpha 应该逐渐减小
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.5f, 0.5f, 0.5f);

    auto particle = EntityEffectParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    f32 initialAlpha = particle->color().a;
    EXPECT_FLOAT_EQ(initialAlpha, 0.5f);

    // tick 直到生命末期
    for (int i = 0; i < 30; ++i) {
        particle->tick(nullptr);
    }

    // alpha 应该减小
    EXPECT_LT(particle->color().a, initialAlpha);
}

TEST_F(ParticleColorConsistencyTest, SpellParticle_TickFadeOut)
{
    // SpellParticle 在 tick 中 alpha 应该逐渐减小
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.5f, 0.0f, 0.5f);

    auto particle = SpellParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    f32 initialAlpha = particle->color().a;

    // tick 直到生命末期
    for (int i = 0; i < 30; ++i) {
        particle->tick(nullptr);
    }

    // alpha 应该减小
    EXPECT_LT(particle->color().a, initialAlpha);
}

TEST_F(ParticleColorConsistencyTest, InstantSpellParticle_TickFadeOut)
{
    // InstantSpellParticle 在 tick 中 alpha 应该逐渐减小
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.5f, 0.5f, 0.5f);

    auto particle = InstantSpellParticle::create(pos, velocity, nullptr);
    ASSERT_NE(particle, nullptr);

    f32 initialAlpha = particle->color().a;

    // tick 直到生命末期
    for (int i = 0; i < 30; ++i) {
        particle->tick(nullptr);
    }

    // alpha 应该减小
    EXPECT_LT(particle->color().a, initialAlpha);
}

} // namespace
} // namespace mc
