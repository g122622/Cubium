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
 * @file ButtonBlockSignalTest.cpp
 * @brief 按钮红石信号方向性单元测试
 *
 * 覆盖原版按钮语义：按下时只向附着面相反方向（输出方向）提供 15 强度弱信号，
 * 不提供强信号，其他方向输出 0。
 */

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/redstone/AbstractButtonBlock.hpp"
#include "common/world/block/blocks/redstone/StoneButtonBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace test {

namespace {

/// 记录方块状态的最小测试世界
class ButtonTestWorld final : public ::mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = state;
        }
        return true;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
};

using AttachFace = BlockStateProperties::AttachFace;

BlockState _button(Direction facing, AttachFace face, bool powered)
{
    return VanillaBlocks::STONE_BUTTON->defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::ATTACH_FACE(), face)
        .with(BlockStateProperties::POWERED(), powered);
}

} // namespace

class ButtonBlockSignalTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ========== 输出方向 ==========

TEST_F(ButtonBlockSignalTest, OutputDirectionForEachAttachFace)
{
    const auto& button = static_cast<const AbstractButtonBlock&>(*VanillaBlocks::STONE_BUTTON);

    // 墙按钮：输出到 FACING 的反方向（背靠附着方块）
    EXPECT_EQ(button.getOutputDirection(_button(Direction::North, AttachFace::Wall, false)), Direction::South);
    EXPECT_EQ(button.getOutputDirection(_button(Direction::East, AttachFace::Wall, false)), Direction::West);

    // 地板按钮：向上输出；天花板按钮：向下输出
    EXPECT_EQ(button.getOutputDirection(_button(Direction::North, AttachFace::Floor, false)), Direction::Up);
    EXPECT_EQ(button.getOutputDirection(_button(Direction::North, AttachFace::Ceiling, false)), Direction::Down);
}

// ========== 信号方向性 ==========

TEST_F(ButtonBlockSignalTest, PressedButtonOnlyPowersOutputSide)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;

    // 墙按钮朝北：输出方向为南
    const BlockState state = _button(Direction::North, AttachFace::Wall, true);

    EXPECT_GT(button.getWeakPower(state, world, pos, Direction::South), 0);
    for (const Direction dir : {Direction::North, Direction::East, Direction::West, Direction::Up, Direction::Down}) {
        EXPECT_EQ(button.getWeakPower(state, world, pos, dir), 0) << "side=" << static_cast<int>(dir);
    }
}

TEST_F(ButtonBlockSignalTest, FloorButtonOnlyPowersUpward)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;
    const BlockState state = _button(Direction::West, AttachFace::Floor, true);

    EXPECT_EQ(button.getWeakPower(state, world, pos, Direction::Up), 15);
    EXPECT_EQ(button.getWeakPower(state, world, pos, Direction::Down), 0);
    EXPECT_EQ(button.getWeakPower(state, world, pos, Direction::West), 0);
    EXPECT_EQ(button.getWeakPower(state, world, pos, Direction::East), 0);
}

TEST_F(ButtonBlockSignalTest, UnpressedButtonEmitsNothing)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;
    const BlockState state = _button(Direction::North, AttachFace::Wall, false);

    EXPECT_EQ(button.getWeakPower(state, world, pos, Direction::South), 0);
}

TEST_F(ButtonBlockSignalTest, ButtonNeverProvidesStrongPower)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;
    const BlockState state = _button(Direction::North, AttachFace::Wall, true);

    for (const Direction dir : Directions::all()) {
        EXPECT_EQ(button.getStrongPower(state, world, pos, dir), 0) << "side=" << static_cast<int>(dir);
    }
}

// ========== 经由红石系统的充能判定 ==========

TEST_F(ButtonBlockSignalTest, PoweredButtonPowersBlockOnOutputSideOnly)
{
    ButtonTestWorld world;
    const BlockPos buttonPos(0, 64, 0);

    // 墙按钮朝北，输出到南侧 (0,64,z+1)。
    const BlockState pressedButton = _button(Direction::North, AttachFace::Wall, true);
    world.setBlockAt(buttonPos, &pressedButton);

    // 直接按原版 Block.getSignal 语义查询：只有从输出侧（南侧方块看向北）能读到 15，
    // 背面（北侧方块看向南）读不到信号。
    EXPECT_EQ(world::redstone::RedstonePower::getSignal(world, buttonPos, Direction::South), 15);
    EXPECT_EQ(world::redstone::RedstonePower::getSignal(world, buttonPos, Direction::North), 0);
}

} // namespace test
} // namespace blocks
} // namespace mc