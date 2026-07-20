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
#include "world/block/blocks/copper/WeatheringCopperBarsBlock.hpp"
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

class CopperBarsTestWorld final : public test::BaseTestWorld {
public:
    CopperBarsTestWorld() = default;

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
        const_cast<CopperBarsTestWorld*>(this)->ensureTickManager();
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

} // namespace

// ============================================================================
// 铜栏杆方块测试
// ============================================================================

class CopperBarsTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ---------- 放置与连接逻辑 ----------

TEST_F(CopperBarsTestFixture, Placement_ConnectsToSolidAndWaterlogs)
{
    if (!VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS not registered";
    }

    CopperBarsTestWorld world;
    const BlockPos pos(8, 64, 8);

    TestSolidBlock solid(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    world.setBlockState(pos.north(), &solid.defaultState());
    world.setBlockState(pos, &VanillaBlocks::WATER->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_BARS->getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::WEST()));
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(CopperBarsTestFixture, Placement_ConnectsToIronBars)
{
    if (!VanillaBlocks::COPPER_BARS || !VanillaBlocks::IRON_BARS) {
        GTEST_SKIP() << "COPPER_BARS or IRON_BARS not registered";
    }

    CopperBarsTestWorld world;
    const BlockPos pos(10, 64, 10);

    // 北侧放铁栏杆
    world.setBlockState(pos.north(), &VanillaBlocks::IRON_BARS->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_BARS->getStateForPlacement(context);

    // 铜栏杆应通过 BARS 标签连接到铁栏杆
    EXPECT_TRUE(state.get(BlockStateProperties::NORTH()));
}

TEST_F(CopperBarsTestFixture, Placement_ConnectsToOtherCopperBars)
{
    if (!VanillaBlocks::COPPER_BARS || !VanillaBlocks::EXPOSED_COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS or EXPOSED_COPPER_BARS not registered";
    }

    CopperBarsTestWorld world;
    const BlockPos pos(12, 64, 12);

    // 南侧放风化铜栏杆
    world.setBlockState(pos.south(), &VanillaBlocks::EXPOSED_COPPER_BARS->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_BARS->getStateForPlacement(context);

    // 不同氧化等级的铜栏杆也应通过 BARS 标签互相连接
    EXPECT_TRUE(state.get(BlockStateProperties::SOUTH()));
}

TEST_F(CopperBarsTestFixture, Placement_ConnectsToWall)
{
    if (!VanillaBlocks::COPPER_BARS || !VanillaBlocks::COBBLESTONE_WALL) {
        GTEST_SKIP() << "COPPER_BARS or COBBLESTONE_WALL not registered";
    }

    CopperBarsTestWorld world;
    const BlockPos pos(14, 64, 14);

    // 东侧放墙
    world.setBlockState(pos.east(), &VanillaBlocks::COBBLESTONE_WALL->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_BARS->getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::EAST()));
}

// ---------- 形状计算 ----------

TEST_F(CopperBarsTestFixture, Shape_CenterPillarOnlyWhenNoConnections)
{
    if (!VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS not registered";
    }

    const BlockState state =
        VanillaBlocks::COPPER_BARS->defaultState()
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::OXIDATION(), BlockStateProperties::OxidationLevel::Unaffected);

    const CollisionShape& shape = VanillaBlocks::COPPER_BARS->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    // 仅中心柱，无连接臂
    EXPECT_EQ(shape.boxCount(), 1u);
}

TEST_F(CopperBarsTestFixture, Shape_CenterPlusConnectedSides)
{
    if (!VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS not registered";
    }

    const BlockState state =
        VanillaBlocks::COPPER_BARS->defaultState()
            .with(BlockStateProperties::NORTH(), true)
            .with(BlockStateProperties::EAST(), true)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::WEST(), true)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::OXIDATION(), BlockStateProperties::OxidationLevel::Unaffected);

    const CollisionShape& shape = VanillaBlocks::COPPER_BARS->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    // 中心柱 + 北 + 东 + 西 = 4 个碰撞箱
    EXPECT_EQ(shape.boxCount(), 4u);
}

