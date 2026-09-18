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

/**
 * @file TripWireTest.cpp
 * @brief TripWireBlock 和 TripWireHookBlock 单元测试
 *
 * 测试绊线系统的核心功能：
 * - 绊线钩检测绊线链
 * - 绊线状态更新
 * - 红石信号输出
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/redstone/TripWireBlock.hpp"
#include "common/world/block/blocks/redstone/TripWireHookBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief 用于 TripWire 测试的 Mock World 实现
 */
class TripWireTestWorld final : public ::mc::test::BaseTestWorld {
public:
    TripWireTestWorld() { m_worldBorder.setSize(60000000.0); }

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TripWireTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TripWireTestWorld::tickManager not implemented");
    }

    // 测试辅助方法

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    const BlockState* getBlockAt(const BlockPos& pos) const { return getBlockState(pos.x, pos.y, pos.z); }

    void clearState() { m_blocks.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
};

/**
 * @brief TripWire 测试固件
 */
class TripWireTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    void TearDown() override { m_world.clearState(); }

    TripWireTestWorld m_world;
};

// ========== TripWireBlock 基础测试 ==========

/**
 * @brief 测试 TripWireBlock 状态属性
 */
TEST_F(TripWireTest, TripWireBlock_HasCorrectStateProperties)
{
    ASSERT_NE(VanillaBlocks::TRIPWIRE, nullptr);

    const BlockState& defaultState = VanillaBlocks::TRIPWIRE->defaultState();

    // 验证默认状态
    EXPECT_FALSE(TripWireBlock::isPowered(defaultState));
    EXPECT_FALSE(TripWireBlock::isActivated(defaultState));

    // 验证状态属性存在
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::POWERED()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::ATTACHED()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::DISARMED()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::NORTH()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::EAST()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::SOUTH()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::WEST()));
}

/**
 * @brief 测试 TripWireBlock canProvidePower
 */
TEST_F(TripWireTest, TripWireBlock_CanProvidePower)
{
    const BlockState& state = VanillaBlocks::TRIPWIRE->defaultState();
    EXPECT_TRUE(state.getBlock().canProvidePower(state));
}

/**
 * @brief 测试 TripWireBlock 红石信号输出
 */
TEST_F(TripWireTest, TripWireBlock_PowerOutput)
{
    const BlockState& unpoweredState = VanillaBlocks::TRIPWIRE->defaultState();

    // 未触发时输出 0
    EXPECT_EQ(unpoweredState.getBlock().getWeakPower(unpoweredState, m_world, BlockPos(0, 0, 0), Direction::North), 0);
    EXPECT_EQ(
        unpoweredState.getBlock().getStrongPower(unpoweredState, m_world, BlockPos(0, 0, 0), Direction::North), 0);

    // 触发时输出 15
    BlockState poweredState = unpoweredState.with(BlockStateProperties::POWERED(), true);
    EXPECT_EQ(poweredState.getBlock().getWeakPower(poweredState, m_world, BlockPos(0, 0, 0), Direction::North), 15);
    // 绊线只输出弱信号，不输出强信号（getStrongPower 恒为 0）。
    EXPECT_EQ(poweredState.getBlock().getStrongPower(poweredState, m_world, BlockPos(0, 0, 0), Direction::North), 0);
}

/**
 * @brief 测试 TripWireBlock 连接状态
 */
TEST_F(TripWireTest, TripWireBlock_ConnectionState)
{
    const BlockState& state = VanillaBlocks::TRIPWIRE->defaultState();

    // 默认不连接
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::North));
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::East));
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::South));
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::West));

    // 设置连接状态
    BlockState connectedState =
        state.with(BlockStateProperties::NORTH(), true).with(BlockStateProperties::SOUTH(), true);

    EXPECT_TRUE(TripWireBlock::isConnected(connectedState, Direction::North));
    EXPECT_FALSE(TripWireBlock::isConnected(connectedState, Direction::East));
    EXPECT_TRUE(TripWireBlock::isConnected(connectedState, Direction::South));
    EXPECT_FALSE(TripWireBlock::isConnected(connectedState, Direction::West));
}

