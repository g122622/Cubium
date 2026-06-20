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
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "common/world/block/blocks/redstone/ActivatorRailBlock.hpp"
#include "common/world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "common/world/block/blocks/redstone/PoweredRailBlock.hpp"
#include "common/world/block/blocks/redstone/RailBlock.hpp"
#include "common/world/block/registry/RedstoneBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/math/Vector3.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

namespace {

/**
 * @brief 铁轨 WaterLoggable 测试用的模拟世界
 *
 * 提供方块状态存储和流体状态检测功能。
 * 支持手动设置流体状态以测试含水放置行为。
 */
class RailWaterlogTestWorld : public test::BaseTestWorld {
public:
    RailWaterlogTestWorld() { m_airState = &VanillaBlocks::AIR->defaultState(); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return m_airState;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        i64 key = packPos(x, y, z);
        if (state == nullptr || state == m_airState) {
            m_blocks.erase(key);
        } else {
            m_blocks[key] = state;
        }
        return true;
    }

    [[nodiscard]] bool hasChunk(i32, i32) const override { return true; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        // 优先检查手动设置的流体状态
        const auto fluidIt = m_fluids.find(packPos(x, y, z));
        if (fluidIt != m_fluids.end() && fluidIt->second != nullptr) {
            return fluidIt->second;
        }

        // 否则检查方块的含水流体状态
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return fluidState;
            }
        }

        return fluid::Fluid::getFluidState(0);
    }

    void setBlockDirectly(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[packPos(pos.x, pos.y, pos.z)] = state;
    }

    void setFluidDirectly(const BlockPos& pos, const fluid::FluidState* state)
    {
        m_fluids[packPos(pos.x, pos.y, pos.z)] = state;
    }

    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

private:
    std::unordered_map<i64, const BlockState*> m_blocks;
    std::unordered_map<i64, const fluid::FluidState*> m_fluids;
    const BlockState* m_airState;
};

/**
 * @brief 创建放置上下文的辅助函数
 */
BlockItemUseContext makePlacementContext(
    IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw, f32 hitY = 0.5f)
{
    Vector3 hitPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + hitY, static_cast<f32>(pos.z) + 0.5f);
    ItemStack stack;
    return BlockItemUseContext(world, nullptr, stack, hitPos, pos, face, playerYaw);
}

} // namespace

// ============================================================================
// 铁轨 WaterLoggable 测试固件
// ============================================================================

class RailBlockWaterlogTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// 1. 默认状态 WATERLOGGED=false
// ============================================================================

TEST_F(RailBlockWaterlogTest, RailBlock_DefaultState_WaterloggedFalse)
{
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    const BlockState& defaultState = RedstoneBlocks::RAIL->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_DefaultState_WaterloggedFalse)
{
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    const BlockState& defaultState = RedstoneBlocks::POWERED_RAIL->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_DefaultState_WaterloggedFalse)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& defaultState = RedstoneBlocks::DETECTOR_RAIL->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_DefaultState_WaterloggedFalse)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    const BlockState& defaultState = RedstoneBlocks::ACTIVATOR_RAIL->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// 2. WATERLOGGED 属性可切换
// ============================================================================

TEST_F(RailBlockWaterlogTest, RailBlock_WaterloggedProperty_CanBeToggled)
{
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    BlockState state = RedstoneBlocks::RAIL->defaultState();

    // 设置 WATERLOGGED=true
    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));

    // 设置回 WATERLOGGED=false
    state = state.with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_WaterloggedProperty_CanBeToggled)
{
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    BlockState state = RedstoneBlocks::POWERED_RAIL->defaultState();

    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));

    state = state.with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_WaterloggedProperty_CanBeToggled)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    BlockState state = RedstoneBlocks::DETECTOR_RAIL->defaultState();

    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));

    state = state.with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_WaterloggedProperty_CanBeToggled)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    BlockState state = RedstoneBlocks::ACTIVATOR_RAIL->defaultState();

    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));

    state = state.with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// 3. isWaterlogged 方法
// ============================================================================

TEST_F(RailBlockWaterlogTest, RailBlock_IsWaterlogged_TrueWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(rail.isWaterlogged(waterloggedState));
}

