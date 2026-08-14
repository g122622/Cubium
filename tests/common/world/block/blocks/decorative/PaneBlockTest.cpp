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
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/math/Vector3.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/decorative/PaneBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/fluid/Fluids.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

class PaneTestWorld final : public mc::test::BaseTestWorld {
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

        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
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
        playerYaw,
        0.0f);
}

class TestSolidBlock final : public Block {
public:
    explicit TestSolidBlock(const BlockProperties& properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }
};

} // namespace

class PaneBlockTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
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

// ============================================================================
// PaneBlock isExceptionForConnection Tests
// 参考: PaneBlock::shouldConnectTo 使用 Block::isExceptionForConnection 排除连接例外方块
// ============================================================================

TEST_F(PaneBlockTestFixture, PaneDoesNotConnectToLeaves)
{
    // 树叶是连接例外，玻璃板不应连接到树叶
    if (!VanillaBlocks::OAK_LEAVES) {
        GTEST_SKIP() << "OAK_LEAVES not registered";
    }

    PaneBlock pane(BlockProperties(Material::GLASS).noCollision().notSolid());
    PaneTestWorld world;
    const BlockPos pos(15, 64, 8);

    world.setBlockState(pos.north(), &VanillaBlocks::OAK_LEAVES->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = pane.getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));
}

TEST_F(PaneBlockTestFixture, PaneConnectsToWall)
{
    // 玻璃板应连接到墙方块
    if (!VanillaBlocks::COBBLESTONE_WALL) {
        GTEST_SKIP() << "COBBLESTONE_WALL not registered";
    }

    PaneBlock pane(BlockProperties(Material::GLASS).noCollision().notSolid());
    PaneTestWorld world;
    const BlockPos pos(16, 64, 8);

    world.setBlockState(pos.east(), &VanillaBlocks::COBBLESTONE_WALL->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = pane.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::EAST()));
}

// ============================================================================
// PaneBlock skipRendering Tests
// 参考: net.minecraft.world.level.block.IronBarsBlock#skipRendering
// ============================================================================

TEST_F(PaneBlockTestFixture, SkipRendering_SameBlock_VerticalDirection)
{
    // 同类方块垂直相邻时，始终跳过面渲染
    PaneBlock pane(BlockProperties(Material::IRON).notSolid());

    const BlockState selfState = pane.defaultState()
                                     .with(BlockStateProperties::NORTH(), true)
                                     .with(BlockStateProperties::EAST(), false)
                                     .with(BlockStateProperties::SOUTH(), false)
                                     .with(BlockStateProperties::WEST(), false)
                                     .with(BlockStateProperties::WATERLOGGED(), false);

    const BlockState neighborState = pane.defaultState()
                                         .with(BlockStateProperties::NORTH(), false)
                                         .with(BlockStateProperties::EAST(), true)
                                         .with(BlockStateProperties::SOUTH(), false)
                                         .with(BlockStateProperties::WEST(), false)
                                         .with(BlockStateProperties::WATERLOGGED(), false);

    // 垂直方向（上/下）— 同类方块始终跳过
    EXPECT_TRUE(pane.skipRendering(selfState, neighborState, Direction::Up));
    EXPECT_TRUE(pane.skipRendering(selfState, neighborState, Direction::Down));
}

TEST_F(PaneBlockTestFixture, SkipRendering_SameBlock_HorizontalBothConnected)
{
    // 同类方块水平相邻，双方都连接时跳过面渲染
    PaneBlock pane(BlockProperties(Material::IRON).notSolid());

    // 自身朝北连接，邻居朝南连接
    const BlockState selfState = pane.defaultState()
                                     .with(BlockStateProperties::NORTH(), true)
                                     .with(BlockStateProperties::EAST(), false)
                                     .with(BlockStateProperties::SOUTH(), false)
                                     .with(BlockStateProperties::WEST(), false)
                                     .with(BlockStateProperties::WATERLOGGED(), false);

    const BlockState neighborState = pane.defaultState()
                                         .with(BlockStateProperties::NORTH(), false)
                                         .with(BlockStateProperties::EAST(), false)
                                         .with(BlockStateProperties::SOUTH(), true)
                                         .with(BlockStateProperties::WEST(), false)
                                         .with(BlockStateProperties::WATERLOGGED(), false);

    // 北面 — 自身连接，邻居反向也连接 → 跳过
    EXPECT_TRUE(pane.skipRendering(selfState, neighborState, Direction::North));
}

TEST_F(PaneBlockTestFixture, SkipRendering_SameBlock_HorizontalOneSidedNotSkipped)
{
    // 同类方块水平相邻，但仅单方连接时不跳过面渲染
    PaneBlock pane(BlockProperties(Material::IRON).notSolid());

    // 自身朝北不连接，邻居朝南连接
    const BlockState selfState = pane.defaultState()
                                     .with(BlockStateProperties::NORTH(), false)
                                     .with(BlockStateProperties::EAST(), false)
                                     .with(BlockStateProperties::SOUTH(), false)
                                     .with(BlockStateProperties::WEST(), false)
                                     .with(BlockStateProperties::WATERLOGGED(), false);

    const BlockState neighborState = pane.defaultState()
                                         .with(BlockStateProperties::NORTH(), false)
                                         .with(BlockStateProperties::EAST(), false)
                                         .with(BlockStateProperties::SOUTH(), true)
                                         .with(BlockStateProperties::WEST(), false)
                                         .with(BlockStateProperties::WATERLOGGED(), false);

    // 北面 — 自身不连接 → 不跳过
    EXPECT_FALSE(pane.skipRendering(selfState, neighborState, Direction::North));
}

