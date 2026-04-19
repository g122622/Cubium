#include <gtest/gtest.h>

#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/DoorBlock.hpp"
#include "common/world/block/blocks/FenceGateBlock.hpp"
#include "common/world/block/blocks/HopperBlock.hpp"
#include "common/world/block/blocks/building/TrapDoorBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"

#include <map>

using namespace mc;
using namespace mc::blocks;

namespace {

class ConstantPowerBlock final : public Block {
public:
    ConstantPowerBlock()
        : Block(BlockProperties(Material::STONE)) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getStrongPower(const BlockState& state, IWorld& world,
                                     const BlockPos& pos, Direction side) const override {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(side);
        return 15;
    }
};

class RedstoneBlockTestWorld final : public IWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it == m_blocks.end() ? nullptr : &it->second;
    }

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        return setBlockState(x, y, z, state, 0);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override {
        MC_UNUSED(flags);
        const BlockPos pos(x, y, z);
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = *state;
        }
        ++m_setBlockCalls;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
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

    void setBlockAt(const BlockPos& pos, const BlockState& state) {
        m_blocks[pos] = state;
    }

    void clearBlockAt(const BlockPos& pos) {
        m_blocks.erase(pos);
    }

    [[nodiscard]] i32 setBlockCalls() const {
        return m_setBlockCalls;
    }

private:
    std::map<BlockPos, BlockState> m_blocks;
    i32 m_setBlockCalls = 0;
};

BlockItemUseContext makePlacementContext(const IWorld& world, const BlockPos& pos, f32 playerYaw = 180.0f) {
    static const ItemStack EMPTY_STACK = ItemStack::EMPTY;
    return BlockItemUseContext(
        world,
        nullptr,
        EMPTY_STACK,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f),
        pos,
        Direction::Up,
        playerYaw);
}

ConstantPowerBlock& powerBlock() {
    static ConstantPowerBlock block;
    return block;
}

void setPowerSource(RedstoneBlockTestWorld& world, const BlockPos& pos) {
    world.setBlockAt(pos, powerBlock().defaultState());
}

} // namespace

class RedstoneBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(RedstoneBlockTest, DoorBlockPlacement_UsesRedstonePower) {
    RedstoneBlockTestWorld world;
    DoorBlock door(BlockProperties(Material::WOOD));
    const BlockPos pos(10, 64, 10);

    world.setBlockAt(pos.up(), VanillaBlocks::AIR->defaultState());
    setPowerSource(world, pos.up().north());

    const BlockState state = door.getStateForPlacement(makePlacementContext(world, pos));

    EXPECT_TRUE(state.get(BlockStateProperties::POWERED()));
    EXPECT_TRUE(state.get(BlockStateProperties::OPEN()));
}

TEST_F(RedstoneBlockTest, DoorBlockNeighborChanged_UpdatesFromRedstonePower) {
    RedstoneBlockTestWorld world;
    DoorBlock door(BlockProperties(Material::WOOD));
    const BlockPos pos(10, 64, 10);

    BlockState state = door.defaultState()
        .with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::OPEN(), false)
        .with(BlockStateProperties::POWERED(), false);
    BlockState upperState = state.with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockAt(pos, state);
    world.setBlockAt(pos.up(), upperState);
    setPowerSource(world, pos.up().north());

    door.neighborChanged(world, pos, powerBlock(), pos.up().north(), false);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(updated->get(BlockStateProperties::POWERED()));
    EXPECT_TRUE(updated->get(BlockStateProperties::OPEN()));

    const BlockState* upperUpdated = world.getBlockState(pos.up());
    ASSERT_NE(upperUpdated, nullptr);
    EXPECT_EQ(upperUpdated->get(BlockStateProperties::HALF()), BlockStateProperties::DoubleBlockHalf::Upper);
    EXPECT_TRUE(upperUpdated->get(BlockStateProperties::POWERED()));
    EXPECT_TRUE(upperUpdated->get(BlockStateProperties::OPEN()));
}

TEST_F(RedstoneBlockTest, FenceGateBlockPlacement_UsesRedstonePower) {
    RedstoneBlockTestWorld world;
    FenceGateBlock fenceGate(BlockProperties(Material::WOOD));
    const BlockPos pos(12, 64, 12);

    setPowerSource(world, pos.north());

    const BlockState state = fenceGate.getStateForPlacement(makePlacementContext(world, pos));

    EXPECT_TRUE(state.get(BlockStateProperties::POWERED()));
    EXPECT_TRUE(state.get(BlockStateProperties::OPEN()));
}

