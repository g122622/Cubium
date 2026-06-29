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
#include "client/renderer/trident/particle/particles/effect/AshParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/CopperFireFlameParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/DustParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/DustPlumeParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/EggCrackParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/ElectricSparkParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/FireflyParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/FlashParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/GlowParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/InfestedParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/ItemPickupParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/LightParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/OmenParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/SculkChargeParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/SculkSoulParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/ShriekParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/SmallFlameParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/SonicBoomParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/TrialSpawnerParticle.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// AshParticle
// ============================================================================

TEST(AshParticleTest, Create_ReturnsNonNull)
{
    auto particle = AshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(AshParticleTest, RenderType_IsLit)
{
    auto particle = AshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(AshParticleTest, Texture_IsAsh)
{
    auto particle = AshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/ash");
}

TEST(AshParticleTest, LightColor_Is0xF0)
{
    auto particle = AshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(AshParticleTest, HasGravity)
{
    auto particle = AshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // AshParticle has DEFAULT_GRAVITY = 0.003
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// WhiteAshParticle
// ============================================================================

TEST(WhiteAshParticleTest, Create_ReturnsNonNull)
{
    auto particle = WhiteAshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(WhiteAshParticleTest, RenderType_IsLit)
{
    auto particle = WhiteAshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(WhiteAshParticleTest, Texture_IsWhiteAsh)
{
    auto particle = WhiteAshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/white_ash");
}

TEST(WhiteAshParticleTest, LightColor_Is0xF0)
{
    auto particle = WhiteAshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(WhiteAshParticleTest, HasGravity)
{
    auto particle = WhiteAshParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // WhiteAshParticle has DEFAULT_GRAVITY = 0.002
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// CopperFireFlameParticle
// ============================================================================

TEST(CopperFireFlameParticleTest, Create_ReturnsNonNull)
{
    auto particle = CopperFireFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(CopperFireFlameParticleTest, RenderType_IsLit)
{
    auto particle = CopperFireFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(CopperFireFlameParticleTest, Texture_IsCopperFireFlame)
{
    auto particle = CopperFireFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/copper_fire_flame");
}

TEST(CopperFireFlameParticleTest, LightColor_Is0xF0)
{
    auto particle = CopperFireFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(CopperFireFlameParticleTest, HasNoGravity)
{
    auto particle = CopperFireFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// DustParticle
// ============================================================================

TEST(DustParticleTest, Create_ReturnsNonNull)
{
    auto particle = DustParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(DustParticleTest, RenderType_IsOpaque)
{
    auto particle = DustParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(DustParticleTest, Texture_IsRedstone)
{
    auto particle = DustParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/redstone");
}

TEST(DustParticleTest, LightColor_Is0xF0)
{
    auto particle = DustParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(DustParticleTest, HasNoGravity)
{
    auto particle = DustParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

TEST(DustParticleTest, DefaultColorIsRed)
{
    auto particle = DustParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    const glm::vec4& color = particle->color();
    // Default DustParticle color is red
    EXPECT_GT(color.r, 0.5f);
    EXPECT_LT(color.g, 0.5f);
    EXPECT_LT(color.b, 0.5f);
}

TEST(DustParticleTest, CreateWithColor_SetsCustomColor)
{
    glm::vec4 customColor(0.0f, 0.0f, 1.0f, 1.0f); // Blue
    auto particle = DustParticle::createWithColor(glm::vec3(0.0f), glm::vec3(0.0f), nullptr, customColor);

    EXPECT_NE(particle, nullptr);
    const glm::vec4& color = particle->color();
    EXPECT_NEAR(color.r, 0.0f, 0.01f);
    EXPECT_NEAR(color.g, 0.0f, 0.01f);
    EXPECT_NEAR(color.b, 1.0f, 0.01f);
}

// ============================================================================
// DustColorTransitionParticle
// ============================================================================

TEST(DustColorTransitionParticleTest, Create_ReturnsNonNull)
{
    auto particle = DustColorTransitionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(DustColorTransitionParticleTest, RenderType_IsOpaque)
{
    auto particle = DustColorTransitionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(DustColorTransitionParticleTest, Texture_IsRedstone)
{
    auto particle = DustColorTransitionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/redstone");
}

TEST(DustColorTransitionParticleTest, LightColor_Is0xF0)
{
    auto particle = DustColorTransitionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(DustColorTransitionParticleTest, CreateWithColors_SetsFromColor)
{
    glm::vec4 fromColor(1.0f, 0.0f, 0.0f, 1.0f); // Red
    glm::vec4 toColor(0.0f, 0.0f, 1.0f, 1.0f);   // Blue
    auto particle =
        DustColorTransitionParticle::createWithColors(glm::vec3(0.0f), glm::vec3(0.0f), nullptr, fromColor, toColor);

    EXPECT_NE(particle, nullptr);
    const glm::vec4& color = particle->color();
    // At creation, color should match fromColor
    EXPECT_NEAR(color.r, 1.0f, 0.01f);
    EXPECT_NEAR(color.g, 0.0f, 0.01f);
    EXPECT_NEAR(color.b, 0.0f, 0.01f);
}

// ============================================================================
// DustPlumeParticle
// ============================================================================

TEST(DustPlumeParticleTest, Create_ReturnsNonNull)
{
    auto particle = DustPlumeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(DustPlumeParticleTest, RenderType_IsTranslucent)
{
    auto particle = DustPlumeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(DustPlumeParticleTest, Texture_IsCloud)
{
    auto particle = DustPlumeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/cloud");
}

// ============================================================================
// EggCrackParticle
// ============================================================================

TEST(EggCrackParticleTest, Create_ReturnsNonNull)
{
    auto particle = EggCrackParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(EggCrackParticleTest, RenderType_IsOpaque)
{
    auto particle = EggCrackParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(EggCrackParticleTest, Texture_IsPoof)
{
    auto particle = EggCrackParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/poof");
}

// ============================================================================
// ElectricSparkParticle
// ============================================================================

TEST(ElectricSparkParticleTest, Create_ReturnsNonNull)
{
    auto particle = ElectricSparkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(ElectricSparkParticleTest, RenderType_IsLit)
{
    auto particle = ElectricSparkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(ElectricSparkParticleTest, Texture_IsGlow)
{
    auto particle = ElectricSparkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/glow");
}

TEST(ElectricSparkParticleTest, LightColor_Is0xF0)
{
    auto particle = ElectricSparkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(ElectricSparkParticleTest, HasGravity)
{
    auto particle = ElectricSparkParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // ElectricSparkParticle has DEFAULT_GRAVITY = 0.01
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// FireflyParticle
// ============================================================================

TEST(FireflyParticleTest, Create_ReturnsNonNull)
{
    auto particle = FireflyParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(FireflyParticleTest, RenderType_IsLit)
{
    auto particle = FireflyParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(FireflyParticleTest, Texture_IsGlow)
{
    auto particle = FireflyParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/glow");
}

TEST(FireflyParticleTest, LightColor_Is0xF0)
{
    auto particle = FireflyParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(FireflyParticleTest, HasNoGravity)
{
    auto particle = FireflyParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// FlashParticle
// ============================================================================

TEST(FlashParticleTest, Create_ReturnsNonNull)
{
    auto particle = FlashParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(FlashParticleTest, RenderType_IsLit)
{
    auto particle = FlashParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(FlashParticleTest, Texture_IsFlash)
{
    auto particle = FlashParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/flash");
}

TEST(FlashParticleTest, LightColor_IsFullBrightness)
{
    auto particle = FlashParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // FULL_BRIGHTNESS = 15728880
    EXPECT_EQ(particle->getLightColor(nullptr), 15728880u);
}

TEST(FlashParticleTest, VeryShortLifetime)
{
    auto particle = FlashParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // FlashParticle DEFAULT_LIFETIME = 4.0
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }
    EXPECT_FALSE(particle->isAlive());
}

// ============================================================================
// GlowParticle
// ============================================================================

TEST(GlowParticleTest, Create_ReturnsNonNull)
{
    auto particle = GlowParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(GlowParticleTest, RenderType_IsLit)
{
    auto particle = GlowParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(GlowParticleTest, Texture_IsGlow)
{
    auto particle = GlowParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/glow");
}

TEST(GlowParticleTest, LightColor_Is0xF0)
{
    auto particle = GlowParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(GlowParticleTest, HasNoGravity)
{
    auto particle = GlowParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// InfestedParticle
// ============================================================================

TEST(InfestedParticleTest, Create_ReturnsNonNull)
{
    auto particle = InfestedParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(InfestedParticleTest, RenderType_IsTranslucent)
{
    auto particle = InfestedParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(InfestedParticleTest, Texture_IsCloud)
{
    auto particle = InfestedParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/cloud");
}

TEST(InfestedParticleTest, HasNoGravity)
{
    auto particle = InfestedParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// ItemPickupParticle
// ============================================================================

TEST(ItemPickupParticleTest, Create_ReturnsNonNull)
{
    auto particle = ItemPickupParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(ItemPickupParticleTest, RenderType_IsOpaque)
{
    auto particle = ItemPickupParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(ItemPickupParticleTest, Texture_IsPoof)
{
    auto particle = ItemPickupParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/poof");
}

TEST(ItemPickupParticleTest, HasNegativeGravity)
{
    auto particle = ItemPickupParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // ItemPickupParticle has DEFAULT_GRAVITY = -0.01 (rises upward)
    EXPECT_LT(particle->gravity(), 0.0);
}

// ============================================================================
// LightParticle
// ============================================================================

TEST(LightParticleTest, Create_ReturnsNonNull)
{
    auto particle = LightParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(LightParticleTest, RenderType_IsLit)
{
    auto particle = LightParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(LightParticleTest, Texture_IsFlame)
{
    auto particle = LightParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/flame");
}

TEST(LightParticleTest, LightColor_IsFullBrightness)
{
    auto particle = LightParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // LightParticle returns FULL_BRIGHTNESS = 15728880
    EXPECT_EQ(particle->getLightColor(nullptr), 15728880u);
}

TEST(LightParticleTest, HasNoGravity)
{
    auto particle = LightParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// OminousSpawningParticle
// ============================================================================

TEST(OminousSpawningParticleTest, Create_ReturnsNonNull)
{
    auto particle = OminousSpawningParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(OminousSpawningParticleTest, RenderType_IsTranslucent)
{
    auto particle = OminousSpawningParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(OminousSpawningParticleTest, Texture_IsEntityEffect)
{
    auto particle = OminousSpawningParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/entity_effect");
}

// ============================================================================
// RaidOmenParticle
// ============================================================================

TEST(RaidOmenParticleTest, Create_ReturnsNonNull)
{
    auto particle = RaidOmenParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(RaidOmenParticleTest, RenderType_IsTranslucent)
{
    auto particle = RaidOmenParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(RaidOmenParticleTest, Texture_IsEntityEffect)
{
    auto particle = RaidOmenParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/entity_effect");
}

// ============================================================================
// TrialOmenParticle
// ============================================================================

TEST(TrialOmenParticleTest, Create_ReturnsNonNull)
{
    auto particle = TrialOmenParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(TrialOmenParticleTest, RenderType_IsTranslucent)
{
    auto particle = TrialOmenParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(TrialOmenParticleTest, Texture_IsEntityEffect)
{
    auto particle = TrialOmenParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/entity_effect");
}

// ============================================================================
// SculkChargeParticle
// ============================================================================

TEST(SculkChargeParticleTest, Create_ReturnsNonNull)
{
    auto particle = SculkChargeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(SculkChargeParticleTest, RenderType_IsLit)
{
    auto particle = SculkChargeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(SculkChargeParticleTest, Texture_IsSculkCharge)
{
    auto particle = SculkChargeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/sculk_charge");
}

TEST(SculkChargeParticleTest, LightColor_Is0xF0)
{
    auto particle = SculkChargeParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

// ============================================================================
// SculkChargePopParticle
// ============================================================================

TEST(SculkChargePopParticleTest, Create_ReturnsNonNull)
{
    auto particle = SculkChargePopParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(SculkChargePopParticleTest, RenderType_IsLit)
{
    auto particle = SculkChargePopParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(SculkChargePopParticleTest, Texture_IsSculkChargePop)
{
    auto particle = SculkChargePopParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/sculk_charge_pop");
}

TEST(SculkChargePopParticleTest, LightColor_Is0xF0)
{
    auto particle = SculkChargePopParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

// ============================================================================
// SculkSoulParticle
// ============================================================================

TEST(SculkSoulParticleTest, Create_ReturnsNonNull)
{
    auto particle = SculkSoulParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(SculkSoulParticleTest, RenderType_IsTranslucent)
{
    auto particle = SculkSoulParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(SculkSoulParticleTest, Texture_IsSculkSoul)
{
    auto particle = SculkSoulParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/sculk_soul");
}

TEST(SculkSoulParticleTest, LightColor_Is0xF0)
{
    auto particle = SculkSoulParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

// ============================================================================
// ShriekParticle
// ============================================================================

TEST(ShriekParticleTest, Create_ReturnsNonNull)
{
    auto particle = ShriekParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(ShriekParticleTest, RenderType_IsTranslucent)
{
    auto particle = ShriekParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(ShriekParticleTest, Texture_IsSoul)
{
    auto particle = ShriekParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/soul");
}

// ============================================================================
// SmallFlameParticle
// ============================================================================

TEST(SmallFlameParticleTest, Create_ReturnsNonNull)
{
    auto particle = SmallFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(SmallFlameParticleTest, RenderType_IsLit)
{
    auto particle = SmallFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(SmallFlameParticleTest, Texture_IsFlame)
{
    auto particle = SmallFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/flame");
}

TEST(SmallFlameParticleTest, LightColor_Is0xF0)
{
    auto particle = SmallFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

TEST(SmallFlameParticleTest, HasNoGravity)
{
    auto particle = SmallFlameParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// SonicBoomParticle
// ============================================================================

TEST(SonicBoomParticleTest, Create_ReturnsNonNull)
{
    auto particle = SonicBoomParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(SonicBoomParticleTest, RenderType_IsLit)
{
    auto particle = SonicBoomParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(SonicBoomParticleTest, Texture_IsSonicBoom)
{
    auto particle = SonicBoomParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/sonic_boom");
}

TEST(SonicBoomParticleTest, LightColor_IsFullBrightness)
{
    auto particle = SonicBoomParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 15728880u);
}

TEST(SonicBoomParticleTest, HasNoGravity)
{
    auto particle = SonicBoomParticle::create(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), nullptr);

    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// TrialSpawnerDetectionParticle
// ============================================================================

TEST(TrialSpawnerDetectionParticleTest, Create_ReturnsNonNull)
{
    auto particle = TrialSpawnerDetectionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(TrialSpawnerDetectionParticleTest, RenderType_IsLit)
{
    auto particle = TrialSpawnerDetectionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(TrialSpawnerDetectionParticleTest, Texture_IsTrialSpawnerDetection)
{
    auto particle = TrialSpawnerDetectionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/trial_spawner_detection");
}

TEST(TrialSpawnerDetectionParticleTest, LightColor_Is0xF0)
{
    auto particle = TrialSpawnerDetectionParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

// ============================================================================
// TrialSpawnerDetectionOminousParticle
// ============================================================================

TEST(TrialSpawnerDetectionOminousParticleTest, Create_ReturnsNonNull)
{
    auto particle = TrialSpawnerDetectionOminousParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(TrialSpawnerDetectionOminousParticleTest, RenderType_IsLit)
{
    auto particle = TrialSpawnerDetectionOminousParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST(TrialSpawnerDetectionOminousParticleTest, Texture_IsTrialSpawnerDetectionOminous)
{
    auto particle = TrialSpawnerDetectionOminousParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/trial_spawner_detection_ominous");
}

TEST(TrialSpawnerDetectionOminousParticleTest, LightColor_Is0xF0)
{
    auto particle = TrialSpawnerDetectionOminousParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getLightColor(nullptr), 0xF0u);
}

} // namespace
} // namespace mc