TEST_F(PaneBlockTestFixture, SkipRendering_BarsTag_VerticalDirection)
{
    // BARS 标签方块（铁栏杆↔铜栏杆）垂直相邻时不跳过面渲染
    // 因为铁栏杆/铜栏杆只有 NSEW 属性，没有 UP/DOWN 属性，
    // 所以垂直方向的 BARS 标签检查不会满足 hasProperty 条件
    if (!VanillaBlocks::IRON_BARS || !VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "IRON_BARS or COPPER_BARS not registered";
    }

    const BlockState ironBarsState = VanillaBlocks::IRON_BARS->defaultState();
    const BlockState copperBarsState = VanillaBlocks::COPPER_BARS->defaultState();

    // 垂直方向 — 不同 BARS 方块之间不跳过（因为没有反向属性）
    EXPECT_FALSE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, copperBarsState, Direction::Up));
    EXPECT_FALSE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, copperBarsState, Direction::Down));
}

TEST_F(PaneBlockTestFixture, SkipRendering_SameBlock_Vertical_VanillaBars)
{
    // 同类方块（同一 Block 实例）垂直相邻时，始终跳过面渲染
    if (!VanillaBlocks::IRON_BARS) {
        GTEST_SKIP() << "IRON_BARS not registered";
    }

    const BlockState ironBarsState = VanillaBlocks::IRON_BARS->defaultState();

    // 同类方块垂直相邻 — 始终跳过
    EXPECT_TRUE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, ironBarsState, Direction::Up));
    EXPECT_TRUE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, ironBarsState, Direction::Down));
}

TEST_F(PaneBlockTestFixture, SkipRendering_BarsTag_HorizontalBothConnected)
{
    // BARS 标签方块水平相邻且双方都连接时跳过面渲染
    if (!VanillaBlocks::IRON_BARS || !VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "IRON_BARS or COPPER_BARS not registered";
    }

    // 铁栏杆朝北连接
    const BlockState ironBarsState = VanillaBlocks::IRON_BARS->defaultState()
                                         .with(BlockStateProperties::NORTH(), true)
                                         .with(BlockStateProperties::EAST(), false)
                                         .with(BlockStateProperties::SOUTH(), false)
                                         .with(BlockStateProperties::WEST(), false)
                                         .with(BlockStateProperties::WATERLOGGED(), false);

    // 铜栏杆朝南连接
    const BlockState copperBarsState = VanillaBlocks::COPPER_BARS->defaultState()
                                           .with(BlockStateProperties::NORTH(), false)
                                           .with(BlockStateProperties::EAST(), false)
                                           .with(BlockStateProperties::SOUTH(), true)
                                           .with(BlockStateProperties::WEST(), false)
                                           .with(BlockStateProperties::WATERLOGGED(), false);

    // 北面 — 铁栏杆连接，铜栏杆反向也连接 → 跳过
    EXPECT_TRUE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, copperBarsState, Direction::North));
}

TEST_F(PaneBlockTestFixture, SkipRendering_BarsTag_HorizontalOneSidedNotSkipped)
{
    // BARS 标签方块水平相邻，但仅单方连接时不跳过
    if (!VanillaBlocks::IRON_BARS || !VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "IRON_BARS or COPPER_BARS not registered";
    }

    // 铁栏杆朝北不连接
    const BlockState ironBarsState = VanillaBlocks::IRON_BARS->defaultState()
                                         .with(BlockStateProperties::NORTH(), false)
                                         .with(BlockStateProperties::EAST(), false)
                                         .with(BlockStateProperties::SOUTH(), false)
                                         .with(BlockStateProperties::WEST(), false)
                                         .with(BlockStateProperties::WATERLOGGED(), false);

    // 铜栏杆朝南连接
    const BlockState copperBarsState = VanillaBlocks::COPPER_BARS->defaultState()
                                           .with(BlockStateProperties::NORTH(), false)
                                           .with(BlockStateProperties::EAST(), false)
                                           .with(BlockStateProperties::SOUTH(), true)
                                           .with(BlockStateProperties::WEST(), false)
                                           .with(BlockStateProperties::WATERLOGGED(), false);

    // 北面 — 铁栏杆自身不连接 → 不跳过
    EXPECT_FALSE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, copperBarsState, Direction::North));
}

TEST_F(PaneBlockTestFixture, SkipRendering_NonBarsBlock_NotSkipped)
{
    // 非 BARS 标签方块与铁栏杆相邻时不跳过
    if (!VanillaBlocks::IRON_BARS || !VanillaBlocks::STONE) {
        GTEST_SKIP() << "IRON_BARS or STONE not registered";
    }

    const BlockState ironBarsState = VanillaBlocks::IRON_BARS->defaultState();
    const BlockState stoneState = VanillaBlocks::STONE->defaultState();

    // 石头不是 BARS 标签方块 → 不跳过
    EXPECT_FALSE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, stoneState, Direction::North));
    EXPECT_FALSE(VanillaBlocks::IRON_BARS->skipRendering(ironBarsState, stoneState, Direction::Up));
}