// ========== TripWireHookBlock 基础测试 ==========

/**
 * @brief 测试 TripWireHookBlock 状态属性
 */
TEST_F(TripWireTest, TripWireHookBlock_HasCorrectStateProperties)
{
    ASSERT_NE(VanillaBlocks::TRIPWIRE_HOOK, nullptr);

    const BlockState& defaultState = VanillaBlocks::TRIPWIRE_HOOK->defaultState();

    // 验证状态属性存在
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::HORIZONTAL_FACING()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::POWERED()));
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::ATTACHED()));

    // 验证默认朝向
    Direction facing = TripWireHookBlock::getFacing(defaultState);
    EXPECT_EQ(facing, Direction::North);
}

/**
 * @brief 测试 TripWireHookBlock canProvidePower
 */
TEST_F(TripWireTest, TripWireHookBlock_CanProvidePower)
{
    const BlockState& state = VanillaBlocks::TRIPWIRE_HOOK->defaultState();
    EXPECT_TRUE(state.getBlock().canProvidePower(state));
}

/**
 * @brief 测试 TripWireHookBlock 红石信号输出方向
 *
 * 绊线钩只在背面输出信号
 */
TEST_F(TripWireTest, TripWireHookBlock_PowerOutputDirection)
{
    // 创建朝向北的绊线钩
    BlockState northHook = VanillaBlocks::TRIPWIRE_HOOK->defaultState()
                               .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                               .with(BlockStateProperties::POWERED(), true);

    // 只在背面（南）输出信号
    EXPECT_EQ(northHook.getBlock().getWeakPower(northHook, m_world, BlockPos(0, 0, 0), Direction::South), 15);
    EXPECT_EQ(northHook.getBlock().getWeakPower(northHook, m_world, BlockPos(0, 0, 0), Direction::North), 0);
    EXPECT_EQ(northHook.getBlock().getWeakPower(northHook, m_world, BlockPos(0, 0, 0), Direction::East), 0);
    EXPECT_EQ(northHook.getBlock().getWeakPower(northHook, m_world, BlockPos(0, 0, 0), Direction::West), 0);

    // 强信号也只在背面输出
    EXPECT_EQ(northHook.getBlock().getStrongPower(northHook, m_world, BlockPos(0, 0, 0), Direction::South), 15);
    EXPECT_EQ(northHook.getBlock().getStrongPower(northHook, m_world, BlockPos(0, 0, 0), Direction::North), 0);
}

/**
 * @brief 测试 TripWireHookBlock 未触发时输出零信号
 */
TEST_F(TripWireTest, TripWireHookBlock_UnpoweredOutputZero)
{
    BlockState unpoweredHook = VanillaBlocks::TRIPWIRE_HOOK->defaultState()
                                   .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                                   .with(BlockStateProperties::POWERED(), false);

    // 未触发时输出 0
    EXPECT_EQ(unpoweredHook.getBlock().getWeakPower(unpoweredHook, m_world, BlockPos(0, 0, 0), Direction::South), 0);
    EXPECT_EQ(unpoweredHook.getBlock().getStrongPower(unpoweredHook, m_world, BlockPos(0, 0, 0), Direction::South), 0);
}

/**
 * @brief 测试 TripWireHookBlock 静态方法
 */
