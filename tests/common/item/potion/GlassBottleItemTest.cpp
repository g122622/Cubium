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
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/CauldronBlock.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <unordered_map>

namespace mc {
namespace {

class GlassBottleTestWorld final : public test::BaseTestWorld {
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
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
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
    Player player(1, "BottleTester");
    player.setPosition(0.5f, 64.0f, 0.5f);
    player.setRotation(0.0f, 0.0f);
    player.getHeldItem(Hand::MainHand) = ItemStack(&item, 1);

    const ItemActionResult result = item.onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isConsume());
    ASSERT_NE(result.getResult().getItem(), nullptr);
    EXPECT_EQ(result.getResult().getItem(), Items::POTION);
    EXPECT_TRUE(::mc::potion::PotionUtils::isWaterBottle(result.getResult()));
}

TEST_F(GlassBottleItemTest, OnItemRightClick_OnFilledCauldron_ReturnsWaterBottle)
{
    const BlockState filledCauldron =
        VanillaBlocks::CAULDRON->defaultState().with(BlockStateProperties::LEVEL_0_3(), 2);
    m_world.setBlockState(0, 65, 3, &filledCauldron);

    item::GlassBottleItem item(ItemProperties().maxStackSize(64));
    Player player(1, "BottleTester");
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
    Player player(1, "BottleTester");
    player.setPosition(0.5f, 64.0f, 0.5f);
    player.setRotation(0.0f, 0.0f);
    player.getHeldItem(Hand::MainHand) = ItemStack(&item, 1);

    const ItemActionResult result = item.onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isPass());
    EXPECT_EQ(result.getResult().getItem(), &item);
}

} // namespace
} // namespace mc
