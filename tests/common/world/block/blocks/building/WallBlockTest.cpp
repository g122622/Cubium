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
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/Shapes.hpp"
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
#include "world/fluid/Fluids.hpp"
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
    // 固体方块连接：上方无方块时高度为 Low，上方有完整方块时为 Tall
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    world.setBlockState(pos.north(), &solid.defaultState());
    // 注意：上方无方块，因此碰撞形状的 Down 面投影为空
    // isCovered 空形状不覆盖 TEST_SHAPES_WALL，所以高度为 Low
    // 参考: MC原版 WallBlock#makeWallState 逻辑
    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = wall.getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Low);
}

TEST_F(WallBlockTest, Placement_ConnectsToOtherWall)
{
    // 墙连接其他墙时：上方无方块覆盖，高度为 Low
    // 参考: MC原版 WallBlock#makeWallState，上方无方块时 aboveFaceShape 为空，
    // isCovered(testShapesWall, emptyShape) 返回 false，所以高度为 Low
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    // 使用原版墙方块进行测试
    if (VanillaBlocks::COBBLESTONE_WALL) {
        world.setBlockState(pos.east(), &VanillaBlocks::COBBLESTONE_WALL->defaultState());

        BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
        const BlockState state = wall.getStateForPlacement(context);

        // 上方无方块，墙连接高度为 Low
        EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Low);
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

    // 上方无方块覆盖，固体邻居连接高度为 Low
    // 参考: MC原版 WallBlock#makeWallState，上方无方块时 aboveFaceShape 为空，
    // isCovered 返回 false，所以高度为 Low
    EXPECT_EQ(updated.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Low);
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

// ============================================================================
// WallBlock _shouldRaisePost Tests
// 参考: net.minecraft.block.WallBlock#shouldRaisePost
// ============================================================================

TEST_F(WallBlockTest, ShouldRaisePost_NoConnections)
{
    // 无连接时应该升起墙柱
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    world.setBlockState(pos, &wall.defaultState());

    const BlockState state =
        wall.defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false);

    BlockState result =
        wall.updatePostPlacement(state, Direction::North, VanillaBlocks::AIR->defaultState(), world, pos, pos.north());
    EXPECT_TRUE(result.get(BlockStateProperties::UP()));
}

TEST_F(WallBlockTest, ShouldRaisePost_SymmetricTallStraightWall)
{
    // 对称直线Tall墙（南北都有Tall连接，东西无连接）不应升起墙柱
    // 参考: MC原版 WallBlock#shouldRaisePost 中 flag6 检查
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    // 设置上方为完整方块（使得固体邻居连接为Tall）
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos.south(), &solid.defaultState());
    world.setBlockState(pos.up(), &solid.defaultState());

    BlockState state = wall.defaultState()
                           .with(BlockStateProperties::UP(), false)
                           .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Tall)
                           .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
                           .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::Tall)
                           .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    BlockState result =
        wall.updatePostPlacement(state, Direction::North, solid.defaultState(), world, pos, pos.north());
    EXPECT_FALSE(result.get(BlockStateProperties::UP()));
}

TEST_F(WallBlockTest, ShouldRaisePost_SymmetricLowStraightWallNoPost)
{
    // 对称直线Low墙（南北都有Low连接，东西无连接，上方无覆盖）不升起墙柱
    // 参考: MC原版 WallBlock#shouldRaisePost，当两侧都非None且非Tall时，
    // 不满足flag6条件，进入 isCovered 检查，上方无覆盖返回false
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    // 设置上方无方块（空气），固体邻居连接为Low
    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos.south(), &solid.defaultState());
    // 上方为空气，所以面形状为空 -> Low连接

    BlockState state = wall.defaultState()
                           .with(BlockStateProperties::UP(), false)
                           .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Low)
                           .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
                           .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::Low)
                           .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    BlockState result =
        wall.updatePostPlacement(state, Direction::North, solid.defaultState(), world, pos, pos.north());
    // Low直线墙且上方无覆盖，shouldRaisePost返回false
    EXPECT_FALSE(result.get(BlockStateProperties::UP()));
}

TEST_F(WallBlockTest, ShouldRaisePost_SymmetricLowStraightWall)
{
    // 对称直线Low墙（南北都有Low连接，东西无连接）不应升起墙柱
    // 这是关键测试：旧实现仅检查Tall高度的直线墙，导致Low直线墙错误地升起墙柱
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    // 设置铁栏杆在南北方向（铁栏杆与墙连接返回 Low 高度）
    if (VanillaBlocks::IRON_BARS) {
        world.setBlockState(pos.north(), &VanillaBlocks::IRON_BARS->defaultState());
        world.setBlockState(pos.south(), &VanillaBlocks::IRON_BARS->defaultState());

        BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
        BlockState placedState = wall.getStateForPlacement(context);

        // 南北都连接铁栏杆 → Low 直线墙 → UP 应为 false
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Low);
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::Low);
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::None);
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::None);
        EXPECT_FALSE(placedState.get(BlockStateProperties::UP()));
    }
}