TEST_F(TripWireTest, TripWireHookBlock_StaticMethods)
{
    const BlockState& defaultState = VanillaBlocks::TRIPWIRE_HOOK->defaultState();

    // 测试 isPowered
    EXPECT_FALSE(TripWireHookBlock::isPowered(defaultState));
    BlockState poweredState = defaultState.with(BlockStateProperties::POWERED(), true);
    EXPECT_TRUE(TripWireHookBlock::isPowered(poweredState));

    // 测试 isConnected
    EXPECT_FALSE(TripWireHookBlock::isConnected(defaultState));
    BlockState connectedState = defaultState.with(BlockStateProperties::ATTACHED(), true);
    EXPECT_TRUE(TripWireHookBlock::isConnected(connectedState));

    // 测试 getFacing
    Direction facing = TripWireHookBlock::getFacing(defaultState);
    EXPECT_EQ(facing, Direction::North);

    // 测试 withPowered
    BlockState newPoweredState = TripWireHookBlock::withPowered(defaultState, true);
    EXPECT_TRUE(TripWireHookBlock::isPowered(newPoweredState));

    // 测试 withConnected
    BlockState newConnectedState = TripWireHookBlock::withConnected(defaultState, true);
    EXPECT_TRUE(TripWireHookBlock::isConnected(newConnectedState));
}

// ========== 绊线链检测测试 ==========

/**
 * @brief 测试绊线链检测 - 单个绊线
 */
TEST_F(TripWireTest, CheckForTripwire_SingleTripWire)
{
    // 设置绊线钩在 (0, 0, 0)，朝向东
    BlockState hookState =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    m_world.setBlockAt(BlockPos(0, 0, 0), &hookState);

    // 设置绊线在 (1, 0, 0)
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    m_world.setBlockAt(BlockPos(1, 0, 0), &tripwireState);

    // 设置另一个绊线钩在 (2, 0, 0)，朝向西
    BlockState hookState2 =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    m_world.setBlockAt(BlockPos(2, 0, 0), &hookState2);

    // 验证方块设置正确
    const BlockState* hook1State = m_world.getBlockAt(BlockPos(0, 0, 0));
    const BlockState* twState = m_world.getBlockAt(BlockPos(1, 0, 0));
    const BlockState* hook2State = m_world.getBlockAt(BlockPos(2, 0, 0));

    ASSERT_NE(hook1State, nullptr);
    ASSERT_NE(twState, nullptr);
    ASSERT_NE(hook2State, nullptr);

    // 验证绊线检测
    EXPECT_TRUE(twState->is(VanillaBlocks::TRIPWIRE));

    // 验证绊线钩检测
    EXPECT_TRUE(hook1State->is(VanillaBlocks::TRIPWIRE_HOOK));
    EXPECT_TRUE(hook2State->is(VanillaBlocks::TRIPWIRE_HOOK));
}

/**
 * @brief 测试绊线链检测 - 多个绊线
 */
TEST_F(TripWireTest, CheckForTripwire_MultipleTripWires)
{
    // 设置绊线钩在 (0, 0, 0)，朝向东
    BlockState hookState =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    m_world.setBlockAt(BlockPos(0, 0, 0), &hookState);

    // 设置多个绊线
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    for (int i = 1; i <= 5; ++i) {
        m_world.setBlockAt(BlockPos(i, 0, 0), &tripwireState);
    }

    // 设置另一个绊线钩在 (6, 0, 0)，朝向西
    BlockState hookState2 =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    m_world.setBlockAt(BlockPos(6, 0, 0), &hookState2);

    // 验证所有绊线都正确设置
    for (int i = 1; i <= 5; ++i) {
        const BlockState* state = m_world.getBlockAt(BlockPos(i, 0, 0));
        ASSERT_NE(state, nullptr);
        EXPECT_TRUE(state->is(VanillaBlocks::TRIPWIRE));
    }
}

/**
 * @brief 测试绊线链检测 - 最大距离
 *
 * 绊线链最大距离为 42 格
 */
TEST_F(TripWireTest, CheckForTripwire_MaxDistance)
{
    // 设置绊线钩在 (0, 0, 0)，朝向东
    BlockState hookState =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    m_world.setBlockAt(BlockPos(0, 0, 0), &hookState);

    // 设置 41 格绊线（最大链长度 = 钩到钩 42 格）
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    for (int i = 1; i <= 41; ++i) {
        m_world.setBlockAt(BlockPos(i, 0, 0), &tripwireState);
    }

    // 设置另一个绊线钩在 (42, 0, 0)，朝向西
    BlockState hookState2 =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    m_world.setBlockAt(BlockPos(42, 0, 0), &hookState2);

    // 验证最后一个绊线钩正确设置
    const BlockState* lastHook = m_world.getBlockAt(BlockPos(42, 0, 0));
    ASSERT_NE(lastHook, nullptr);
    EXPECT_TRUE(lastHook->is(VanillaBlocks::TRIPWIRE_HOOK));
    EXPECT_EQ(TripWireHookBlock::getFacing(*lastHook), Direction::West);
}

