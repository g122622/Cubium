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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/block/blocks/redstone/RailState.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "common/world/block/blocks/redstone/ActivatorRailBlock.hpp"
#include "common/world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "common/world/block/blocks/redstone/PoweredRailBlock.hpp"
#include "common/world/block/blocks/redstone/RailBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <map>
#include <memory>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;
using namespace mc::test;

namespace {

/**
 * @brief 铁轨测试世界
 *
 * 提供简单的方块状态存储，用于测试 RailState 的形状计算逻辑。
 * 使用 std::map<BlockPos, unique_ptr<BlockState>> 确保方块状态指针的生命周期。
 */
class RailTestWorld : public BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return nullptr;
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

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        return setBlockState(x, y, z, state);
    }

    void setRail(i32 x, i32 y, i32 z, const AbstractRailBlock& rail, RailShape shape)
    {
        BlockState state = rail.withRailShape(rail.defaultState(), shape);
        setBlockState(x, y, z, &state);
    }

    void clearAll() { m_blocks.clear(); }

private:
    std::map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
};

} // anonymous namespace

// ============================================================================
// RailState 形状计算测试
// ============================================================================

TEST(RailStateTest, SingleRailDefaultsToNorthSouth)
{
    // 单个铁轨无邻居时，应保持当前形状（默认为NorthSouth）
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);

    RailState state(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = state.place(false, false, RailShape::NorthSouth);

    EXPECT_EQ(rail.getRailShape(result), RailShape::NorthSouth);
}

TEST(RailStateTest, SingleRailDefaultsToEastWest)
{
    // 单个铁轨东西朝向无邻居时，应保持EastWest
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::EastWest);

    RailState state(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = state.place(false, false, RailShape::EastWest);

    EXPECT_EQ(rail.getRailShape(result), RailShape::EastWest);
}

TEST(RailStateTest, TwoRailsNorthSouth)
{
    // 两个南北相邻的铁轨应形成南北直轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth); // 北方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::EastWest);

    EXPECT_EQ(rail.getRailShape(result), RailShape::NorthSouth);
}

TEST(RailStateTest, TwoRailsEastWest)
{
    // 两个东西相邻的铁轨应形成东西直轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::EastWest);
    world.setRail(1, 0, 0, rail, RailShape::EastWest); // 东方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::NorthSouth);

    EXPECT_EQ(rail.getRailShape(result), RailShape::EastWest);
}

TEST(RailStateTest, CurvedRailSouthEast)
{
    // 南+东方向有两个铁轨时，应形成东南弯轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(1, 0, 0, rail, RailShape::EastWest);   // 东方
    world.setRail(0, 0, 1, rail, RailShape::NorthSouth); // 南方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::NorthSouth);

    EXPECT_EQ(rail.getRailShape(result), RailShape::SouthEast);
}

TEST(RailStateTest, CurvedRailNorthWest)
{
    // 北+西方向有两个铁轨时，应形成西北弯轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(-1, 0, 0, rail, RailShape::EastWest);   // 西方
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth); // 北方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::NorthSouth);

    EXPECT_EQ(rail.getRailShape(result), RailShape::NorthWest);
}

TEST(RailStateTest, ThreeConnectionsUnpoweredSouthEast)
{
    // 三连接（北+南+东），无红石信号时优先选择SE弯轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(1, 0, 0, rail, RailShape::EastWest);    // 东方
    world.setRail(0, 0, 1, rail, RailShape::NorthSouth);  // 南方
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth); // 北方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::NorthSouth);

    // 无红石信号：SE优先（无信号时 SE 是最后匹配的）
    EXPECT_EQ(rail.getRailShape(result), RailShape::SouthEast);
}

TEST(RailStateTest, ThreeConnectionsPoweredNorthEast)
{
    // 三连接（北+南+东），有红石信号时优先选择NE弯轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(1, 0, 0, rail, RailShape::EastWest);    // 东方
    world.setRail(0, 0, 1, rail, RailShape::NorthSouth);  // 南方
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth); // 北方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(true, false, RailShape::NorthSouth);

    // 有红石信号：NE优先（有信号时 NW 是最后匹配的，但需要NW连接条件满足）
    // 此处只有 N+S+E 三个方向，有信号时 NW 不满足，NE 满足
    EXPECT_EQ(rail.getRailShape(result), RailShape::NorthEast);
}