// ---------- 氧化属性 ----------

TEST_F(CopperBarsTestFixture, OxidationLevel_UnaffectedCopperBars)
{
    if (!VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS not registered";
    }

    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::COPPER_BARS);
    ASSERT_NE(weatheringBlock, nullptr);
    EXPECT_EQ(weatheringBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Unaffected);
}

TEST_F(CopperBarsTestFixture, OxidationLevel_ExposedCopperBars)
{
    if (!VanillaBlocks::EXPOSED_COPPER_BARS) {
        GTEST_SKIP() << "EXPOSED_COPPER_BARS not registered";
    }

    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_BARS);
    ASSERT_NE(weatheringBlock, nullptr);
    EXPECT_EQ(weatheringBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Exposed);
}

TEST_F(CopperBarsTestFixture, OxidationLevel_WeatheredCopperBars)
{
    if (!VanillaBlocks::WEATHERED_COPPER_BARS) {
        GTEST_SKIP() << "WEATHERED_COPPER_BARS not registered";
    }

    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WEATHERED_COPPER_BARS);
    ASSERT_NE(weatheringBlock, nullptr);
    EXPECT_EQ(weatheringBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Weathered);
}

TEST_F(CopperBarsTestFixture, OxidationLevel_OxidizedCopperBars)
{
    if (!VanillaBlocks::OXIDIZED_COPPER_BARS) {
        GTEST_SKIP() << "OXIDIZED_COPPER_BARS not registered";
    }

    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_BARS);
    ASSERT_NE(weatheringBlock, nullptr);
    EXPECT_EQ(weatheringBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Oxidized);
}

TEST_F(CopperBarsTestFixture, OxidationChain_NextOxidationBlock)
{
    if (!VanillaBlocks::COPPER_BARS || !VanillaBlocks::EXPOSED_COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS or EXPOSED_COPPER_BARS not registered";
    }

    const auto* copper = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::COPPER_BARS);
    ASSERT_NE(copper, nullptr);
    EXPECT_EQ(copper->getNextOxidationBlock(), VanillaBlocks::EXPOSED_COPPER_BARS);
}

TEST_F(CopperBarsTestFixture, OxidationChain_PreviousOxidationBlock)
{
    if (!VanillaBlocks::COPPER_BARS || !VanillaBlocks::EXPOSED_COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS or EXPOSED_COPPER_BARS not registered";
    }

    const auto* exposed = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_BARS);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getPreviousOxidationBlock(), VanillaBlocks::COPPER_BARS);
}

TEST_F(CopperBarsTestFixture, OxidationChain_OxidizedHasNoNext)
{
    if (!VanillaBlocks::OXIDIZED_COPPER_BARS) {
        GTEST_SKIP() << "OXIDIZED_COPPER_BARS not registered";
    }

    const auto* oxidized = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_BARS);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_EQ(oxidized->getNextOxidationBlock(), nullptr);
}

TEST_F(CopperBarsTestFixture, TicksRandomly_OnlyWhenNotFullyOxidized)
{
    if (!VanillaBlocks::COPPER_BARS || !VanillaBlocks::OXIDIZED_COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS or OXIDIZED_COPPER_BARS not registered";
    }

    EXPECT_TRUE(VanillaBlocks::COPPER_BARS->ticksRandomly());

    const auto* oxidized = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_BARS);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_FALSE(VanillaBlocks::OXIDIZED_COPPER_BARS->ticksRandomly());
}

// ---------- 涂蜡版本 ----------

TEST_F(CopperBarsTestFixture, WaxedCopperBars_NotOxidizable)
{
    if (!VanillaBlocks::WAXED_COPPER_BARS) {
        GTEST_SKIP() << "WAXED_COPPER_BARS not registered";
    }

    const auto* waxedBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_COPPER_BARS);
    EXPECT_EQ(waxedBlock, nullptr);
}

