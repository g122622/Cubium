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
#include "util/Direction.hpp"
#include "util/math/Vector3.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

class WallTestWorld final : public test::BaseTestWorld {
public:
    WallTestWorld() = default;

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

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<WallTestWorld*>(this)->ensureTickManager();
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

// ============================================================================
// Test Fixture
// ============================================================================

class WallBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ============================================================================
// WallBlock Connection Tests
// ============================================================================

TEST_F(WallBlockTest, Placement_NoConnections_HasUpAndNoSideHeights)
{
    // 无连接时：UP=true，四方向高度为 None
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = wall.getStateForPlacement(context);

    // 无连接时墙柱应升起
    EXPECT_TRUE(state.get(BlockStateProperties::UP()));
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::None);
}

TEST_F(WallBlockTest, Placement_ConnectsToSolidBlock)
{
    // 固体方块连接：高度应为 Tall
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    world.setBlockState(pos.north(), &solid.defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = wall.getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Tall);
}

TEST_F(WallBlockTest, Placement_ConnectsToOtherWall)
{
    // 墙连接其他墙时：高度应为 Tall
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    // 使用原版墙方块进行测试
    if (VanillaBlocks::COBBLESTONE_WALL) {
        world.setBlockState(pos.east(), &VanillaBlocks::COBBLESTONE_WALL->defaultState());

        BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
        const BlockState state = wall.getStateForPlacement(context);

        EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Tall);
    }
}

TEST_F(WallBlockTest, Placement_DoesNotConnectToLeaves)
{
    // 参考: Block::isExceptionForConnection - 树叶不应连接
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    if (VanillaBlocks::OAK_LEAVES) {
        world.setBlockState(pos.south(), &VanillaBlocks::OAK_LEAVES->defaultState());

        BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
        const BlockState state = wall.getStateForPlacement(context);

        EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::None);
    }
}

TEST_F(WallBlockTest, Placement_DoesNotConnectToAir)
{
    // 空气不应连接
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = wall.getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::None);
}

TEST_F(WallBlockTest, UpdatePostPlacement_UpdatesHeightsOnNeighborChange)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(12, 70, 3);

    // _calculateState 从世界中查询邻居，所以必须设置邻居方块
    world.setBlockState(pos.north(), &solid.defaultState());

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    const BlockState updated =
        wall.updatePostPlacement(state, Direction::North, solid.defaultState(), world, pos, pos.north());

    EXPECT_EQ(updated.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Tall);
}

TEST_F(WallBlockTest, Waterlogged_ReturnsCorrectFluidState)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    fluid::FluidRegistry::instance().initialize();

    const BlockState waterloggedState = wall.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = wall.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(WallBlockTest, IsWaterlogged)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState waterlogged = wall.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const BlockState notWaterlogged = wall.defaultState().with(BlockStateProperties::WATERLOGGED(), false);

    EXPECT_TRUE(wall.isWaterlogged(waterlogged));
    EXPECT_FALSE(wall.isWaterlogged(notWaterlogged));
}

// ============================================================================
// WallBlock Shape Tests
// ============================================================================

TEST_F(WallBlockTest, Shape_PillarOnlyWhenNoConnections)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    const CollisionShape& shape = wall.getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(WallBlockTest, Shape_WithNorthTallConnection)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Tall)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    const CollisionShape& shape = wall.getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    // UP + 北面Tall连接 = 柱子 + 北面面板 + 顶部突出 = 3个box
    EXPECT_GE(shape.boxCount(), 2u);
}

TEST_F(WallBlockTest, Shape_WithLowConnection)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Low)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    const CollisionShape& shape = wall.getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

// ============================================================================
// WallBlock Rotate and Mirror Tests
// ============================================================================

TEST_F(WallBlockTest, Rotate_Clockwise90)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Tall)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    // 顺时针90度旋转: North -> East, East -> South, South -> West, West -> North
    const BlockState& rotated = wall.rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::None);
}

TEST_F(WallBlockTest, Rotate_Clockwise180)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Tall)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::Low)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    const BlockState& rotated = wall.rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(rotated.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::Low);
}

TEST_F(WallBlockTest, Mirror_LeftRight)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Tall)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::Low)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    // 左右镜像: East <-> West
    const BlockState& mirrored = wall.mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::Low);
}

TEST_F(WallBlockTest, Mirror_FrontBack)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Tall)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::Low)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    // 前后镜像: North <-> South
    const BlockState& mirrored = wall.mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::None);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Low);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(mirrored.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::None);
}

// ============================================================================
// WallBlock Static Method Tests
// ============================================================================

TEST_F(WallBlockTest, IsWall_WithWallBlockTag)
{
    // 参考: WallBlock::isWall 使用 BlockTags::WALLS 标签
    if (VanillaBlocks::COBBLESTONE_WALL) {
        EXPECT_TRUE(WallBlock::isWall(VanillaBlocks::COBBLESTONE_WALL->defaultState()));
    }
    if (VanillaBlocks::STONE_BRICK_WALL) {
        EXPECT_TRUE(WallBlock::isWall(VanillaBlocks::STONE_BRICK_WALL->defaultState()));
    }
}

TEST_F(WallBlockTest, IsWall_NonWallBlockReturnsFalse)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));

    // 非墙方块不应被识别为墙
    EXPECT_FALSE(WallBlock::isWall(solid.defaultState()));
}

// ============================================================================
// WallBlock Redstone Tests
// ============================================================================

TEST_F(WallBlockTest, CanConnectRedstone)
{
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    const BlockState state = wall.defaultState();

    // 墙方块总是可以连接红石
    EXPECT_TRUE(wall.canConnectRedstone(state, Direction::North));
    EXPECT_TRUE(wall.canConnectRedstone(state, Direction::East));
    EXPECT_TRUE(wall.canConnectRedstone(state, Direction::South));
    EXPECT_TRUE(wall.canConnectRedstone(state, Direction::West));
}