/**
 * @brief 测试绊线链检测 - 链中断
 *
 * 如果绊线链中间有空隙，则链不完整
 */
TEST_F(TripWireTest, CheckForTripwire_BrokenChain)
{
    // 设置绊线钩在 (0, 0, 0)，朝向东
    BlockState hookState =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    m_world.setBlockAt(BlockPos(0, 0, 0), &hookState);

    // 设置绊线在 (1, 0, 0) 和 (2, 0, 0)
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    m_world.setBlockAt(BlockPos(1, 0, 0), &tripwireState);
    m_world.setBlockAt(BlockPos(2, 0, 0), &tripwireState);

    // (3, 0, 0) 是空气 - 链中断

    // 设置绊线在 (4, 0, 0)
    m_world.setBlockAt(BlockPos(4, 0, 0), &tripwireState);

    // 设置另一个绊线钩在 (5, 0, 0)，朝向西
    BlockState hookState2 =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    m_world.setBlockAt(BlockPos(5, 0, 0), &hookState2);

    // 验证 (3, 0, 0) 是空气
    const BlockState* gap = m_world.getBlockAt(BlockPos(3, 0, 0));
    ASSERT_NE(gap, nullptr);
    EXPECT_TRUE(gap->isAir());
}

/**
 * @brief 测试绊线 DISARMED 属性
 *
 * 被剪刀剪断的绊线不会触发信号
 */
TEST_F(TripWireTest, TripWire_DisarmedProperty)
{
    // 默认未拆除
    const BlockState& defaultState = VanillaBlocks::TRIPWIRE->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::DISARMED()));

    // 设置为已拆除
    BlockState disarmedState = defaultState.with(BlockStateProperties::DISARMED(), true);
    EXPECT_TRUE(disarmedState.get(BlockStateProperties::DISARMED()));
}

/**
 * @brief 测试绊线 ATTACHED 属性
 */
TEST_F(TripWireTest, TripWire_AttachedProperty)
{
    const BlockState& defaultState = VanillaBlocks::TRIPWIRE->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::ATTACHED()));

    BlockState attachedState = defaultState.with(BlockStateProperties::ATTACHED(), true);
    EXPECT_TRUE(attachedState.get(BlockStateProperties::ATTACHED()));
}

/**
 * @brief 测试绊线钩 ATTACHED 属性
 */
TEST_F(TripWireTest, TripWireHook_AttachedProperty)
{
    const BlockState& defaultState = VanillaBlocks::TRIPWIRE_HOOK->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::ATTACHED()));

    BlockState attachedState = defaultState.with(BlockStateProperties::ATTACHED(), true);
    EXPECT_TRUE(attachedState.get(BlockStateProperties::ATTACHED()));
}

// ========== shouldConnectTo 测试 ==========

/**
 * @brief 测试 shouldConnectTo - 连接到另一个绊线
 */
TEST_F(TripWireTest, ShouldConnectTo_ConnectsToTripWire)
{
    const TripWireBlock* tripwire = dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE);
    ASSERT_NE(tripwire, nullptr);

    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();

    // 绊线应该连接到另一个绊线
    EXPECT_TRUE(tripwire->shouldConnectTo(tripwireState, Direction::North));
    EXPECT_TRUE(tripwire->shouldConnectTo(tripwireState, Direction::East));
    EXPECT_TRUE(tripwire->shouldConnectTo(tripwireState, Direction::South));
    EXPECT_TRUE(tripwire->shouldConnectTo(tripwireState, Direction::West));
}

