#include <gtest/gtest.h>

#include "entity/entities/player/Player.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/Items.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/fluid/Fluid.hpp"

#include <unordered_map>

using namespace mc;

namespace {

class TestBlockReader final : public IBlockReader {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(key(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override {
        (void)x;
        (void)z;
        return y >= 0 && y < 256;
    }

    // IWorld 接口实现 - 同时作为测试辅助方法
    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[key(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }

private:
    static i64 key(i32 x, i32 y, i32 z) {
        return (static_cast<i64>(x) << 40) ^ (static_cast<i64>(y) << 20) ^ static_cast<i64>(z & 0xFFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
};

}

class BlockItemTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

TEST_F(BlockItemTest, RegistryMapsStoneBlockItem) {
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STONE->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(&item->block(), VanillaBlocks::STONE);
}

TEST_F(BlockItemTest, CreativeInventoryGetsRegisteredBlockItems) {
    Player player(1, "test");
    player.setCreativeModeInventory();

    const ItemStack selected = player.inventory().getSelectedStack();
    EXPECT_FALSE(selected.isEmpty());
    ASSERT_NE(selected.getItem(), nullptr);
    EXPECT_TRUE(BlockItemRegistry::instance().isBlockItem(selected.getItem()));
}

TEST_F(BlockItemTest, PlacementContextUsesAdjacentPosForSolidBlock) {
    TestBlockReader world;
    world.setBlock(0, 64, 0, &VanillaBlocks::STONE->defaultState());

    const BlockItem* stoneItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STONE->blockId());
    ASSERT_NE(stoneItem, nullptr);

    ItemStack stack(*stoneItem, 64);
    BlockItemUseContext context(world,
                                nullptr,
                                stack,
                                Vector3(0.5f, 64.99f, 0.5f),
                                BlockPos(0, 64, 0),
                                Direction::Up,
                                0.0f);

    EXPECT_FALSE(context.replacingClickedBlock());
    EXPECT_EQ(context.placementPos(), BlockPos(0, 65, 0));
    EXPECT_TRUE(context.canPlace());
}