TEST_F(CopperBarsTestFixture, WaxedCopperBars_DoesNotTickRandomly)
{
    if (!VanillaBlocks::WAXED_COPPER_BARS) {
        GTEST_SKIP() << "WAXED_COPPER_BARS not registered";
    }

    EXPECT_FALSE(VanillaBlocks::WAXED_COPPER_BARS->ticksRandomly());
}

TEST_F(CopperBarsTestFixture, WaxedCopperBars_PlacementConnectsToIronBars)
{
    if (!VanillaBlocks::WAXED_COPPER_BARS || !VanillaBlocks::IRON_BARS) {
        GTEST_SKIP() << "WAXED_COPPER_BARS or IRON_BARS not registered";
    }

    CopperBarsTestWorld world;
    const BlockPos pos(20, 64, 20);

    world.setBlockState(pos.west(), &VanillaBlocks::IRON_BARS->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::WAXED_COPPER_BARS->getStateForPlacement(context);

    // 涂蜡铜栏杆也应通过 BARS 标签连接到铁栏杆
    EXPECT_TRUE(state.get(BlockStateProperties::WEST()));
}

// ---------- 含水状态 ----------

TEST_F(CopperBarsTestFixture, WaterloggedState_ReturnsWaterFluid)
{
    if (!VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS not registered";
    }

    const BlockState waterloggedState =
        VanillaBlocks::COPPER_BARS->defaultState().with(BlockStateProperties::WATERLOGGED(), true);

    const fluid::FluidState* fluidState = VanillaBlocks::COPPER_BARS->getFluidState(waterloggedState);
    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

// ---------- 铁栏杆也连接到铜栏杆（反向连接） ----------

TEST_F(CopperBarsTestFixture, IronBarsConnectsToCopperBars)
{
    if (!VanillaBlocks::IRON_BARS || !VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "IRON_BARS or COPPER_BARS not registered";
    }

    CopperBarsTestWorld world;
    const BlockPos pos(25, 64, 25);

    // 东侧放铜栏杆
    world.setBlockState(pos.east(), &VanillaBlocks::COPPER_BARS->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::IRON_BARS->getStateForPlacement(context);

    // PaneBlock (铁栏杆) 应通过 BARS 标签连接到铜栏杆
    EXPECT_TRUE(state.get(BlockStateProperties::EAST()));
}

// ---------- 铁栏杆不应连接到树叶 ----------

TEST_F(CopperBarsTestFixture, CopperBarsDoesNotConnectToLeaves)
{
    if (!VanillaBlocks::COPPER_BARS || !VanillaBlocks::OAK_LEAVES) {
        GTEST_SKIP() << "COPPER_BARS or OAK_LEAVES not registered";
    }

    CopperBarsTestWorld world;
    const BlockPos pos(30, 64, 30);

    world.setBlockState(pos.north(), &VanillaBlocks::OAK_LEAVES->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_BARS->getStateForPlacement(context);

    // 树叶是连接例外，铜栏杆不应连接到树叶
    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));
}

// ---------- connectsTo 静态方法 ----------

TEST_F(CopperBarsTestFixture, ConnectsTo_StaticMethodReturnsDirectionState)
{
    if (!VanillaBlocks::COPPER_BARS) {
        GTEST_SKIP() << "COPPER_BARS not registered";
    }

    const BlockState northState =
        VanillaBlocks::COPPER_BARS->defaultState()
            .with(BlockStateProperties::NORTH(), true)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::OXIDATION(), BlockStateProperties::OxidationLevel::Unaffected);

    EXPECT_TRUE(WeatheringCopperBarsBlock::connectsTo(northState, Direction::North));
    EXPECT_FALSE(WeatheringCopperBarsBlock::connectsTo(northState, Direction::East));
}