TEST_F(WallBlockTest, ShouldRaisePost_AsymmetricConnections)
{
    // 不对称连接（如只有北面连接，南面无连接）应升起墙柱
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    world.setBlockState(pos.north(), &solid.defaultState());

    BlockState state = wall.defaultState()
                           .with(BlockStateProperties::UP(), true)
                           .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::Tall)
                           .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
                           .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
                           .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    BlockState result =
        wall.updatePostPlacement(state, Direction::North, solid.defaultState(), world, pos, pos.north());
    EXPECT_TRUE(result.get(BlockStateProperties::UP()));
}

TEST_F(WallBlockTest, ShouldRaisePost_CornerConnections)
{
    // 角落连接（北面和东面都有连接）应升起墙柱
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos.east(), &solid.defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState placedState = wall.getStateForPlacement(context);

    // 北和东有连接，南和西无连接 → 不对称 → UP=true
    EXPECT_TRUE(placedState.get(BlockStateProperties::UP()));
}

// ============================================================================
// WallBlock _isCovered Tests
// 参考: net.minecraft.block.WallBlock#isCovered
// ============================================================================

TEST_F(WallBlockTest, IsCovered_FullBlockCoversTestShape)
{
    // 完整方块形状应覆盖任何测试形状
    VoxelShape fullBlock = Shapes::block();
    VoxelShape testShape = Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 9.0 / 16.0);
    EXPECT_TRUE(WallBlock::isCovered(testShape, fullBlock));
}

TEST_F(WallBlockTest, IsCovered_EmptyShapeDoesNotCover)
{
    // 空形状不应覆盖任何测试形状
    VoxelShape emptyShape = Shapes::empty();
    VoxelShape testShape = Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 9.0 / 16.0);
    EXPECT_FALSE(WallBlock::isCovered(testShape, emptyShape));
}

TEST_F(WallBlockTest, IsCovered_PartialShapeDoesNotCover)
{
    // 部分形状（如半砖下半部分）不应覆盖中心柱测试形状
    // 下半砖: (0, 0, 0) -> (1, 0.5, 1)
    VoxelShape halfSlab = Shapes::box(0.0, 0.0, 0.0, 1.0, 0.5, 1.0);
    VoxelShape testShapePost = Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 9.0 / 16.0);
    // TEST_SHAPE_POST 从 y=0 到 y=1，半砖只到 y=0.5，所以不覆盖
    // 注意: Down面投影只取接触Down面的部分，半砖的Down面投影是完整的，所以覆盖
    // 实际上需要取Down面投影来测试
    VoxelShape halfSlabDownFace = halfSlab.getFaceShape(Direction::Down);
    EXPECT_TRUE(WallBlock::isCovered(testShapePost, halfSlabDownFace));
}

TEST_F(WallBlockTest, IsCovered_NarrowShapeDoesNotCover)
{
    // 窄形状不应覆盖中心柱测试形状
    // 火把形状：中心细柱 (7/16, 0, 7/16) -> (9/16, 1, 9/16) 恰好等于 TEST_SHAPE_POST
    VoxelShape torchShape = Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 9.0 / 16.0);
    // 火把的Down面投影 = (7/16, 0, 7/16) -> (9/16, 1, 9/16)，完全覆盖TEST_SHAPE_POST
    VoxelShape torchDownFace = torchShape.getFaceShape(Direction::Down);
    EXPECT_TRUE(WallBlock::isCovered(torchShape, torchDownFace));
}

TEST_F(WallBlockTest, IsCovered_TestShapePostCoverage)
{
    // TEST_SHAPE_POST 是 (7/16, 0, 7/16) -> (9/16, 1, 9/16)
    // 一个比 TEST_SHAPE_POST 大的形状应该覆盖它
    VoxelShape largerShape = Shapes::box(6.0 / 16.0, 0.0, 6.0 / 16.0, 10.0 / 16.0, 1.0, 10.0 / 16.0);
    VoxelShape testShapePost = Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 9.0 / 16.0);
    EXPECT_TRUE(WallBlock::isCovered(testShapePost, largerShape));
}

// ============================================================================
// WallBlock Face Shape Coverage Tests (上方方块覆盖决定 TALL/LOW)
// ============================================================================

