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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/particles/block/ComposterParticle.hpp"
#include "client/renderer/trident/particle/particles/block/ScrapeParticle.hpp"
#include "client/renderer/trident/particle/particles/block/WaxParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/DamageIndicatorParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/NoteParticle.hpp"
#include "client/renderer/trident/particle/particles/mob/SquidInkParticle.hpp"
#include "client/renderer/trident/particle/particles/special/VaultConnectionParticle.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// ComposterParticle
// ============================================================================

TEST(ComposterParticleTest, Create_ReturnsNonNull)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(1.0f, 1.0f, 1.0f);

    auto particle = ComposterParticle::create(pos, velocity, nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(ComposterParticleTest, RenderType_IsOpaque)
{
    auto particle = ComposterParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(ComposterParticleTest, Texture_IsGeneric)
{
    auto particle = ComposterParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/generic");
}

TEST(ComposterParticleTest, HasNoGravityAndNoPhysics)
{
    auto particle = ComposterParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(ComposterParticleTest, ColorIsWhite)
{
    auto particle = ComposterParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_FLOAT_EQ(color.g, 1.0f);
    EXPECT_FLOAT_EQ(color.b, 1.0f);
    EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST(ComposterParticleTest, VelocityScaledDownBy002)
{
    // ComposterParticle scales velocity by 0.02 in constructor
    glm::vec3 velocity(100.0f, 100.0f, 100.0f);

    auto particle = ComposterParticle::create(glm::vec3(0.0f), velocity, nullptr);

    // velocity * 0.02 = (2.0, 2.0, 2.0)
    EXPECT_NEAR(particle->velocity().x, 2.0f, 0.01f);
    EXPECT_NEAR(particle->velocity().y, 2.0f, 0.01f);
    EXPECT_NEAR(particle->velocity().z, 2.0f, 0.01f);
}

TEST(ComposterParticleTest, ShortLifetime)
{
    auto particle = ComposterParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // lifetime = 3 + random(0~4), so maxAge is in [3, 7]
    EXPECT_GE(particle->maxAge(), 3.0);
    EXPECT_LE(particle->maxAge(), 7.0);

    // After 10 ticks, must be expired
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }
    EXPECT_FALSE(particle->isAlive());
}

// ============================================================================
// WaxOnParticle
// ============================================================================

TEST(WaxOnParticleTest, Create_ReturnsNonNull)
{
    auto particle = WaxOnParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(WaxOnParticleTest, RenderType_IsTranslucent)
{
    auto particle = WaxOnParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(WaxOnParticleTest, Texture_IsWaxOn)
{
    auto particle = WaxOnParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/wax_on");
}

TEST(WaxOnParticleTest, HasNoGravityAndNoPhysics)
{
    auto particle = WaxOnParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(WaxOnParticleTest, ColorIsHoneyOrange)
{
    auto particle = WaxOnParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_NEAR(color.r, 0.91f, 0.01f);
    EXPECT_NEAR(color.g, 0.55f, 0.01f);
    EXPECT_NEAR(color.b, 0.08f, 0.01f);
}

TEST(WaxOnParticleTest, Tick_UpdatesPosition)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 0.0f, 0.0f);

    auto particle = WaxOnParticle::create(pos, velocity, nullptr);
    glm::vec3 initialPos = particle->position();

    particle->tick(nullptr);

    // Position should have changed after tick
    EXPECT_NE(particle->position().x, initialPos.x);
}

// ============================================================================
// WaxOffParticle
// ============================================================================

TEST(WaxOffParticleTest, Create_ReturnsNonNull)
{
    auto particle = WaxOffParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(WaxOffParticleTest, RenderType_IsTranslucent)
{
    auto particle = WaxOffParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(WaxOffParticleTest, Texture_IsWaxOff)
{
    auto particle = WaxOffParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/wax_off");
}

TEST(WaxOffParticleTest, HasNoGravityAndNoPhysics)
{
    auto particle = WaxOffParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(WaxOffParticleTest, ColorIsWhitePink)
{
    auto particle = WaxOffParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_FLOAT_EQ(color.g, 0.9f);
    EXPECT_FLOAT_EQ(color.b, 1.0f);
}

TEST(WaxOffParticleTest, Tick_UpdatesPosition)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 0.0f, 0.0f);

    auto particle = WaxOffParticle::create(pos, velocity, nullptr);
    glm::vec3 initialPos = particle->position();

    particle->tick(nullptr);

    EXPECT_NE(particle->position().x, initialPos.x);
}

// ============================================================================
// ScrapeParticle
// ============================================================================

TEST(ScrapeParticleTest, Create_ReturnsNonNull)
{
    auto particle = ScrapeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(ScrapeParticleTest, RenderType_IsTranslucent)
{
    auto particle = ScrapeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(ScrapeParticleTest, Texture_IsScrape)
{
    auto particle = ScrapeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/scrape");
}

TEST(ScrapeParticleTest, HasNoGravityAndNoPhysics)
{
    auto particle = ScrapeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(ScrapeParticleTest, ColorIsGreenVariant)
{
    auto particle = ScrapeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    // ScrapeParticle randomly picks one of two green variants:
    // (0.29, 0.58, 0.51) or (0.43, 0.77, 0.62)
    // Green channel should be dominant
    EXPECT_GT(color.g, color.r);
    EXPECT_GT(color.g, color.b * 0.5f);
}

TEST(ScrapeParticleTest, Tick_UpdatesPosition)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 0.0f, 0.0f);

    auto particle = ScrapeParticle::create(pos, velocity, nullptr);
    glm::vec3 initialPos = particle->position();

    particle->tick(nullptr);

    // Position should have changed (velocity is scaled to 0.01, but still nonzero)
    EXPECT_NE(particle->position().x, initialPos.x);
}

// ============================================================================
// DamageIndicatorParticle
// ============================================================================

TEST(DamageIndicatorParticleTest, Create_ReturnsNonNull)
{
    auto particle = DamageIndicatorParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(DamageIndicatorParticleTest, RenderType_IsOpaque)
{
    auto particle = DamageIndicatorParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(DamageIndicatorParticleTest, HasNoGravityAndNoPhysics)
{
    auto particle = DamageIndicatorParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(DamageIndicatorParticleTest, ColorIsGoldenYellow)
{
    auto particle = DamageIndicatorParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    // Golden-yellow: (1.0, 0.9, 0.3)
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_NEAR(color.g, 0.9f, 0.01f);
    EXPECT_NEAR(color.b, 0.3f, 0.01f);
}

TEST(DamageIndicatorParticleTest, MovesUpward)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = DamageIndicatorParticle::create(pos, velocity, nullptr);

    // Constructor adds 0.2 to velocity.y for upward motion
    EXPECT_GT(particle->velocity().y, 0.0f);

    glm::vec3 initialPos = particle->position();
    particle->tick(nullptr);

    // Y position should have increased (moves upward)
    EXPECT_GT(particle->position().y, initialPos.y);
}

// ============================================================================
// NoteParticle
// ============================================================================

TEST(NoteParticleTest, Create_ReturnsNonNull)
{
    auto particle = NoteParticle::create(glm::vec3(0.0f), glm::vec3(0.0f, 0.5f, 0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(NoteParticleTest, RenderType_IsOpaque)
{
    auto particle = NoteParticle::create(glm::vec3(0.0f), glm::vec3(0.0f, 0.5f, 0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(NoteParticleTest, Texture_IsNote)
{
    auto particle = NoteParticle::create(glm::vec3(0.0f), glm::vec3(0.0f, 0.5f, 0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/note");
}

TEST(NoteParticleTest, HasNoGravityAndNoPhysics)
{
    auto particle = NoteParticle::create(glm::vec3(0.0f), glm::vec3(0.0f, 0.5f, 0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(NoteParticleTest, ColorDependsOnVelocityY)
{
    // velocity.y = 0.0 -> red hue (h=0 in HSV)
    auto p0 = NoteParticle::create(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 0.0f), nullptr);
    const glm::vec4& color0 = p0->color();
    EXPECT_NEAR(color0.r, 1.0f, 0.01f);
    EXPECT_NEAR(color0.g, 0.0f, 0.01f);
    EXPECT_NEAR(color0.b, 0.0f, 0.01f);

    // velocity.y = 1.0 -> should map to a different hue
    auto p1 = NoteParticle::create(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), nullptr);
    const glm::vec4& color1 = p1->color();
    // hue=6.0 wraps to same as 0.0 in HSV (red), so r should be high
    EXPECT_NEAR(color1.r, 1.0f, 0.01f);
}

// ============================================================================
// SquidInkParticle & GlowSquidInkParticle
// ============================================================================

TEST(SquidInkParticleTest, Create_ReturnsNonNull)
{
    auto particle = SquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(SquidInkParticleTest, RenderType_IsTranslucent)
{
    auto particle = SquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(SquidInkParticleTest, Texture_IsSquidInk)
{
    auto particle = SquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/squid_ink");
}

TEST(SquidInkParticleTest, ColorIsDarkBlueBlack)
{
    auto particle = SquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    // Dark blue-black ink: r and g near 0, b slightly higher, alpha starts at 0.8
    EXPECT_LT(color.r, 0.2f);
    EXPECT_LT(color.g, 0.2f);
    EXPECT_LT(color.b, 0.3f);
    EXPECT_NEAR(color.a, 0.8f, 0.01f);
}

TEST(GlowSquidInkParticleTest, Create_ReturnsNonNull)
{
    auto particle = GlowSquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(GlowSquidInkParticleTest, RenderType_IsTranslucent)
{
    auto particle = GlowSquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(GlowSquidInkParticleTest, Texture_IsGlowSquidInk)
{
    auto particle = GlowSquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/glow_squid_ink");
}

TEST(GlowSquidInkParticleTest, LightColor_IsSelfLit)
{
    auto particle = GlowSquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // GlowSquidInkParticle overrides getLightColor to return 0xF0
    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(GlowSquidInkParticleTest, ColorIsBrightCyanBlue)
{
    auto particle = GlowSquidInkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    // Bright cyan-blue: r low, g medium-high, b high
    EXPECT_LT(color.r, 0.5f);
    EXPECT_GT(color.g, 0.5f);
    EXPECT_GT(color.b, 0.7f);
}

// ============================================================================
// VaultConnectionParticle
// ============================================================================

TEST(VaultConnectionParticleTest, Create_ReturnsNonNull)
{
    auto particle = VaultConnectionParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(VaultConnectionParticleTest, RenderType_IsLit)
{
    auto particle = VaultConnectionParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(VaultConnectionParticleTest, Texture_IsVaultConnection)
{
    auto particle = VaultConnectionParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/vault_connection");
}

TEST(VaultConnectionParticleTest, HasNoGravityAndNoPhysics)
{
    auto particle = VaultConnectionParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(VaultConnectionParticleTest, LightColor_IsFullBrightness)
{
    auto particle = VaultConnectionParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), nullptr);

    // VaultConnectionParticle returns FULL_BRIGHTNESS = 15728880
    EXPECT_EQ(particle->getLightColor(nullptr), 15728880u);
}

TEST(VaultConnectionParticleTest, VelocityUsedAsTargetOffset)
{
    // In create(), velocity is interpreted as target offset:
    // targetPos = pos + velocity
    glm::vec3 pos(10.0f, 20.0f, 30.0f);
    glm::vec3 velocity(5.0f, 5.0f, 5.0f);

    auto particle = VaultConnectionParticle::create(pos, velocity, nullptr);

    // After construction, velocity is set to (0,0,0) and position is pos
    EXPECT_FLOAT_EQ(particle->position().x, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 20.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 30.0f);
}

TEST(VaultConnectionParticleTest, Tick_MovesTowardTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 10.0f, 10.0f);

    auto particle = VaultConnectionParticle::create(pos, velocity, nullptr);

    // After one tick, position should move toward target (pos + velocity)
    particle->tick(nullptr);

    // Position should have changed (moving toward target via lerp)
    // It should not still be at origin
    bool positionChanged =
        (particle->position().x != 0.0f || particle->position().y != 0.0f || particle->position().z != 0.0f);
    EXPECT_TRUE(positionChanged);
}

} // namespace
} // namespace mc
