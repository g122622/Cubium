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
#include "client/renderer/trident/particle/particles/block/DiggingParticle.hpp"
#include "client/renderer/trident/particle/particles/block/ItemParticle.hpp"
#include "client/renderer/trident/particle/particles/block/LeavesParticle.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

// ============================================================================
// ItemParticle
// ============================================================================

TEST(ItemParticleTest, Create_ReturnsNonNull)
{
    auto particle = ItemParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(ItemParticleTest, RenderType_IsTerrainSheet)
{
    auto particle = ItemParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
}

TEST(ItemParticleTest, Texture_IsGeneric)
{
    auto particle = ItemParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/generic");
}

TEST(ItemParticleTest, HasGravity)
{
    auto particle = ItemParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // ItemParticle has DEFAULT_GRAVITY = 0.03
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// CherryLeavesParticle
// ============================================================================

TEST(CherryLeavesParticleTest, Create_ReturnsNonNull)
{
    auto particle = CherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(CherryLeavesParticleTest, RenderType_IsTranslucent)
{
    auto particle = CherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(CherryLeavesParticleTest, Texture_IsCherry)
{
    auto particle = CherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/cherry");
}

TEST(CherryLeavesParticleTest, HasLowGravity)
{
    auto particle = CherryLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // CherryLeavesParticle has DEFAULT_GRAVITY = 0.003
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// PaleOakLeavesParticle
// ============================================================================

TEST(PaleOakLeavesParticleTest, Create_ReturnsNonNull)
{
    auto particle = PaleOakLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(PaleOakLeavesParticleTest, RenderType_IsTranslucent)
{
    auto particle = PaleOakLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

TEST(PaleOakLeavesParticleTest, Texture_IsPaleOak)
{
    auto particle = PaleOakLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/pale_oak");
}

TEST(PaleOakLeavesParticleTest, HasLowGravity)
{
    auto particle = PaleOakLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // PaleOakLeavesParticle has DEFAULT_GRAVITY = 0.003
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// TintedLeavesParticle
// ============================================================================

TEST(TintedLeavesParticleTest, Create_ReturnsNonNull)
{
    auto particle = TintedLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(TintedLeavesParticleTest, RenderType_IsOpaque)
{
    auto particle = TintedLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

TEST(TintedLeavesParticleTest, Texture_IsLeaf)
{
    auto particle = TintedLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:particle/leaf");
}

TEST(TintedLeavesParticleTest, HasLowGravity)
{
    auto particle = TintedLeavesParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // TintedLeavesParticle has DEFAULT_GRAVITY = 0.003
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// BlockMarkerParticle
// ============================================================================

TEST(BlockMarkerParticleTest, Create_ReturnsNonNull)
{
    auto particle = BlockMarkerParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(BlockMarkerParticleTest, RenderType_IsTerrainSheet)
{
    auto particle = BlockMarkerParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
}

TEST(BlockMarkerParticleTest, HasNoGravity)
{
    auto particle = BlockMarkerParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // BlockMarkerParticle does not move, no gravity
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

// ============================================================================
// BlockCrumbleParticle
// ============================================================================

TEST(BlockCrumbleParticleTest, Create_ReturnsNonNull)
{
    auto particle = BlockCrumbleParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST(BlockCrumbleParticleTest, RenderType_IsTerrainSheet)
{
    auto particle = BlockCrumbleParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
}

TEST(BlockCrumbleParticleTest, HasGravity)
{
    auto particle = BlockCrumbleParticle::create(glm::vec3(0.0f), glm::vec3(0.0f), nullptr);

    // BlockCrumbleParticle has DEFAULT_GRAVITY = 0.03
    EXPECT_GT(particle->gravity(), 0.0);
}

// ============================================================================
// BlockMarkerParticle createWithBlock 测试
// ============================================================================

class BlockMarkerWithBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(BlockMarkerWithBlockTest, CreateWithBlock_ReturnsNonNull)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = BlockMarkerParticle::createWithBlock(glm::vec3(0.0f), glm::vec3(0.0f), stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
}

TEST_F(BlockMarkerWithBlockTest, CreateWithBlock_NoGravity)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = BlockMarkerParticle::createWithBlock(glm::vec3(0.0f), glm::vec3(0.0f), stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

TEST_F(BlockMarkerWithBlockTest, CreateWithBlock_TextureIsStone)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = BlockMarkerParticle::createWithBlock(glm::vec3(0.0f), glm::vec3(0.0f), stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:block/stone");
}

// ============================================================================
// BlockCrumbleParticle createWithBlock 测试
// ============================================================================

class BlockCrumbleWithBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(BlockCrumbleWithBlockTest, CreateWithBlock_ReturnsNonNull)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = BlockCrumbleParticle::createWithBlock(glm::vec3(0.0f), glm::vec3(0.0f), stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
}

TEST_F(BlockCrumbleWithBlockTest, CreateWithBlock_HasGravity)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = BlockCrumbleParticle::createWithBlock(glm::vec3(0.0f), glm::vec3(0.0f), stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_GT(particle->gravity(), 0.0);
}

TEST_F(BlockCrumbleWithBlockTest, CreateWithBlock_TextureIsStone)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = BlockCrumbleParticle::createWithBlock(glm::vec3(0.0f), glm::vec3(0.0f), stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_EQ(particle->getTextureLocation().toString(), "minecraft:block/stone");
}

} // namespace
} // namespace mc
