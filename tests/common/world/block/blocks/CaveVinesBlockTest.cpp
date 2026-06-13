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

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/cave/CaveVinesBlock.hpp"
#include "common/world/block/blocks/cave/CaveVinesPlantBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// CaveVinesBlock 测试
// ============================================================================

class CaveVinesBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();

        m_vines =
            std::make_unique<CaveVinesBlock>(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    }

    void TearDown() override { m_vines.reset(); }

    std::unique_ptr<CaveVinesBlock> m_vines;
};

TEST_F(CaveVinesBlockTest, DefaultStateHasNoBerries)
{
    const BlockState& state = m_vines->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::BERRIES()));
}

TEST_F(CaveVinesBlockTest, DefaultStateAgeIsZero)
{
    const BlockState& state = m_vines->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::AGE_0_25()), 0);
}

TEST_F(CaveVinesBlockTest, LightLevelWithBerries)
{
    const BlockState& withBerries = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    const BlockState& withoutBerries = m_vines->defaultState().with(BlockStateProperties::BERRIES(), false);

    EXPECT_EQ(m_vines->getLightLevel(withBerries), 14);
    EXPECT_EQ(m_vines->getLightLevel(withoutBerries), 0);
}

TEST_F(CaveVinesBlockTest, GetCloneItemStackReturnsGlowBerries)
{
    const BlockState& state = m_vines->defaultState();
    ItemStack stack = m_vines->getCloneItemStack(state);

    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem()->itemId(), Items::GLOW_BERRIES->itemId());
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(CaveVinesBlockTest, GetCloneItemStackWithBerries)
{
    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    ItemStack stack = m_vines->getCloneItemStack(state);

    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem()->itemId(), Items::GLOW_BERRIES->itemId());
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// CaveVinesPlantBlock 测试
// ============================================================================

class CaveVinesPlantBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();

        m_plant = std::make_unique<CaveVinesPlantBlock>(
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    }

    void TearDown() override { m_plant.reset(); }

    std::unique_ptr<CaveVinesPlantBlock> m_plant;
};

TEST_F(CaveVinesPlantBlockTest, DefaultStateHasNoBerries)
{
    const BlockState& state = m_plant->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::BERRIES()));
}

TEST_F(CaveVinesPlantBlockTest, LightLevelWithBerries)
{
    const BlockState& withBerries = m_plant->defaultState().with(BlockStateProperties::BERRIES(), true);
    const BlockState& withoutBerries = m_plant->defaultState().with(BlockStateProperties::BERRIES(), false);

    EXPECT_EQ(m_plant->getLightLevel(withBerries), 14);
    EXPECT_EQ(m_plant->getLightLevel(withoutBerries), 0);
}

TEST_F(CaveVinesPlantBlockTest, GetCloneItemStackReturnsGlowBerries)
{
    const BlockState& state = m_plant->defaultState();
    ItemStack stack = m_plant->getCloneItemStack(state);

    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem()->itemId(), Items::GLOW_BERRIES->itemId());
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(CaveVinesPlantBlockTest, GetCloneItemStackWithBerries)
{
    const BlockState& state = m_plant->defaultState().with(BlockStateProperties::BERRIES(), true);
    ItemStack stack = m_plant->getCloneItemStack(state);

    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem()->itemId(), Items::GLOW_BERRIES->itemId());
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// Glow Berries 物品注册测试
// ============================================================================

class GlowBerriesItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(GlowBerriesItemTest, GlowBerriesIsRegistered)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
}

TEST_F(GlowBerriesItemTest, GlowBerriesHasNonZeroItemId)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_GT(Items::GLOW_BERRIES->itemId(), 0u);
}

TEST_F(GlowBerriesItemTest, GlowBerriesIsFood)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_TRUE(Items::GLOW_BERRIES->isFood());
}

TEST_F(GlowBerriesItemTest, GlowBerriesMaxStackSize)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_EQ(Items::GLOW_BERRIES->maxStackSize(), 64);
}

TEST_F(GlowBerriesItemTest, GlowBerriesItemStackCreation)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    ItemStack stack(*Items::GLOW_BERRIES, 1);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getCount(), 1);
    EXPECT_EQ(stack.getItem(), Items::GLOW_BERRIES);
}
