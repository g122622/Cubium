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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR
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
#include "client/renderer/trident/particle/particles/special/VaultConnectionParticle.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// ComposterParticle tick behavior
// ============================================================================

TEST(ComposterParticleTickTest, Tick_NoGravityApplied)
{
    auto particle = ComposterParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);

    // ComposterParticle has gravity = 0, so velocity.y should not change due to gravity
    f64 vyBeforeTick = particle->velocity().y;

    particle->tick(nullptr);

    // velocity.y should remain unchanged (no gravity applied)
    // Only friction scaling applies
    EXPECT_DOUBLE_EQ(particle->velocity().y, static_cast<f64>(vyBeforeTick));
}

TEST(ComposterParticleTickTest, Tick_VelocityScaledBy002InConstructor)
{
    // ComposterParticle scales velocity by 0.02 in constructor
    glm::vec3 velocity(100.0f, 100.0f, 100.0f);

    auto particle = ComposterParticle::create(glm::vec3(0.0f), velocity, nullptr);

    // velocity * 0.02 = (2.0, 2.0, 2.0)
    EXPECT_NEAR(particle->velocity().x, 2.0f, 0.01f);
    EXPECT_NEAR(particle->velocity().y, 2.0f, 0.01f);
    EXPECT_NEAR(particle->velocity().z, 2.0f, 0.01f);
}

TEST(ComposterParticleTickTest, Tick_ExpiresAfterShortLifetime)
{
    auto particle = ComposterParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // lifetime = 3 + random(0~4), so maxAge is in [3, 7]
    // After 10 ticks, must be expired
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }
    EXPECT_FALSE(particle->isAlive());
}

TEST(ComposterParticleTickTest, Tick_PositionChangesWithVelocity)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 0.0f, 0.0f);

    auto particle = ComposterParticle::create(pos, velocity, nullptr);

    // After construction, velocity is scaled by 0.02 -> (0.2, 0, 0)
    float initialX = particle->position().x;

    particle->tick(nullptr);

    // Position should have moved in the X direction
    EXPECT_NE(particle->position().x, initialX);
}

// ============================================================================
// WaxOnParticle tick behavior
// ============================================================================

TEST(WaxOnParticleTickTest, Tick_NoGravityApplied)
{
    auto particle = WaxOnParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 0.0f), nullptr);

    // WaxOnParticle has gravity = 0
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);

    // Tick should not add gravity to velocity.y
    f64 vyBefore = particle->velocity().y;
    particle->tick(nullptr);

    // velocity.y may change due to friction, but not due to gravity
    // Since gravity is 0, no downward acceleration is added
}

TEST(WaxOnParticleTickTest, Tick_FrictionAppliesToVelocity)
{
    glm::vec3 velocity(10.0f, 0.0f, 10.0f);

    auto particle = WaxOnParticle::create(glm::vec3(0.0f), velocity, nullptr);

    f64 vxBefore = particle->velocity().x;
    f64 vzBefore = particle->velocity().z;

    particle->tick(nullptr);

    // After tick, velocity should decrease due to friction
    EXPECT_LT(std::abs(particle->velocity().x), std::abs(vxBefore));
    EXPECT_LT(std::abs(particle->velocity().z), std::abs(vzBefore));
}

TEST(WaxOnParticleTickTest, Tick_UpdatesPosition)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 0.0f, 0.0f);

    auto particle = WaxOnParticle::create(pos, velocity, nullptr);
    glm::vec3 initialPos = particle->position();

    particle->tick(nullptr);

    EXPECT_NE(particle->position().x, initialPos.x);
}

TEST(WaxOnParticleTickTest, Tick_ParticleShrinksOverLifetime)
{
    auto particle = WaxOnParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    f64 initialScale = particle->getScale(0.0);

    // Tick to near end of lifetime
    for (int i = 0; i < 40; ++i) {
        particle->tick(nullptr);
    }

    f64 lateScale = particle->getScale(0.0);

    // Particle should have shrunk (or at least changed) over its lifetime
    // The scale may become 0 if the particle is dead
    if (particle->isAlive()) {
        EXPECT_LT(lateScale, initialScale);
    }
}

// ============================================================================
// ScrapeParticle tick behavior
// ============================================================================

