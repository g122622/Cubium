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
#include "client/renderer/trident/particle/particles/ambient/NetherSporeParticle.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// CrimsonSporeParticle
// ============================================================================

TEST(CrimsonSporeParticleTest, Create_ReturnsNonNull)
{
    auto particle = CrimsonSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(CrimsonSporeParticleTest, RenderType_IsOpaque)
{
    auto particle = CrimsonSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(CrimsonSporeParticleTest, Texture_IsCrimsonSpore)
{
    auto particle = CrimsonSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/crimson_spore");
}

TEST(CrimsonSporeParticleTest, HasNoGravity)
{
    auto particle = CrimsonSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // CrimsonSporeParticle has DEFAULT_GRAVITY = 0.0
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// WarpedSporeParticle
// ============================================================================

TEST(WarpedSporeParticleTest, Create_ReturnsNonNull)
{
    auto particle = WarpedSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(WarpedSporeParticleTest, RenderType_IsOpaque)
{
    auto particle = WarpedSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(WarpedSporeParticleTest, Texture_IsWarpedSpore)
{
    auto particle = WarpedSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/warped_spore");
}

TEST(WarpedSporeParticleTest, HasNoGravity)
{
    auto particle = WarpedSporeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // WarpedSporeParticle has DEFAULT_GRAVITY = 0.0
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

} // namespace
} // namespace mc
