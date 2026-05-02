#include <gtest/gtest.h>

#include "world/block/WaterLoggableHelpers.hpp"
#include "world/block/IWaterLoggable.hpp"
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/decorative/LadderBlock.hpp"
#include "world/block/blocks/decorative/LanternBlock.hpp"
#include "world/block/blocks/decorative/ChainBlock.hpp"
#include "world/block/blocks/decorative/ScaffoldingBlock.hpp"
#include "world/block/blocks/decorative/PaneBlock.hpp"
#include "world/block/blocks/ChestBlock.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/BlockPos.hpp"
#include "world/IWorld.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "entity/core/Entity.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/math/Vector3.hpp"
#include "util/math/random/Random.hpp"
#include "core/Constants.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

class WaterlogTestWorld final : public IWorld {
public:
    WaterlogTestWorld() = default;

    void ensureTickManager() {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    void setBlockDirectly(const BlockPos& pos, const BlockState* state) {
        m_blocks[packPos(pos.x, pos.y, pos.z)] = state;
    }

    void setFluidDirectly(const BlockPos& pos, const fluid::FluidState* state) {
        m_fluids[packPos(pos.x, pos.y, pos.z)] = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    bool setBlock(const BlockPos& pos, const BlockState* state) {
        return setBlock(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        // First check if there's a fluid at this position
        const auto fluidIt = m_fluids.find(packPos(x, y, z));
        if (fluidIt != m_fluids.end() && fluidIt->second != nullptr) {
            return fluidIt->second;
        }

        // Otherwise check block's fluid state
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return fluidState;
            }
        }

        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        MC_UNUSED(entity);
        return 0;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        const_cast<WaterlogTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z) {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::unordered_map<i64, const fluid::FluidState*> m_fluids;
    u64 m_seed = 0;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
};

BlockItemUseContext makePlacementContext(IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw) {
    static const ItemStack EMPTY_STACK = ItemStack::EMPTY;
    return BlockItemUseContext(
        world,
        nullptr,
        EMPTY_STACK,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f),
        pos,
        face,
        playerYaw);
}

} // namespace

// ========== WaterLoggableHelpers Tests ==========

class WaterLoggableHelpersTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(WaterLoggableHelpersTest, IsWaterFluidState_ReturnsTrueForWater) {
    WaterlogTestWorld world;
    BlockPos pos(0, 0, 0);

    // Set up water at position
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* waterState = &waterFluid->defaultState();
    world.setFluidDirectly(pos, waterState);

    const fluid::FluidState* fluidState = world.getFluidState(pos.x, pos.y, pos.z);
    EXPECT_TRUE(waterloggable::isWaterFluidState(fluidState));
}

TEST_F(WaterLoggableHelpersTest, IsWaterFluidState_ReturnsFalseForAir) {
    WaterlogTestWorld world;
    BlockPos pos(0, 0, 0);

    // No fluid set - should be air
    const fluid::FluidState* fluidState = world.getFluidState(pos.x, pos.y, pos.z);
    EXPECT_FALSE(waterloggable::isWaterFluidState(fluidState));
}

TEST_F(WaterLoggableHelpersTest, IsWaterSourceFluidState_ReturnsTrueForSource) {
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* sourceState = &waterFluid->defaultState();

    EXPECT_TRUE(waterloggable::isWaterSourceFluidState(sourceState));
}

TEST_F(WaterLoggableHelpersTest, ShouldWaterlogAt_ReturnsTrueInWater) {
    WaterlogTestWorld world;
    BlockPos pos(5, 10, 5);

    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    EXPECT_TRUE(waterloggable::shouldWaterlogAt(world, pos));
}

TEST_F(WaterLoggableHelpersTest, ShouldWaterlogAt_ReturnsFalseInAir) {
    WaterlogTestWorld world;
    BlockPos pos(5, 10, 5);

    EXPECT_FALSE(waterloggable::shouldWaterlogAt(world, pos));
}

TEST_F(WaterLoggableHelpersTest, GetWaterFluidState_ReturnsWaterWhenWaterlogged) {
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    BlockState state = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);

    ASSERT_NE(waterState, nullptr);
    EXPECT_TRUE(waterState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(WaterLoggableHelpersTest, GetWaterFluidState_ReturnsNullptrWhenNotWaterlogged) {
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    BlockState state = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);

    EXPECT_EQ(waterState, nullptr);
}