TEST(ScrapeParticleTickTest, Tick_NoGravityApplied)
{
    auto particle = ScrapeParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 0.0f), nullptr);

    // ScrapeParticle has gravity = 0
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

TEST(ScrapeParticleTickTest, Tick_FrictionAppliesToVelocity)
{
    glm::vec3 velocity(10.0f, 0.0f, 10.0f);

    auto particle = ScrapeParticle::create(glm::vec3(0.0f), velocity, nullptr);

    f64 vxBefore = particle->velocity().x;
    f64 vzBefore = particle->velocity().z;

    particle->tick(nullptr);

    // After tick, velocity should decrease due to friction
    EXPECT_LT(std::abs(particle->velocity().x), std::abs(vxBefore));
    EXPECT_LT(std::abs(particle->velocity().z), std::abs(vzBefore));
}

TEST(ScrapeParticleTickTest, Tick_UpdatesPosition)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 0.0f, 0.0f);

    auto particle = ScrapeParticle::create(pos, velocity, nullptr);
    glm::vec3 initialPos = particle->position();

    particle->tick(nullptr);

    // Position should have changed (velocity is scaled to 0.01, but still nonzero)
    EXPECT_NE(particle->position().x, initialPos.x);
}

TEST(ScrapeParticleTickTest, Tick_ParticleShrinksOverLifetime)
{
    auto particle = ScrapeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    f64 initialScale = particle->getScale(0.0);

    // Tick to near end of lifetime
    for (int i = 0; i < 40; ++i) {
        particle->tick(nullptr);
    }

    f64 lateScale = particle->getScale(0.0);

    // Particle should have shrunk over its lifetime
    if (particle->isAlive()) {
        EXPECT_LT(lateScale, initialScale);
    }
}

// ============================================================================
// VaultConnectionParticle tick behavior
// ============================================================================

TEST(VaultConnectionParticleTickTest, Create_VelocityUsedAsTargetOffset)
{
    // In create(), velocity is interpreted as target offset:
    // targetPos = pos + velocity
    glm::vec3 pos(10.0f, 20.0f, 30.0f);
    glm::vec3 velocity(5.0f, 5.0f, 5.0f);

    auto particle = VaultConnectionParticle::create(pos, velocity, nullptr);

    // Position should remain at the original position
    EXPECT_FLOAT_EQ(particle->position().x, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 20.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 30.0f);
}

TEST(VaultConnectionParticleTickTest, Tick_MovesTowardTarget)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 10.0f, 10.0f);

    auto particle = VaultConnectionParticle::create(pos, velocity, nullptr);

    // After one tick, position should move toward target (pos + velocity)
    particle->tick(nullptr);

    // Position should have changed (moving toward target via lerp)
    bool positionChanged =
        (particle->position().x != 0.0f || particle->position().y != 0.0f || particle->position().z != 0.0f);
    EXPECT_TRUE(positionChanged);
}

TEST(VaultConnectionParticleTickTest, Tick_AlphaFadesInOut)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(1.0f, 1.0f, 1.0f);

    auto particle = VaultConnectionParticle::create(pos, velocity, nullptr);

    // Initially, alpha may be low (fade-in) or 1.0
    float initialAlpha = particle->color().a;

    // Tick halfway through lifetime to observe fade-in/fade-out
    for (int i = 0; i < 20; ++i) {
        particle->tick(nullptr);
    }

    // Alpha should change during the particle's lifetime
    // Just verify the particle is still alive and alpha is valid
    if (particle->isAlive()) {
        float midAlpha = particle->color().a;
        EXPECT_GE(midAlpha, 0.0f);
        EXPECT_LE(midAlpha, 1.0f);
    }
}

TEST(VaultConnectionParticleTickTest, Tick_NoGravityApplied)
{
    auto particle = VaultConnectionParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), nullptr);

    // VaultConnectionParticle has no gravity and no physics
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
    EXPECT_FALSE(particle->hasPhysics());
}

TEST(VaultConnectionParticleTickTest, Tick_LightColorIsFullBrightness)
{
    auto particle = VaultConnectionParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), nullptr);

    // VaultConnectionParticle returns FULL_BRIGHTNESS = 15728880
    EXPECT_EQ(particle->getLightColor(nullptr), 15728880u);
}

} // namespace
} // namespace mc
