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
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/decorative/PaneBlock.hpp"
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

class FenceWallTestWorld final : public mc::test::BaseTestWorld {
public:
    FenceWallTestWorld() = default;

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
        const_cast<FenceWallTestWorld*>(this)->ensureTickManager();
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

class FenceBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ============================================================================
// FenceBlock Connection Tests
// ============================================================================

TEST_F(FenceBlockTest, Placement_ConnectsToSameFenceType)
{
    // 参考: net.minecraft.block.FenceBlock#connectsTo - 同类栅栏连接
    // 木质栅栏连接木质栅栏（使用VanillaBlocks确保标签注册正确）
    if (!VanillaBlocks::OAK_FENCE || !VanillaBlocks::SPRUCE_FENCE) {
        GTEST_SKIP() << "OAK_FENCE or SPRUCE_FENCE not registered";
    }

    FenceWallTestWorld world;
    const BlockPos pos(8, 64, 8);

    // 橡木栅栏旁边放云杉木栅栏——两者都是木质栅栏，应该连接
    world.setBlockState(pos.north(), &VanillaBlocks::SPRUCE_FENCE->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::OAK_FENCE->getStateForPlacement(context);

    // 木质栅栏之间互相连接
    EXPECT_TRUE(state.get(BlockStateProperties::NORTH()));
}

TEST_F(FenceBlockTest, VanillaFenceConnectsToSolidBlock)
{
    // 固体方块连接
    if (!VanillaBlocks::OAK_FENCE) {
        GTEST_SKIP() << "OAK_FENCE not registered";
    }

    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    FenceWallTestWorld world;
    const BlockPos pos(8, 64, 8);

    world.setBlockState(pos.east(), &solid.defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::OAK_FENCE->getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::EAST()));
}

TEST_F(FenceBlockTest, VanillaFenceDoesNotConnectToLeaves)
{
    // 参考: Block::isExceptionForConnection - 树叶是连接例外
    if (!VanillaBlocks::OAK_FENCE || !VanillaBlocks::OAK_LEAVES) {
        GTEST_SKIP() << "OAK_FENCE or OAK_LEAVES not registered";
    }

    FenceWallTestWorld world;
    const BlockPos pos(8, 64, 8);

    world.setBlockState(pos.south(), &VanillaBlocks::OAK_LEAVES->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::OAK_FENCE->getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::SOUTH()));
}

TEST_F(FenceBlockTest, WoodenFenceTagValidation)
{
    // 验证 FENCES 和 WOODEN_FENCES 标签的分化逻辑
    // 木质栅栏应同时属于 FENCES 和 WOODEN_FENCES
    if (!VanillaBlocks::OAK_FENCE) {
        GTEST_SKIP() << "OAK_FENCE not registered";
    }

    EXPECT_TRUE(BlockTags::FENCES().contains(*VanillaBlocks::OAK_FENCE));
    EXPECT_TRUE(BlockTags::WOODEN_FENCES().contains(*VanillaBlocks::OAK_FENCE));

    if (VanillaBlocks::SPRUCE_FENCE) {
        EXPECT_TRUE(BlockTags::FENCES().contains(*VanillaBlocks::SPRUCE_FENCE));
        EXPECT_TRUE(BlockTags::WOODEN_FENCES().contains(*VanillaBlocks::SPRUCE_FENCE));
    }
}

TEST_F(FenceBlockTest, FenceTagDoesNotContainSolidBlock)
{
    // 固体方块不属于 FENCES 标签
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    EXPECT_FALSE(BlockTags::FENCES().contains(solid.defaultState()));
    EXPECT_FALSE(BlockTags::WOODEN_FENCES().contains(solid.defaultState()));
}

TEST_F(FenceBlockTest, Placement_DoesNotConnectToAir)
{
    // 空气不应连接
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));
    FenceWallTestWorld world;
    const BlockPos pos(8, 64, 8);

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = fence.getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::WEST()));
}