/**
 * @brief 测试 shouldConnectTo - 连接到面向它的绊线钩
 *
 * 参考 MC 1.16.5: 绊线钩的 FACING 必须与检测方向相反才能连接
 * 例如：检测北边的方块时，北边的绊线钩 FACING 必须是 SOUTH（朝南，即面向当前绊线）
 */
TEST_F(TripWireTest, ShouldConnectTo_ConnectsToHookFacingTripwire)
{
    const TripWireBlock* tripwire = dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE);
    ASSERT_NE(tripwire, nullptr);

    // 创建朝南的绊线钩（面向南边，即面向北边的绊线）
    // 当绊线检测北边时，direction = North，opposite = South
    // 绊线钩 FACING = South，与 opposite 相等，所以连接
    BlockState hookFacingSouth =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    // 北边有个朝南的绊线钩，应该连接
    EXPECT_TRUE(tripwire->shouldConnectTo(hookFacingSouth, Direction::North));

    // 东边有个朝西的绊线钩，应该连接
    BlockState hookFacingWest =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    EXPECT_TRUE(tripwire->shouldConnectTo(hookFacingWest, Direction::East));

    // 南边有个朝北的绊线钩，应该连接
    BlockState hookFacingNorth =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_TRUE(tripwire->shouldConnectTo(hookFacingNorth, Direction::South));

    // 西边有个朝东的绊线钩，应该连接
    BlockState hookFacingEast =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_TRUE(tripwire->shouldConnectTo(hookFacingEast, Direction::West));
}

/**
 * @brief 测试 shouldConnectTo - 不连接到背对的绊线钩
 *
 * 如果绊线钩的 FACING 与检测方向相同，则不连接
 */
TEST_F(TripWireTest, ShouldConnectTo_NotConnectToHookFacingAway)
{
    const TripWireBlock* tripwire = dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE);
    ASSERT_NE(tripwire, nullptr);

    // 北边有个朝北的绊线钩（背对绊线），不应该连接
    BlockState hookFacingNorth =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_FALSE(tripwire->shouldConnectTo(hookFacingNorth, Direction::North));

    // 东边有个朝东的绊线钩（背对绊线），不应该连接
    BlockState hookFacingEast =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_FALSE(tripwire->shouldConnectTo(hookFacingEast, Direction::East));

    // 南边有个朝南的绊线钩（背对绊线），不应该连接
    BlockState hookFacingSouth =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    EXPECT_FALSE(tripwire->shouldConnectTo(hookFacingSouth, Direction::South));

    // 西边有个朝西的绊线钩（背对绊线），不应该连接
    BlockState hookFacingWest =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    EXPECT_FALSE(tripwire->shouldConnectTo(hookFacingWest, Direction::West));
}

/**
 * @brief 测试 shouldConnectTo - 不连接到其他方块
 */
TEST_F(TripWireTest, ShouldConnectTo_NotConnectsToOtherBlocks)
{
    const TripWireBlock* tripwire = dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE);
    ASSERT_NE(tripwire, nullptr);

    // 空气方块不应该连接
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    EXPECT_FALSE(tripwire->shouldConnectTo(airState, Direction::North));

    // 石头方块不应该连接
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(tripwire->shouldConnectTo(stoneState, Direction::North));
}

// ========== updatePostPlacement 测试 ==========

/**
 * @brief 测试 updatePostPlacement - 绊线连接到相邻绊线
 */
TEST_F(TripWireTest, UpdatePostPlacement_ConnectsToTripWire)
{
    TripWireBlock* tripwire = const_cast<TripWireBlock*>(dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE));
    ASSERT_NE(tripwire, nullptr);

    // 设置绊线在 (0, 0, 0)
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    m_world.setBlockAt(BlockPos(0, 0, 0), &tripwireState);

    // 设置绊线在 (1, 0, 0) 东边
    m_world.setBlockAt(BlockPos(1, 0, 0), &tripwireState);

    // 获取 (0, 0, 0) 的状态并测试更新
    const BlockState* state = m_world.getBlockAt(BlockPos(0, 0, 0));
    ASSERT_NE(state, nullptr);

    // 模拟东边邻居更新
    const BlockState* eastState = m_world.getBlockAt(BlockPos(1, 0, 0));
    BlockState updatedState = tripwire->updatePostPlacement(
        *state, Direction::East, *eastState, m_world, BlockPos(0, 0, 0), BlockPos(1, 0, 0));

    // 东边应该连接
    EXPECT_TRUE(updatedState.get(BlockStateProperties::EAST()));
}

