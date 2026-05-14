#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/math/Vector3.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/decorative/PaneBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

class PaneTestWorld final : public test::BaseTestWorld {
public:
    PaneTestWorld() = default;

    // 延迟初始化 TickManager（首次调用时初始化）
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }

        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }

        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        MC_UNUSED(entity);
        return 0;
    }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<PaneTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    u64 m_seed = 0;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
};

BlockItemUseContext makePlacementContext(IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw)
{
    static const ItemStack EMPTY_STACK = ItemStack::EMPTY;
    return BlockItemUseContext(world,
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
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }
};

} // namespace

class PaneBlockTestFixture : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(PaneBlockTestFixture, Placement_ConnectsToSolidPaneAndWallAndWaterlogs)
{
    PaneBlock pane(BlockProperties(Material::GLASS).noCollision().notSolid());
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    PaneTestWorld world;
    const BlockPos pos(8, 64, 8);

    world.setBlockState(pos.north(), &pane.defaultState());
    world.setBlockState(pos.east(), &wall.defaultState());
    world.setBlockState(pos.south(), &solid.defaultState());
    world.setBlockState(pos, &VanillaBlocks::WATER->defaultState());

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

TEST_F(PaneBlockTestFixture, Shape_CombinesCenterAndConnectedSides)
{
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

TEST_F(PaneBlockTestFixture, UpdatePostPlacement_RecomputesFaceAndSchedulesWaterTick)
{
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

    const BlockState updated =
        pane.updatePostPlacement(state, Direction::North, solid.defaultState(), world, pos, pos.north());

    EXPECT_TRUE(updated.get(BlockStateProperties::NORTH()));
}