TEST_F(FenceBlockTest, UpdatePostPlacement_UpdatesConnectionOnNeighborChange)
{
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));
    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(10.0f));
    FenceWallTestWorld world;
    const BlockPos pos(12, 70, 3);

    const BlockState state = fence.defaultState()
                                 .with(BlockStateProperties::NORTH(), false)
                                 .with(BlockStateProperties::EAST(), false)
                                 .with(BlockStateProperties::SOUTH(), false)
                                 .with(BlockStateProperties::WEST(), false)
                                 .with(BlockStateProperties::WATERLOGGED(), false);

    const BlockState updated =
        fence.updatePostPlacement(state, Direction::North, solid.defaultState(), world, pos, pos.north());

    EXPECT_TRUE(updated.get(BlockStateProperties::NORTH()));
}

TEST_F(FenceBlockTest, Shape_PillarOnlyWhenNoConnections)
{
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    const BlockState state = fence.defaultState()
                                 .with(BlockStateProperties::NORTH(), false)
                                 .with(BlockStateProperties::EAST(), false)
                                 .with(BlockStateProperties::SOUTH(), false)
                                 .with(BlockStateProperties::WEST(), false)
                                 .with(BlockStateProperties::WATERLOGGED(), false);

    const CollisionShape& shape = fence.getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    // 无连接时只有柱子，boxCount应该为1
    EXPECT_EQ(shape.boxCount(), 1u);
}

TEST_F(FenceBlockTest, Shape_CombinesPillarAndRails)
{
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    const BlockState state = fence.defaultState()
                                 .with(BlockStateProperties::NORTH(), true)
                                 .with(BlockStateProperties::EAST(), true)
                                 .with(BlockStateProperties::SOUTH(), false)
                                 .with(BlockStateProperties::WEST(), false)
                                 .with(BlockStateProperties::WATERLOGGED(), false);

    const CollisionShape& shape = fence.getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    // 柱子 + 北面横杆 + 东面横杆 = 3个box
    EXPECT_EQ(shape.boxCount(), 3u);
}

TEST_F(FenceBlockTest, Rotate_RotatesConnections)
{
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    const BlockState state = fence.defaultState()
                                 .with(BlockStateProperties::NORTH(), true)
                                 .with(BlockStateProperties::EAST(), false)
                                 .with(BlockStateProperties::SOUTH(), false)
                                 .with(BlockStateProperties::WEST(), false)
                                 .with(BlockStateProperties::WATERLOGGED(), false);

    // 顺时针90度旋转: North -> East
    const BlockState& rotated = fence.rotate(state, Rotation::Clockwise90);
    EXPECT_FALSE(rotated.get(BlockStateProperties::NORTH()));
    EXPECT_TRUE(rotated.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(rotated.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(rotated.get(BlockStateProperties::WEST()));
}

TEST_F(FenceBlockTest, Mirror_SwapsConnections)
{
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    const BlockState state = fence.defaultState()
                                 .with(BlockStateProperties::NORTH(), true)
                                 .with(BlockStateProperties::EAST(), true)
                                 .with(BlockStateProperties::SOUTH(), false)
                                 .with(BlockStateProperties::WEST(), false)
                                 .with(BlockStateProperties::WATERLOGGED(), false);

    // 左右镜像: East <-> West
    const BlockState& mirrored = fence.mirror(state, Mirror::LeftRight);
    EXPECT_TRUE(mirrored.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(mirrored.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(mirrored.get(BlockStateProperties::SOUTH()));
    EXPECT_TRUE(mirrored.get(BlockStateProperties::WEST()));
}

TEST_F(FenceBlockTest, Waterlogged_ReturnsCorrectFluidState)
{
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));
    fluid::FluidRegistry::instance().initialize();

    const BlockState waterloggedState = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = fence.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(FenceBlockTest, IsWaterlogged)
{
    FenceBlock fence(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f));

    const BlockState waterlogged = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const BlockState notWaterlogged = fence.defaultState().with(BlockStateProperties::WATERLOGGED(), false);

    EXPECT_TRUE(fence.isWaterlogged(waterlogged));
    EXPECT_FALSE(fence.isWaterlogged(notWaterlogged));
}