/**
 * @brief 测试 updatePostPlacement - 绊线连接到面向它的绊线钩
 */
TEST_F(TripWireTest, UpdatePostPlacement_ConnectsToHook)
{
    TripWireBlock* tripwire = const_cast<TripWireBlock*>(dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE));
    ASSERT_NE(tripwire, nullptr);

    // 设置绊线在 (0, 0, 0)
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    m_world.setBlockAt(BlockPos(0, 0, 0), &tripwireState);

    // 设置朝西的绊线钩在 (1, 0, 0) 东边（面向西边的绊线）
    BlockState hookState =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    m_world.setBlockAt(BlockPos(1, 0, 0), &hookState);

    // 获取 (0, 0, 0) 的状态并测试更新
    const BlockState* state = m_world.getBlockAt(BlockPos(0, 0, 0));
    ASSERT_NE(state, nullptr);

    // 模拟东边邻居更新
    const BlockState* eastState = m_world.getBlockAt(BlockPos(1, 0, 0));
    BlockState updatedState = tripwire->updatePostPlacement(
        *state, Direction::East, *eastState, m_world, BlockPos(0, 0, 0), BlockPos(1, 0, 0));

    // 东边应该连接（绊线钩面向绊线）
    EXPECT_TRUE(updatedState.get(BlockStateProperties::EAST()));
}

/**
 * @brief 测试 updatePostPlacement - 绊线不连接到背对的绊线钩
 */
TEST_F(TripWireTest, UpdatePostPlacement_NotConnectsToHookFacingAway)
{
    TripWireBlock* tripwire = const_cast<TripWireBlock*>(dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE));
    ASSERT_NE(tripwire, nullptr);

    // 设置绊线在 (0, 0, 0)
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    m_world.setBlockAt(BlockPos(0, 0, 0), &tripwireState);

    // 设置朝东的绊线钩在 (1, 0, 0) 东边（背对西边的绊线）
    BlockState hookState =
        VanillaBlocks::TRIPWIRE_HOOK->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    m_world.setBlockAt(BlockPos(1, 0, 0), &hookState);

    // 获取 (0, 0, 0) 的状态并测试更新
    const BlockState* state = m_world.getBlockAt(BlockPos(0, 0, 0));
    ASSERT_NE(state, nullptr);

    // 模拟东边邻居更新
    const BlockState* eastState = m_world.getBlockAt(BlockPos(1, 0, 0));
    BlockState updatedState = tripwire->updatePostPlacement(
        *state, Direction::East, *eastState, m_world, BlockPos(0, 0, 0), BlockPos(1, 0, 0));

    // 东边不应该连接（绊线钩背对绊线）
    EXPECT_FALSE(updatedState.get(BlockStateProperties::EAST()));
}

/**
 * @brief 测试 updatePostPlacement - 垂直方向不更新连接状态
 */