TEST(RailStateTest, PoweredRailNoCurves)
{
    // 动力铁轨不支持弯轨，三连接时保持当前形状
    RailTestWorld world;
    const PoweredRailBlock& poweredRail = dynamic_cast<const PoweredRailBlock&>(*VanillaBlocks::POWERED_RAIL);
    world.setRail(0, 0, 0, poweredRail, RailShape::NorthSouth);
    world.setRail(1, 0, 0, poweredRail, RailShape::EastWest);   // 东方
    world.setRail(0, 0, 1, poweredRail, RailShape::NorthSouth); // 南方

    RailState rstate(world, BlockPos(0, 0, 0), poweredRail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::NorthSouth);

    // 动力铁轨不支持弯轨，三连接时保持当前形状
    EXPECT_TRUE(poweredRail.getRailShape(result) == RailShape::NorthSouth ||
        poweredRail.getRailShape(result) == RailShape::EastWest);
    // 不应该选择弯轨形状
    EXPECT_NE(poweredRail.getRailShape(result), RailShape::SouthEast);
    EXPECT_NE(poweredRail.getRailShape(result), RailShape::SouthWest);
    EXPECT_NE(poweredRail.getRailShape(result), RailShape::NorthWest);
    EXPECT_NE(poweredRail.getRailShape(result), RailShape::NorthEast);
}

TEST(RailStateTest, AscendingRail)
{
    // 铁轨北方向上方有铁轨时，应形成向北上升的斜坡
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth); // 北方同层
    world.setRail(0, 1, -1, rail, RailShape::NorthSouth); // 北方上方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::NorthSouth);

    EXPECT_EQ(rail.getRailShape(result), RailShape::AscendingNorth);
}

TEST(RailStateTest, CountPotentialConnections)
{
    // 测试潜在连接数计算
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);

    // 无邻居
    RailState state0(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    EXPECT_EQ(state0.countPotentialConnections(), 0);

    // 两个邻居
    world.setRail(1, 0, 0, rail, RailShape::EastWest);
    world.setRail(0, 0, 1, rail, RailShape::NorthSouth);
    RailState state2(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    EXPECT_EQ(state2.countPotentialConnections(), 2);

    // 四个邻居
    world.setRail(-1, 0, 0, rail, RailShape::EastWest);
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth);
    RailState state4(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    EXPECT_EQ(state4.countPotentialConnections(), 4);
}

TEST(RailStateTest, IsStraightFlag)
{
    // 验证 isStraight 标志
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    const PoweredRailBlock& poweredRail = dynamic_cast<const PoweredRailBlock&>(*VanillaBlocks::POWERED_RAIL);
    const DetectorRailBlock& detectorRail = dynamic_cast<const DetectorRailBlock&>(*VanillaBlocks::DETECTOR_RAIL);
    const ActivatorRailBlock& activatorRail = dynamic_cast<const ActivatorRailBlock&>(*VanillaBlocks::ACTIVATOR_RAIL);

    EXPECT_FALSE(rail.isStraight());         // 普通铁轨支持弯轨
    EXPECT_TRUE(poweredRail.isStraight());   // 动力铁轨不支持弯轨
    EXPECT_TRUE(detectorRail.isStraight());  // 探测铁轨不支持弯轨
    EXPECT_TRUE(activatorRail.isStraight()); // 激活铁轨不支持弯轨
}

TEST(RailStateTest, FourConnectionsUnpowered)
{
    // 四连接（四个方向都有铁轨），无红石信号时优先选择SE弯轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(1, 0, 0, rail, RailShape::EastWest);    // 东方
    world.setRail(-1, 0, 0, rail, RailShape::EastWest);   // 西方
    world.setRail(0, 0, 1, rail, RailShape::NorthSouth);  // 南方
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth); // 北方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(false, false, RailShape::NorthSouth);

    // 无红石信号：SE是最后匹配的弯轨
    EXPECT_EQ(rail.getRailShape(result), RailShape::SouthEast);
}

TEST(RailStateTest, FourConnectionsPowered)
{
    // 四连接，有红石信号时优先选择NW弯轨
    RailTestWorld world;
    const RailBlock& rail = dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL);
    world.setRail(0, 0, 0, rail, RailShape::NorthSouth);
    world.setRail(1, 0, 0, rail, RailShape::EastWest);    // 东方
    world.setRail(-1, 0, 0, rail, RailShape::EastWest);   // 西方
    world.setRail(0, 0, 1, rail, RailShape::NorthSouth);  // 南方
    world.setRail(0, 0, -1, rail, RailShape::NorthSouth); // 北方

    RailState rstate(world, BlockPos(0, 0, 0), rail, *world.getBlockState(0, 0, 0));
    BlockState result = rstate.place(true, false, RailShape::NorthSouth);

    // 有红石信号：NW是最后匹配的弯轨
    EXPECT_EQ(rail.getRailShape(result), RailShape::NorthWest);
}