TEST_F(RailBlockWaterlogTest, RailBlock_IsWaterlogged_FalseWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(rail.isWaterlogged(normalState));
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_IsWaterlogged_TrueWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::POWERED_RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(rail.isWaterlogged(waterloggedState));
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_IsWaterlogged_FalseWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::POWERED_RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(rail.isWaterlogged(normalState));
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_IsWaterlogged_TrueWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::DETECTOR_RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(rail.isWaterlogged(waterloggedState));
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_IsWaterlogged_FalseWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::DETECTOR_RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(rail.isWaterlogged(normalState));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_IsWaterlogged_TrueWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::ACTIVATOR_RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(rail.isWaterlogged(waterloggedState));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_IsWaterlogged_FalseWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::ACTIVATOR_RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(rail.isWaterlogged(normalState));
}

// ============================================================================
// 4. getFluidState 返回水流状态
// ============================================================================

TEST_F(RailBlockWaterlogTest, RailBlock_GetFluidState_ReturnsWaterWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = rail.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(RailBlockWaterlogTest, RailBlock_GetFluidState_ReturnsEmptyWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* fluidState = rail.getFluidState(normalState);

    EXPECT_TRUE(fluidState == nullptr || fluidState->isEmpty());
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_GetFluidState_ReturnsWaterWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::POWERED_RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = rail.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_GetFluidState_ReturnsEmptyWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::POWERED_RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* fluidState = rail.getFluidState(normalState);

    EXPECT_TRUE(fluidState == nullptr || fluidState->isEmpty());
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_GetFluidState_ReturnsWaterWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::DETECTOR_RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = rail.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_GetFluidState_ReturnsEmptyWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::DETECTOR_RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* fluidState = rail.getFluidState(normalState);

    EXPECT_TRUE(fluidState == nullptr || fluidState->isEmpty());
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_GetFluidState_ReturnsWaterWhenWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::ACTIVATOR_RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = rail.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_GetFluidState_ReturnsEmptyWhenNotWaterlogged)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::ACTIVATOR_RAIL);
    BlockState normalState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* fluidState = rail.getFluidState(normalState);

    EXPECT_TRUE(fluidState == nullptr || fluidState->isEmpty());
}

// ============================================================================
// 5. IWaterLoggable 接口 dynamic_cast
// ============================================================================

TEST_F(RailBlockWaterlogTest, RailBlock_ImplementsIWaterLoggable)
{
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    const IWaterLoggable* waterloggable = dynamic_cast<const IWaterLoggable*>(RedstoneBlocks::RAIL);
    EXPECT_NE(waterloggable, nullptr);
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_ImplementsIWaterLoggable)
{
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    const IWaterLoggable* waterloggable = dynamic_cast<const IWaterLoggable*>(RedstoneBlocks::POWERED_RAIL);
    EXPECT_NE(waterloggable, nullptr);
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_ImplementsIWaterLoggable)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const IWaterLoggable* waterloggable = dynamic_cast<const IWaterLoggable*>(RedstoneBlocks::DETECTOR_RAIL);
    EXPECT_NE(waterloggable, nullptr);
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_ImplementsIWaterLoggable)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    const IWaterLoggable* waterloggable = dynamic_cast<const IWaterLoggable*>(RedstoneBlocks::ACTIVATOR_RAIL);
    EXPECT_NE(waterloggable, nullptr);
}

// ============================================================================
// 6. getStateForPlacement 含水放置
// ============================================================================