TEST_F(WallBlockTest, SolidBlockAboveMakesTallConnection)
{
    // 上方有完整方块时，固体邻居连接应为 Tall
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(8, 64, 8);

    world.setBlockState(pos.north(), &solid.defaultState());
    // 上方放置完整方块
    world.setBlockState(pos.up(), &solid.defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = wall.getStateForPlacement(context);

    // 上方有完整方块覆盖，连接应为 Tall
    EXPECT_EQ(state.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Tall);
}

// ============================================================================
// WallBlock 边界场景测试
// ============================================================================

TEST_F(WallBlockTest, CrossIntersection_AllFourSidesConnected_NoPostWhenLow)
{
    // 十字路口：四方向都有Low连接（上方无方块），不升起墙柱
    // 四方向都连接 → 对称（northNone==southNone, eastNone==westNone）
    // 不是直线Tall墙 → 检查上方覆盖 → 无覆盖 → UP=false
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos.south(), &solid.defaultState());
    world.setBlockState(pos.east(), &solid.defaultState());
    world.setBlockState(pos.west(), &solid.defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState placedState = wall.getStateForPlacement(context);

    // 四方向Low连接 + 上方无覆盖 → UP=false（不升起墙柱）
    EXPECT_FALSE(placedState.get(BlockStateProperties::UP()));
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Low);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Low);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::Low);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::Low);
}

TEST_F(WallBlockTest, TShapeWithSolidAbove_StillRaisesPost)
{
    // T形墙（北、东、西有连接，南无连接）+ 上方有完整方块
    // 不对称 → 应该升起墙柱
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos.east(), &solid.defaultState());
    world.setBlockState(pos.west(), &solid.defaultState());
    world.setBlockState(pos.up(), &solid.defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState placedState = wall.getStateForPlacement(context);

    // 不对称连接 → UP=true
    EXPECT_TRUE(placedState.get(BlockStateProperties::UP()));
    // 上方有完整方块 → 连接为Tall
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::None);
}

TEST_F(WallBlockTest, WallAboveWithUpTrue_ForceRaisePost)
{
    // 上方是墙且UP=true → 强制升起墙柱
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    // 设置南北方向有固体邻居（使连接对称），上方有墙且UP=true
    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos.south(), &solid.defaultState());

    if (VanillaBlocks::COBBLESTONE_WALL) {
        const BlockState& wallAbove =
            VanillaBlocks::COBBLESTONE_WALL->defaultState()
                .with(BlockStateProperties::UP(), true)
                .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::None)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
                .with(BlockStateProperties::WATERLOGGED(), false);
        world.setBlockState(pos.up(), &wallAbove);

        BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
        BlockState placedState = wall.getStateForPlacement(context);

        // 上方墙UP=true → 强制升起墙柱
        EXPECT_TRUE(placedState.get(BlockStateProperties::UP()));
    }
}

TEST_F(WallBlockTest, IsCovered_ShapeSliceFaceProjection)
{
    // 验证 Shapes::slice 坐标修复后 getFaceShape 的正确性
    // 半砖的 Down 面投影应该是完整的 (0,0,0)→(1,1,1) 形状
    VoxelShape halfSlab = Shapes::box(0.0, 0.0, 0.0, 1.0, 0.5, 1.0);
    VoxelShape halfSlabDownFace = halfSlab.getFaceShape(Direction::Down);
    // 半砖的 Down 面投影应该等同于完整方块
    EXPECT_TRUE(Shapes::isBlock(halfSlabDownFace));

    // 上半砖的 Down 面投影应该是空的（上半砖不接触底面）
    VoxelShape upperSlab = Shapes::box(0.0, 0.5, 0.0, 1.0, 1.0, 1.0);
    VoxelShape upperSlabDownFace = upperSlab.getFaceShape(Direction::Down);
    EXPECT_TRUE(upperSlabDownFace.isEmpty());
}

TEST_F(WallBlockTest, SolidBlockAbove_CrossShapeAllTall)
{
    // 十字路口 + 上方完整方块 → 四方向都是 Tall
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos.south(), &solid.defaultState());
    world.setBlockState(pos.east(), &solid.defaultState());
    world.setBlockState(pos.west(), &solid.defaultState());
    world.setBlockState(pos.up(), &solid.defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState placedState = wall.getStateForPlacement(context);

    // 上方有完整方块 → 所有连接都是 Tall
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::Tall);
    EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::Tall);
}

TEST_F(WallBlockTest, FenceGateParallel_ConnectionIsLow)
{
    // 栅栏门平行连接 → Low 高度（不受上方覆盖影响）
    WallBlock wall(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    WallTestWorld world;
    const BlockPos pos(5, 60, 5);

    if (VanillaBlocks::OAK_FENCE_GATE) {
        // 栅栏门朝南北方向（与墙的东西面平行）
        const BlockState& gateState = VanillaBlocks::OAK_FENCE_GATE->defaultState().with(
            BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
        world.setBlockState(pos.east(), &gateState);

        BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
        BlockState placedState = wall.getStateForPlacement(context);

        // 栅栏门平行方向连接 → Low
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_EAST()), BlockStateProperties::WallHeight::Low);
        // 其他方向无连接
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_NORTH()), BlockStateProperties::WallHeight::None);
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_SOUTH()), BlockStateProperties::WallHeight::None);
        EXPECT_EQ(placedState.get(BlockStateProperties::WALL_HEIGHT_WEST()), BlockStateProperties::WallHeight::None);
    }
}