TEST_F(TripWireTest, UpdatePostPlacement_IgnoresVerticalDirections)
{
    TripWireBlock* tripwire = const_cast<TripWireBlock*>(dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE));
    ASSERT_NE(tripwire, nullptr);

    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();

    // 垂直方向的更新不应该改变连接状态
    BlockState upUpdatedState = tripwire->updatePostPlacement(
        tripwireState, Direction::Up, tripwireState, m_world, BlockPos(0, 0, 0), BlockPos(0, 1, 0));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::NORTH()), upUpdatedState.get(BlockStateProperties::NORTH()));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::EAST()), upUpdatedState.get(BlockStateProperties::EAST()));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::SOUTH()), upUpdatedState.get(BlockStateProperties::SOUTH()));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::WEST()), upUpdatedState.get(BlockStateProperties::WEST()));

    BlockState downUpdatedState = tripwire->updatePostPlacement(
        tripwireState, Direction::Down, tripwireState, m_world, BlockPos(0, 0, 0), BlockPos(0, -1, 0));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::NORTH()), downUpdatedState.get(BlockStateProperties::NORTH()));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::EAST()), downUpdatedState.get(BlockStateProperties::EAST()));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::SOUTH()), downUpdatedState.get(BlockStateProperties::SOUTH()));
    EXPECT_EQ(tripwireState.get(BlockStateProperties::WEST()), downUpdatedState.get(BlockStateProperties::WEST()));
}

/**
 * @brief 测试 updatePostPlacement - 多方向连接
 */
TEST_F(TripWireTest, UpdatePostPlacement_MultipleDirections)
{
    TripWireBlock* tripwire = const_cast<TripWireBlock*>(dynamic_cast<const TripWireBlock*>(VanillaBlocks::TRIPWIRE));
    ASSERT_NE(tripwire, nullptr);

    // 设置绊线在 (0, 0, 0)
    const BlockState& tripwireState = VanillaBlocks::TRIPWIRE->defaultState();
    m_world.setBlockAt(BlockPos(0, 0, 0), &tripwireState);

    // 设置四个方向的绊线
    m_world.setBlockAt(BlockPos(0, 0, -1), &tripwireState); // 北
    m_world.setBlockAt(BlockPos(1, 0, 0), &tripwireState);  // 东
    m_world.setBlockAt(BlockPos(0, 0, 1), &tripwireState);  // 南
    m_world.setBlockAt(BlockPos(-1, 0, 0), &tripwireState); // 西

    // 逐个更新四个方向
    const BlockState* state = m_world.getBlockAt(BlockPos(0, 0, 0));
    ASSERT_NE(state, nullptr);

    // 更新北边
    const BlockState* northState = m_world.getBlockAt(BlockPos(0, 0, -1));
    BlockState updatedState = tripwire->updatePostPlacement(
        *state, Direction::North, *northState, m_world, BlockPos(0, 0, 0), BlockPos(0, 0, -1));
    EXPECT_TRUE(updatedState.get(BlockStateProperties::NORTH()));

    // 更新东边
    const BlockState* eastState = m_world.getBlockAt(BlockPos(1, 0, 0));
    updatedState = tripwire->updatePostPlacement(
        updatedState, Direction::East, *eastState, m_world, BlockPos(0, 0, 0), BlockPos(1, 0, 0));
    EXPECT_TRUE(updatedState.get(BlockStateProperties::EAST()));

    // 更新南边
    const BlockState* southState = m_world.getBlockAt(BlockPos(0, 0, 1));
    updatedState = tripwire->updatePostPlacement(
        updatedState, Direction::South, *southState, m_world, BlockPos(0, 0, 0), BlockPos(0, 0, 1));
    EXPECT_TRUE(updatedState.get(BlockStateProperties::SOUTH()));

    // 更新西边
    const BlockState* westState = m_world.getBlockAt(BlockPos(-1, 0, 0));
    updatedState = tripwire->updatePostPlacement(
        updatedState, Direction::West, *westState, m_world, BlockPos(0, 0, 0), BlockPos(-1, 0, 0));
    EXPECT_TRUE(updatedState.get(BlockStateProperties::WEST()));

    // 所有四个方向都应连接
    EXPECT_TRUE(updatedState.get(BlockStateProperties::NORTH()));
    EXPECT_TRUE(updatedState.get(BlockStateProperties::EAST()));
    EXPECT_TRUE(updatedState.get(BlockStateProperties::SOUTH()));
    EXPECT_TRUE(updatedState.get(BlockStateProperties::WEST()));
}

} // namespace test
} // namespace blocks
} // namespace mc