TEST_F(RailBlockWaterlogTest, RailBlock_GetStateForPlacement_DefaultNotWaterlogged)
{
    // 默认测试世界中无水，放置时 WATERLOGGED 应为 false
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);
    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::RAIL);

    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_GetStateForPlacement_DefaultNotWaterlogged)
{
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);
    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::POWERED_RAIL);

    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_GetStateForPlacement_DefaultNotWaterlogged)
{
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);
    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::DETECTOR_RAIL);

    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_GetStateForPlacement_DefaultNotWaterlogged)
{
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);
    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::ACTIVATOR_RAIL);

    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, RailBlock_GetStateForPlacement_WaterloggedWhenInWater)
{
    // 在有水的位置放置铁轨，WATERLOGGED 应为 true
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);

    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::RAIL);
    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_GetStateForPlacement_WaterloggedWhenInWater)
{
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);

    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::POWERED_RAIL);
    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_GetStateForPlacement_WaterloggedWhenInWater)
{
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);

    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::DETECTOR_RAIL);
    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_GetStateForPlacement_WaterloggedWhenInWater)
{
    RailWaterlogTestWorld world;
    BlockPos pos(5, 10, 5);

    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(pos, &waterFluid->defaultState());

    AbstractRailBlock& rail = dynamic_cast<AbstractRailBlock&>(*RedstoneBlocks::ACTIVATOR_RAIL);
    auto context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    BlockState state = rail.getStateForPlacement(context);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// 7. WaterLoggableHelpers 与铁轨的交互
// ============================================================================

TEST_F(RailBlockWaterlogTest, GetWaterFluidState_ReturnsWaterWhenWaterlogged)
{
    // 使用 WaterLoggableHelpers::getWaterFluidState 验证含水铁轨返回水流状态
    BlockState waterloggedState = RedstoneBlocks::RAIL->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(waterloggedState);
    ASSERT_NE(waterState, nullptr);
    EXPECT_TRUE(waterState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(RailBlockWaterlogTest, GetWaterFluidState_ReturnsNullptrWhenNotWaterlogged)
{
    BlockState normalState = RedstoneBlocks::RAIL->defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(normalState);
    EXPECT_EQ(waterState, nullptr);
}

TEST_F(RailBlockWaterlogTest, IsWaterFluidState_ReturnsTrueForRailWaterloggedFluid)
{
    const AbstractRailBlock& rail = dynamic_cast<const AbstractRailBlock&>(*RedstoneBlocks::RAIL);
    BlockState waterloggedState = rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = rail.getFluidState(waterloggedState);

    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(waterloggable::isWaterFluidState(fluidState));
}

// ============================================================================
// 8. 含水状态下方块属性仍可正常操作
// ============================================================================

TEST_F(RailBlockWaterlogTest, RailBlock_WaterloggedWithShapeChange)
{
    // 验证含水状态可以和铁轨形状属性同时设置
    ASSERT_NE(RedstoneBlocks::RAIL, nullptr);
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*RedstoneBlocks::RAIL);

    BlockState state = rail.defaultState()
                           .with(BlockStateProperties::WATERLOGGED(), true)
                           .with(RailBlock::SHAPE(), RailShape::EastWest);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_EQ(rail.getRailShape(state), RailShape::EastWest);
}

TEST_F(RailBlockWaterlogTest, PoweredRailBlock_WaterloggedWithPoweredChange)
{
    // 验证含水状态可以和 POWERED 属性同时设置
    ASSERT_NE(RedstoneBlocks::POWERED_RAIL, nullptr);
    const PoweredRailBlock& rail = dynamic_cast<const PoweredRailBlock&>(*RedstoneBlocks::POWERED_RAIL);

    BlockState state = rail.defaultState()
                           .with(BlockStateProperties::WATERLOGGED(), true)
                           .with(PoweredRailBlock::POWERED(), true)
                           .with(PoweredRailBlock::SHAPE(), RailShape::AscendingNorth);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_TRUE(PoweredRailBlock::isPowered(state));
    EXPECT_EQ(rail.getRailShape(state), RailShape::AscendingNorth);
}

TEST_F(RailBlockWaterlogTest, DetectorRailBlock_WaterloggedWithPoweredChange)
{
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const DetectorRailBlock& rail = dynamic_cast<const DetectorRailBlock&>(*RedstoneBlocks::DETECTOR_RAIL);

    BlockState state =
        rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true).with(DetectorRailBlock::POWERED(), true);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_TRUE(DetectorRailBlock::isPowered(state));
}

TEST_F(RailBlockWaterlogTest, ActivatorRailBlock_WaterloggedWithPoweredChange)
{
    ASSERT_NE(RedstoneBlocks::ACTIVATOR_RAIL, nullptr);
    const ActivatorRailBlock& rail = dynamic_cast<const ActivatorRailBlock&>(*RedstoneBlocks::ACTIVATOR_RAIL);

    BlockState state =
        rail.defaultState().with(BlockStateProperties::WATERLOGGED(), true).with(ActivatorRailBlock::POWERED(), true);

    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_TRUE(ActivatorRailBlock::isPowered(state));
}
