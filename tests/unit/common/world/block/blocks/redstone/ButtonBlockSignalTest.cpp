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
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/redstone/AbstractButtonBlock.hpp"
#include "common/world/block/blocks/redstone/StoneButtonBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {
namespace test {

namespace {

/// 记录方块状态的最小测试世界
class ButtonTestWorld final : public ::mc::test::BaseTestWorld {
public:
    ButtonTestWorld()
    {
        m_tickManager = std::make_unique<world::tick::TickManager>(*this);
    }

    using IWorld::getBlockState;

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        return *m_tickManager;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        return *m_tickManager;
    }

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

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        setBlockState(pos.x, pos.y, pos.z, state);
    }

    /// 记录每一次邻居更新通知（位置 + 触发源方块）
    void updateNeighbors(const BlockPos& pos, Block& sourceBlock) override
    {
        m_updates.emplace_back(pos, &sourceBlock);
    }

    [[nodiscard]] bool wasUpdated(const BlockPos& pos) const
    {
        return std::any_of(m_updates.begin(), m_updates.end(), [&pos](const auto& entry) {
            return entry.first == pos;
        });
    }

    [[nodiscard]] std::size_t updateCount() const
    {
        return m_updates.size();
    }

private:
    std::vector<std::pair<BlockPos, Block*>> m_updates;
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    std::unique_ptr<world::tick::TickManager> m_tickManager;
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
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
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

// 原版 ButtonBlock.getSignal 只看 POWERED，**不区分方向**：按下后 15 级弱信号对六个方向都成立。
// 方向限制属于强信号（getDirectSignal）的语义，见 ButtonNeverProvidesStrongPower。
TEST_F(ButtonBlockSignalTest, PressedButtonOnlyPowersOutputSide)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;

    // 墙按钮朝北：输出方向为南，但弱信号不分方向
    const BlockState state = _button(Direction::North, AttachFace::Wall, true);

    for (const Direction dir : Directions::all()) {
        EXPECT_GT(button.getWeakPower(state, world, pos, dir), 0) << "side=" << static_cast<int>(dir);
    }
}

TEST_F(ButtonBlockSignalTest, FloorButtonOnlyPowersUpward)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;
    const BlockState state = _button(Direction::West, AttachFace::Floor, true);

    // 地板按钮的弱信号同样不分方向；方向差异只体现在强信号（输出侧为 Up）
    for (const Direction dir : Directions::all()) {
        EXPECT_EQ(button.getWeakPower(state, world, pos, dir), 15) << "side=" << static_cast<int>(dir);
    }
    EXPECT_EQ(button.getStrongPower(state, world, pos, Direction::Up), 15);
    EXPECT_EQ(button.getStrongPower(state, world, pos, Direction::Down), 0);
    EXPECT_EQ(button.getStrongPower(state, world, pos, Direction::West), 0);
    EXPECT_EQ(button.getStrongPower(state, world, pos, Direction::East), 0);
}

TEST_F(ButtonBlockSignalTest, UnpressedButtonEmitsNothing)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;
    const BlockState state = _button(Direction::North, AttachFace::Wall, false);

    EXPECT_EQ(button.getWeakPower(state, world, pos, Direction::South), 0);
}

// 原版 ButtonBlock.getDirectSignal 只在「连接方向」（getConnectedDirection，即输出方向）返回 15，
// 其余方向为 0——按钮并非「永不强充能」，只是强充能仅针对输出侧那一面。
TEST_F(ButtonBlockSignalTest, ButtonNeverProvidesStrongPower)
{
    ButtonTestWorld world;
    const BlockPos pos(0, 64, 0);
    const auto& button = *VanillaBlocks::STONE_BUTTON;
    // 墙按钮朝北 → 输出方向为南
    const BlockState state = _button(Direction::North, AttachFace::Wall, true);

    EXPECT_EQ(button.getStrongPower(state, world, pos, Direction::South), 15);
    for (const Direction dir : {Direction::North, Direction::East, Direction::West, Direction::Up, Direction::Down}) {
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

    // RedstonePower::getSignal 对应原版 Block.getSignal，查的是 ButtonBlock.getSignal，
    // 该实现只看 POWERED、不区分方向：六个方向都应读到 15。
    // 方向限制只体现在 getDirectSignal（强信号）上。
    for (const Direction dir : Directions::all()) {
        EXPECT_EQ(world::redstone::RedstonePower::getSignal(world, buttonPos, dir), 15)
            << "side=" << static_cast<int>(dir);
    }
}

// ========== 交互按下 ==========

TEST_F(ButtonBlockSignalTest, UseWithoutItem_PressesButtonAndSchedulesRelease)
{
    ButtonTestWorld world;
    const BlockPos buttonPos(0, 64, 0);
    const BlockState unpressed = _button(Direction::North, AttachFace::Wall, false);
    world.setBlockAt(buttonPos, &unpressed);

    Player player(EntityInstanceId{2}, std::string("Tester"), mc::test::testEcsRegistry());
    VanillaBlocks::STONE_BUTTON->useWithoutItem(unpressed, world, buttonPos, player, BlockRaycastResult{});

    // 交互入口必须真正把按钮置为按下（此前方块从未被任何交互路径按下）。
    EXPECT_TRUE(world.getBlockState(buttonPos)->get(BlockStateProperties::POWERED()));

    // 已按下时再次交互应被消费且不改变状态。
    const BlockActionResult second = VanillaBlocks::STONE_BUTTON->useWithoutItem(
        *world.getBlockState(buttonPos), world, buttonPos, player, BlockRaycastResult{});
    EXPECT_EQ(second.getType(), ActionResultType::Consume);
}

// ========== 按下/弹起时的邻居更新范围 ==========

TEST_F(ButtonBlockSignalTest, PressUpdatesFullNeighbourhoodAndSupport)
{
    ButtonTestWorld world;
    const BlockPos buttonPos(0, 64, 0);
    const BlockPos supportPos(0, 64, 1); // 墙按钮朝北，支撑在其南侧

    const BlockState unpressed = _button(Direction::North, AttachFace::Wall, false);
    world.setBlockAt(buttonPos, &unpressed);

    Player player(EntityInstanceId{1}, std::string("Tester"), mc::test::testEcsRegistry());
    const BlockActionResult result =
        VanillaBlocks::STONE_BUTTON->useWithoutItem(unpressed, world, buttonPos, player, BlockRaycastResult{});
    EXPECT_EQ(result.getType(), ActionResultType::Success);

    // 按钮自身位置与支撑方块位置各获得一次邻居更新发起（RedstoneSystem 再据此逐个通知
    // 六向邻居并调用其 neighborChanged）。此前实现只挑输出方向与附着侧两个方向直接调用。
    EXPECT_TRUE(world.wasUpdated(buttonPos));
    EXPECT_TRUE(world.wasUpdated(supportPos));
    EXPECT_TRUE(world.getBlockState(buttonPos)->get(BlockStateProperties::POWERED()));
}

} // namespace test
} // namespace blocks
} // namespace mc
