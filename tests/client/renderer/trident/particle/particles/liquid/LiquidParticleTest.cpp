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

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/particles/liquid/CherryLeavesDripParticle.hpp"
#include "client/renderer/trident/particle/particles/liquid/FallingNectarParticle.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// FallingNectarParticle
// ============================================================================

TEST(FallingNectarParticleTest, Create_ReturnsNonNull)
{
    auto particle = FallingNectarParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(FallingNectarParticleTest, RenderType_IsOpaque)
{
    auto particle = FallingNectarParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(FallingNectarParticleTest, Texture_IsDripHang)
{
    auto particle = FallingNectarParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/drip_hang");
}

TEST(FallingNectarParticleTest, HasGravity)
{
    auto particle = FallingNectarParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // FallingNectarParticle has DEFAULT_GRAVITY = 0.01
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// DrippingCherryLeavesParticle
// ============================================================================

TEST(DrippingCherryLeavesParticleTest, Create_ReturnsNonNull)
{
    auto particle = DrippingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(DrippingCherryLeavesParticleTest, RenderType_IsTranslucent)
{
    auto particle = DrippingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(DrippingCherryLeavesParticleTest, Texture_IsCherry)
{
    auto particle = DrippingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/cherry");
}

TEST(DrippingCherryLeavesParticleTest, HasNoGravity)
{
    auto particle = DrippingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // DrippingCherryLeavesParticle has DEFAULT_GRAVITY = 0.0 (hanging state)
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// FallingCherryLeavesParticle
// ============================================================================

TEST(FallingCherryLeavesParticleTest, Create_ReturnsNonNull)
{
    auto particle = FallingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(FallingCherryLeavesParticleTest, RenderType_IsTranslucent)
{
    auto particle = FallingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(FallingCherryLeavesParticleTest, Texture_IsCherry)
{
    auto particle = FallingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/cherry");
}

TEST(FallingCherryLeavesParticleTest, HasLowGravity)
{
    auto particle = FallingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // FallingCherryLeavesParticle has DEFAULT_GRAVITY = 0.003
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// LandingCherryLeavesParticle
// ============================================================================

TEST(LandingCherryLeavesParticleTest, Create_ReturnsNonNull)
{
    auto particle = LandingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(LandingCherryLeavesParticleTest, RenderType_IsTranslucent)
{
    auto particle = LandingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(LandingCherryLeavesParticleTest, Texture_IsCherry)
{
    auto particle = LandingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/cherry");
}

TEST(LandingCherryLeavesParticleTest, HasNoGravity)
{
    auto particle = LandingCherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // LandingCherryLeavesParticle has DEFAULT_GRAVITY = 0.0 (already landed)
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

} // namespace
} // namespace mc
