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

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/potion/GlassBottleItem.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/LayeredCauldronBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <unordered_map>

namespace mc {
namespace {

class GlassBottleTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("GlassBottleTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("GlassBottleTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
};

class GlassBottleItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        ::mc::potion::Potions::initialize();
    }

    GlassBottleTestWorld m_world;
};

TEST_F(GlassBottleItemTest, OnItemRightClick_OnWaterSource_ReturnsWaterBottle)
{
    m_world.setBlockState(0, 65, 3, &VanillaBlocks::WATER->defaultState());

    item::GlassBottleItem item(ItemProperties().maxStackSize(64));
    Player player(1, "BottleTester", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.5f);
    player.setRotation(0.0f, 0.0f);
    player.getHeldItem(Hand::MainHand) = ItemStack(&item, 1);

    const ItemActionResult result = item.onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isConsume());
    ASSERT_NE(result.getResult().getItem(), nullptr);
    EXPECT_EQ(result.getResult().getItem(), Items::POTION);
    EXPECT_TRUE(::mc::potion::PotionUtils::isWaterBottle(result.getResult()));
}

TEST_F(GlassBottleItemTest, OnItemRightClick_OnWaterCauldron_ReturnsWaterBottle)
{
    // 水炼药锅（LayeredCauldronBlock，水位2）可以用玻璃瓶取水
    const BlockState waterCauldron =
        VanillaBlocks::WATER_CAULDRON->defaultState().with(BlockStateProperties::LEVEL_1_3(), 2);
    m_world.setBlockState(0, 65, 3, &waterCauldron);

    item::GlassBottleItem item(ItemProperties().maxStackSize(64));
    Player player(1, "BottleTester", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.5f);
    player.setRotation(0.0f, 0.0f);
    player.getHeldItem(Hand::MainHand) = ItemStack(&item, 1);

    const ItemActionResult result = item.onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isConsume());
    ASSERT_NE(result.getResult().getItem(), nullptr);
    EXPECT_EQ(result.getResult().getItem(), Items::POTION);
    EXPECT_TRUE(::mc::potion::PotionUtils::isWaterBottle(result.getResult()));
}

TEST_F(GlassBottleItemTest, OnItemRightClick_WhenNothingMatches_PassesThroughHeldItem)
{
    item::GlassBottleItem item(ItemProperties().maxStackSize(64));
    Player player(1, "BottleTester", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.5f);
    player.setRotation(0.0f, 0.0f);
    player.getHeldItem(Hand::MainHand) = ItemStack(&item, 1);

    const ItemActionResult result = item.onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isPass());
    EXPECT_EQ(result.getResult().getItem(), &item);
}

} // namespace
} // namespace mc
