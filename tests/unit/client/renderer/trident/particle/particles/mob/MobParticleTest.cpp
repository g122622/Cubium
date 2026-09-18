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
#include "client/renderer/trident/particle/particles/mob/SpitParticle.hpp"
#include "client/renderer/trident/particle/particles/mob/TotemParticle.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// SpitParticle
// ============================================================================

TEST(SpitParticleTest, Create_ReturnsNonNull)
{
    auto particle = SpitParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(SpitParticleTest, RenderType_IsTranslucent)
{
    auto particle = SpitParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(SpitParticleTest, Texture_IsSpit)
{
    auto particle = SpitParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/spit");
}

TEST(SpitParticleTest, HasGravity)
{
    auto particle = SpitParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // SpitParticle has DEFAULT_GRAVITY = 0.03 (ballistic trajectory)
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// TotemParticle
// ============================================================================

TEST(TotemParticleTest, Create_ReturnsNonNull)
{
    auto particle = TotemParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(TotemParticleTest, RenderType_IsTranslucent)
{
    auto particle = TotemParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(TotemParticleTest, Texture_IsTotemOfUndying)
{
    auto particle = TotemParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/totem_of_undying");
}

} // namespace
} // namespace mc
