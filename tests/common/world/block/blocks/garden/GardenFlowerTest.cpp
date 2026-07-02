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
 */

#include <gtest/gtest.h>

#include "common/entity/effect/EffectType.hpp"
#include "common/item/Items.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/garden/CactusFlowerBlock.hpp"
#include "common/world/block/blocks/pale_garden/EyeblossomBlock.hpp"
#include "common/world/block/registry/GardenBlocks.hpp"
#include "common/world/block/registry/PaleGardenBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// CactusFlowerBlock 测试
// ============================================================================

class CactusFlowerBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
    }
};

TEST_F(CactusFlowerBlockTest, IsRegisteredAndNotNull)
{
    ASSERT_NE(mc::block_registry::GardenBlocks::CACTUS_FLOWER, nullptr);
}

TEST_F(CactusFlowerBlockTest, IsCactusFlowerBlockType)
{
    auto* cactusFlower = dynamic_cast<CactusFlowerBlock*>(mc::block_registry::GardenBlocks::CACTUS_FLOWER);
    ASSERT_NE(cactusFlower, nullptr);
}

TEST_F(CactusFlowerBlockTest, HasNoStewEffect)
{
    auto* flower = static_cast<FlowerBlock*>(mc::block_registry::GardenBlocks::CACTUS_FLOWER);
    EXPECT_FALSE(flower->hasStewEffect());
}

TEST_F(CactusFlowerBlockTest, IsNotSolidAndNoCollision)
{
    const auto& state = mc::block_registry::GardenBlocks::CACTUS_FLOWER->defaultState();
    EXPECT_FALSE(state.isSolid());
    EXPECT_TRUE(mc::block_registry::GardenBlocks::CACTUS_FLOWER->getCollisionShape(state).isEmpty());
}

TEST_F(CactusFlowerBlockTest, IsInSmallFlowersTag)
{
    EXPECT_TRUE(BlockTags::SMALL_FLOWERS().contains(ResourceLocation("minecraft", "cactus_flower")));
}

TEST_F(CactusFlowerBlockTest, IsInReplaceableByTreesTag)
{
    EXPECT_TRUE(BlockTags::REPLACEABLE_BY_TREES().contains(ResourceLocation("minecraft", "cactus_flower")));
}

// ============================================================================
// EyeblossomBlock 测试
// ============================================================================

class EyeblossomBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
    }
};

TEST_F(EyeblossomBlockTest, OpenEyeblossomIsNotNull)
{
    ASSERT_NE(mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM, nullptr);
}

TEST_F(EyeblossomBlockTest, ClosedEyeblossomIsNotNull)
{
    ASSERT_NE(mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM, nullptr);
}

TEST_F(EyeblossomBlockTest, OpenEyeblossomHasBlindnessStewEffect)
{
    auto* flower = static_cast<FlowerBlock*>(mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM);
    EXPECT_TRUE(flower->hasStewEffect());
    EXPECT_EQ(flower->getSuspiciousStewEffect(), static_cast<u32>(entity::effect::EffectType::Blindness));
    EXPECT_EQ(flower->getEffectDuration(), 11);
}

TEST_F(EyeblossomBlockTest, ClosedEyeblossomHasNauseaStewEffect)
{
    auto* flower = static_cast<FlowerBlock*>(mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM);
    EXPECT_TRUE(flower->hasStewEffect());
    EXPECT_EQ(flower->getSuspiciousStewEffect(), static_cast<u32>(entity::effect::EffectType::Nausea));
    EXPECT_EQ(flower->getEffectDuration(), 7);
}

TEST_F(EyeblossomBlockTest, OpenEyeblossomEmitsLight)
{
    auto* eyeblossom = static_cast<EyeblossomBlock*>(mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM);
    const auto& state = mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM->defaultState();
    EXPECT_EQ(eyeblossom->getLightLevel(state), 1);
}

TEST_F(EyeblossomBlockTest, ClosedEyeblossomEmitsNoLight)
{
    auto* eyeblossom = static_cast<EyeblossomBlock*>(mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM);
    const auto& state = mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM->defaultState();
    EXPECT_EQ(eyeblossom->getLightLevel(state), 0);
}

TEST_F(EyeblossomBlockTest, BothInSmallFlowersTag)
{
    EXPECT_TRUE(BlockTags::SMALL_FLOWERS().contains(ResourceLocation("minecraft", "open_eyeblossom")));
    EXPECT_TRUE(BlockTags::SMALL_FLOWERS().contains(ResourceLocation("minecraft", "closed_eyeblossom")));
}

// ============================================================================
// 花朵物品注册测试
// ============================================================================

class FlowerItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        mc::item::tag::ItemTags::initialize();
    }
};

TEST_F(FlowerItemRegistrationTest, CactusFlowerItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cactus_flower"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, WildflowersItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wildflowers"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, OpenEyeblossomItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "open_eyeblossom"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, ClosedEyeblossomItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "closed_eyeblossom"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, CactusFlowerInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cactus_flower"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}

TEST_F(FlowerItemRegistrationTest, WildflowersInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wildflowers"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}

TEST_F(FlowerItemRegistrationTest, OpenEyeblossomInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "open_eyeblossom"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}

TEST_F(FlowerItemRegistrationTest, ClosedEyeblossomInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "closed_eyeblossom"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}