TEST_F(RedstoneBlockTest, FenceGateBlockNeighborChanged_UpdatesFromRedstonePower) {
    RedstoneBlockTestWorld world;
    FenceGateBlock fenceGate(BlockProperties(Material::WOOD));
    const BlockPos pos(12, 64, 12);

    world.setBlockAt(pos, fenceGate.defaultState());
    setPowerSource(world, pos.north());

    fenceGate.neighborChanged(world, pos, powerBlock(), pos.north(), false);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(updated->get(BlockStateProperties::POWERED()));
    EXPECT_TRUE(updated->get(BlockStateProperties::OPEN()));
}

TEST_F(RedstoneBlockTest, FenceGateBlockPlacement_ActualWallsSetInWall) {
    RedstoneBlockTestWorld world;
    FenceGateBlock fenceGate(BlockProperties(Material::WOOD));
    const BlockPos pos(14, 64, 14);

    world.setBlockAt(pos.west(), VanillaBlocks::COBBLESTONE_WALL->defaultState());
    world.setBlockAt(pos.east(), VanillaBlocks::COBBLESTONE_WALL->defaultState());

    const BlockState state = fenceGate.getStateForPlacement(makePlacementContext(world, pos));

    EXPECT_TRUE(state.get(BlockStateProperties::IN_WALL()));
}

TEST_F(RedstoneBlockTest, FenceGateBlockPlacement_GenericSolidsDoNotSetInWall) {
    RedstoneBlockTestWorld world;
    FenceGateBlock fenceGate(BlockProperties(Material::WOOD));
    const BlockPos pos(14, 64, 14);

    world.setBlockAt(pos.west(), VanillaBlocks::STONE->defaultState());
    world.setBlockAt(pos.east(), VanillaBlocks::STONE->defaultState());

    const BlockState state = fenceGate.getStateForPlacement(makePlacementContext(world, pos));

    EXPECT_FALSE(state.get(BlockStateProperties::IN_WALL()));
}

TEST_F(RedstoneBlockTest, TrapDoorBlockPlacement_UsesRedstonePower) {
    RedstoneBlockTestWorld world;
    TrapDoorBlock trapDoor(BlockProperties(Material::WOOD));
    const BlockPos pos(16, 64, 16);

    setPowerSource(world, pos.north());

    const BlockState state = trapDoor.getStateForPlacement(makePlacementContext(world, pos));

    EXPECT_TRUE(state.get(BlockStateProperties::POWERED()));
    EXPECT_TRUE(state.get(BlockStateProperties::OPEN()));
}

TEST_F(RedstoneBlockTest, TrapDoorBlockNeighborChanged_UpdatesFromRedstonePower) {
    RedstoneBlockTestWorld world;
    TrapDoorBlock trapDoor(BlockProperties(Material::WOOD));
    const BlockPos pos(16, 64, 16);

    world.setBlockAt(pos, trapDoor.defaultState());
    setPowerSource(world, pos.north());

    trapDoor.neighborChanged(world, pos, powerBlock(), pos.north(), false);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(updated->get(BlockStateProperties::POWERED()));
    EXPECT_TRUE(updated->get(BlockStateProperties::OPEN()));
}

TEST_F(RedstoneBlockTest, HopperBlockOnBlockAdded_PoweredDisablesHopper) {
    RedstoneBlockTestWorld world;
    HopperBlock hopper(BlockProperties(Material::STONE));
    const BlockPos pos(20, 64, 20);

    world.setBlockAt(pos, hopper.defaultState());
    setPowerSource(world, pos.north());

    hopper.onBlockAdded(world, pos, hopper.defaultState());

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_FALSE(updated->get(BlockStateProperties::ENABLED()));
    EXPECT_EQ(world.setBlockCalls(), 1);
}

TEST_F(RedstoneBlockTest, HopperBlockNeighborChanged_UnpoweredEnablesHopper) {
    RedstoneBlockTestWorld world;
    HopperBlock hopper(BlockProperties(Material::STONE));
    const BlockPos pos(20, 64, 20);

    BlockState state = hopper.defaultState().with(BlockStateProperties::ENABLED(), false);
    world.setBlockAt(pos, state);

    hopper.neighborChanged(world, pos, powerBlock(), pos.north(), false);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(updated->get(BlockStateProperties::ENABLED()));
    EXPECT_EQ(world.setBlockCalls(), 1);
}