// ========== FenceBlock Waterlog Tests ==========

class FenceBlockWaterlogTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(FenceBlockWaterlogTest, GetFluidState_ReturnsWaterWhenWaterlogged) {
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    BlockState waterloggedState = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = fence.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(FenceBlockWaterlogTest, GetFluidState_ReturnsEmptyWhenNotWaterlogged) {
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    BlockState normalState = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* fluidState = fence.getFluidState(normalState);

    // Should return empty fluid state (from Block::getFluidState)
    EXPECT_TRUE(fluidState == nullptr || fluidState->isEmpty());
}

TEST_F(FenceBlockWaterlogTest, IsWaterlogged_ReturnsCorrectValue) {
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    BlockState waterloggedState = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    BlockState normalState = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), false);

    EXPECT_TRUE(fence.isWaterlogged(waterloggedState));
    EXPECT_FALSE(fence.isWaterlogged(normalState));
}

TEST_F(FenceBlockWaterlogTest, Placement_WaterloggedInWater) {
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));
    WaterlogTestWorld world;

    BlockPos pos(10, 20, 10);

    // Set water at position
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = fence.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ========== WallBlock Waterlog Tests ==========

class WallBlockWaterlogTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(WallBlockWaterlogTest, GetFluidState_ReturnsWaterWhenWaterlogged) {
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));

    BlockState waterloggedState = wall.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = wall.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(WallBlockWaterlogTest, Placement_WaterloggedInWater) {
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));
    WaterlogTestWorld world;

    BlockPos pos(5, 15, 5);

    // Set water at position
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = wall.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ========== TrapDoorBlock Waterlog Tests ==========

class TrapDoorBlockWaterlogTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(TrapDoorBlockWaterlogTest, GetFluidState_ReturnsWaterWhenWaterlogged) {
    TrapDoorBlock trapdoor(BlockProperties(Material::WOOD).hardness(3.0f), false);

    BlockState waterloggedState = trapdoor.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = trapdoor.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(TrapDoorBlockWaterlogTest, Placement_WaterloggedInWater) {
    TrapDoorBlock trapdoor(BlockProperties(Material::WOOD).hardness(3.0f), false);
    WaterlogTestWorld world;

    BlockPos pos(8, 8, 8);

    // Set water at position
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = trapdoor.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ========== SlabBlock Waterlog Tests ==========

class SlabBlockWaterlogTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(SlabBlockWaterlogTest, GetFluidState_ReturnsWaterWhenWaterlogged) {
    SlabBlock slab(BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    BlockState waterloggedState = slab.defaultState()
        .with(BlockStateProperties::SLAB_TYPE(), BlockStateProperties::SlabType::Bottom)
        .with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = slab.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(SlabBlockWaterlogTest, GetFluidState_ReturnsEmptyForDoubleSlab) {
    SlabBlock slab(BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    // Double slabs cannot be waterlogged
    BlockState doubleState = slab.defaultState()
        .with(BlockStateProperties::SLAB_TYPE(), BlockStateProperties::SlabType::Double)
        .with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = slab.getFluidState(doubleState);

    // Double slab should not hold water
    EXPECT_TRUE(fluidState == nullptr || fluidState->isEmpty());
}

// ========== StairsBlock Waterlog Tests ==========

class StairsBlockWaterlogTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(StairsBlockWaterlogTest, GetFluidState_ReturnsWaterWhenWaterlogged) {
    BlockState baseState = VanillaBlocks::STONE->defaultState();
    StairsBlock stairs(baseState, BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    BlockState waterloggedState = stairs.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = stairs.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

// ========== ScheduleWaterTick Test ==========

TEST_F(WaterLoggableHelpersTest, ScheduleWaterTick_SchedulesFluidTick) {
    WaterlogTestWorld world;
    BlockPos pos(0, 0, 0);

    // This should not throw and should schedule a fluid tick
    EXPECT_NO_THROW(waterloggable::scheduleWaterTick(world, pos));
}
