#include <gtest/gtest.h>

#include "world/block/blocks/decorative/PaneBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/BlockPos.hpp"
#include "world/IWorld.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "entity/core/Entity.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/math/Vector3.hpp"
#include "core/Constants.hpp"

#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

class PaneTestWorld final : public IWorld {
public:
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
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
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
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override {
        return {};
    }
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

    void scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay,
                           world::tick::TickPriority priority) override {
        MC_UNUSED(fluid);
        MC_UNUSED(delay);
        MC_UNUSED(priority);
        ++m_fluidTickCount;
        m_lastFluidTickPos = pos;
    }

    [[nodiscard]] i32 fluidTickCount() const { return m_fluidTickCount; }
    [[nodiscard]] const BlockPos& lastFluidTickPos() const { return m_lastFluidTickPos; }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z) {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    i32 m_fluidTickCount = 0;
    BlockPos m_lastFluidTickPos{0, 0, 0};
    u64 m_seed = 0;
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

class TestSolidBlock final : public Block {
public:
    explicit TestSolidBlock(const BlockProperties& properties)
        : Block(properties) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }
};

} // namespace

TEST(PaneBlockTest, Placement_ConnectsToSolidPaneAndWallAndWaterlogs) {
    PaneBlock pane(BlockProperties(Material::GLASS).noCollision().notSolid());
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    PaneTestWorld world;
    const BlockPos pos(8, 64, 8);

    world.setBlock(pos.north(), &pane.defaultState());
    world.setBlock(pos.east(), &wall.defaultState());
    world.setBlock(pos.south(), &solid.defaultState());
    world.setBlock(pos, &VanillaBlocks::WATER->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = pane.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::NORTH()));
    EXPECT_TRUE(state.get(BlockStateProperties::EAST()));
    EXPECT_TRUE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::WEST()));
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));

    const fluid::FluidState* fluidState = pane.getFluidState(state);
    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST(PaneBlockTest, Shape_CombinesCenterAndConnectedSides) {
    PaneBlock pane(BlockProperties(Material::GLASS).noCollision().notSolid());

    const BlockState state = pane.defaultState()
        .with(BlockStateProperties::NORTH(), true)
        .with(BlockStateProperties::EAST(), true)
        .with(BlockStateProperties::SOUTH(), false)
        .with(BlockStateProperties::WEST(), true)
        .with(BlockStateProperties::WATERLOGGED(), false);

    const CollisionShape& shape = pane.getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_EQ(shape.boxCount(), 4u);
}

TEST(PaneBlockTest, UpdatePostPlacement_RecomputesFaceAndSchedulesWaterTick) {
    PaneBlock pane(BlockProperties(Material::GLASS).noCollision().notSolid());
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    PaneTestWorld world;
    const BlockPos pos(12, 70, 3);

    const BlockState state = pane.defaultState()
        .with(BlockStateProperties::NORTH(), false)
        .with(BlockStateProperties::EAST(), false)
        .with(BlockStateProperties::SOUTH(), false)
        .with(BlockStateProperties::WEST(), false)
        .with(BlockStateProperties::WATERLOGGED(), true);

    const BlockState updated = pane.updatePostPlacement(state, Direction::North, solid.defaultState(), world, pos, pos.north());

    EXPECT_TRUE(updated.get(BlockStateProperties::NORTH()));
    EXPECT_EQ(world.fluidTickCount(), 1);
    EXPECT_EQ(world.lastFluidTickPos(), pos);
}
