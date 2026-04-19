#include <gtest/gtest.h>

#include "common/item/items/potion/GlassBottleItem.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/item/Items.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/CauldronBlock.hpp"
#include "common/world/fluid/Fluid.hpp"

#include <unordered_map>

namespace mc {
namespace {

class GlassBottleTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 0; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= 0 && y < 256; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
};

class GlassBottleItemTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        Items::initialize();
        ::mc::potion::Potions::initialize();
    }

    GlassBottleTestWorld m_world;
};

TEST_F(GlassBottleItemTest, OnItemRightClick_OnWaterSource_ReturnsWaterBottle) {
    m_world.setBlock(0, 65, 3, &VanillaBlocks::WATER->defaultState());

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

TEST_F(GlassBottleItemTest, OnItemRightClick_OnFilledCauldron_ReturnsWaterBottle) {
    const BlockState filledCauldron =
        VanillaBlocks::CAULDRON->defaultState().with(BlockStateProperties::LEVEL_0_3(), 2);
    m_world.setBlock(0, 65, 3, &filledCauldron);

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

TEST_F(GlassBottleItemTest, OnItemRightClick_WhenNothingMatches_PassesThroughHeldItem) {
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