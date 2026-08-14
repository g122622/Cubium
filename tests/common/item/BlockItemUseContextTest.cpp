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
 * @file BlockItemUseContextTest.cpp
 * @brief BlockItemUseContext 单元测试
 *
 * 重点验证 getNearestLookingDirections 在 player==nullptr 时
 * 使用构造参数 playerYaw / playerPitch 的行为，覆盖仰视/俯视放置场景。
 *
 * 与 MC 1.21.11 Direction.orderedByNearest(Entity) 对齐：
 *   pitch > 0  → 俯视，Down 排在 Up 之前
 *   pitch < 0  → 仰视，Up 排在 Down 之前
 *   pitch == 0 → 水平视线，Y 轴方向由 f7 与 f9/f10 比较（此处恒为水平，
 *                不会将 Y 轴方向提前，但仍会出现在数组后段）
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

#include <set>

namespace mc {
namespace {

/// 测试世界：所有位置均为空气，允许放置判断返回可替换
class ContextTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
};

/// 构造一个最小化的 BlockItemUseContext，仅用于 getNearestLookingDirections 测试
BlockItemUseContext makeContext(f32 playerYaw, f32 playerPitch, Direction clickedFace)
{
    static ContextTestWorld world;
    static const ItemStack EMPTY_STACK = ItemStack::EMPTY;
    const BlockPos pos(0, 64, 0);
    const Vector3 hitPos(0.5f, 64.5f, 0.5f);
    return BlockItemUseContext(world, nullptr, EMPTY_STACK, hitPos, pos, clickedFace, playerYaw, playerPitch);
}

/// 在方向数组中查找指定 Direction 的下标，找不到返回 -1
int indexOf(const std::vector<Direction>& dirs, Direction target)
{
    for (std::size_t i = 0; i < dirs.size(); ++i) {
        if (dirs[i] == target) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace

// ============================================================================
// playerPitch 透传测试
// ============================================================================

TEST(BlockItemUseContextTest, GetPlayerPitch_ReturnsConstructedValue)
{
    auto context = makeContext(0.0f, 45.0f, Direction::Up);
    EXPECT_FLOAT_EQ(context.getPlayerPitch(), 45.0f);
}

TEST(BlockItemUseContextTest, GetPlayerPitch_NegativeValue_Preserved)
{
    auto context = makeContext(0.0f, -30.0f, Direction::Up);
    EXPECT_FLOAT_EQ(context.getPlayerPitch(), -30.0f);
}

TEST(BlockItemUseContextTest, GetPlayerPitch_Zero_Preserved)
{
    auto context = makeContext(90.0f, 0.0f, Direction::Up);
    EXPECT_FLOAT_EQ(context.getPlayerPitch(), 0.0f);
}

// ============================================================================
// getNearestLookingDirections - pitch 影响垂直方向优先级
// ============================================================================

TEST(BlockItemUseContextTest, LookingDirections_LookingDown_PrioritizesDownOverUp)
{
    // pitch=80（强俯视）→ Down 应出现在 Up 之前
    // clickedFace=Up，replacingClickedBlock 在空气位置为 true，直接返回 orderedByNearest 结果
    auto context = makeContext(0.0f, 80.0f, Direction::Up);
    const auto dirs = context.getNearestLookingDirections();

    const int downIdx = indexOf(dirs, Direction::Down);
    const int upIdx = indexOf(dirs, Direction::Up);
    ASSERT_GE(downIdx, 0);
    ASSERT_GE(upIdx, 0);
    EXPECT_LT(downIdx, upIdx) << "Looking down (pitch>0): Down should come before Up";
}

TEST(BlockItemUseContextTest, LookingDirections_LookingUp_PrioritizesUpOverDown)
{
    // pitch=-80（强仰视）→ Up 应出现在 Down 之前
    auto context = makeContext(0.0f, -80.0f, Direction::Up);
    const auto dirs = context.getNearestLookingDirections();

    const int downIdx = indexOf(dirs, Direction::Down);
    const int upIdx = indexOf(dirs, Direction::Up);
    ASSERT_GE(downIdx, 0);
    ASSERT_GE(upIdx, 0);
    EXPECT_LT(upIdx, downIdx) << "Looking up (pitch<0): Up should come before Down";
}

TEST(BlockItemUseContextTest, LookingDirections_Horizontal_YAxisDirectionsLater)
{
    // pitch=0，yaw=0（朝南）→ 水平视线，Y 轴方向不应排在首位
    auto context = makeContext(0.0f, 0.0f, Direction::Up);
    const auto dirs = context.getNearestLookingDirections();

    // 水平视线时，首位应是水平方向（South/North/East/West 之一），而非 Up/Down
    EXPECT_NE(dirs.front(), Direction::Up);
    EXPECT_NE(dirs.front(), Direction::Down);
}

// ============================================================================
// getNearestLookingDirections - clickedFace 影响（非替换模式）
// ============================================================================

TEST(BlockItemUseContextTest, LookingDirections_NonReplacing_PutsOppositeFaceFirst)
{
    // clickedFace=North 且空气位置不可替换（replacingClickedBlock=false）时，
    // orderedByNearest 数组会被调整，使 South（North 的反方向）提到首位
    // 但本测试世界 getBlockState 返回 nullptr → _canReplace 返回 true → replacing=true
    // 因此此用例验证替换模式下首位为视线方向
    auto context = makeContext(0.0f, 80.0f, Direction::North);
    const auto dirs = context.getNearestLookingDirections();
    EXPECT_EQ(dirs.size(), 6u);
    // 替换模式下不重排，首位取决于 pitch：强俯视 → Down
    EXPECT_EQ(dirs.front(), Direction::Down);
}

// ============================================================================
// 与 MC 1.21.11 Direction.orderedByNearest 一致性测试
// ============================================================================

TEST(BlockItemUseContextTest, LookingDirections_ArraySize_AlwaysSix)
{
    for (f32 yaw : {0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 270.0f, -45.0f}) {
        for (f32 pitch : {0.0f, 30.0f, -30.0f, 89.0f, -89.0f}) {
            auto context = makeContext(yaw, pitch, Direction::Up);
            const auto dirs = context.getNearestLookingDirections();
            EXPECT_EQ(dirs.size(), 6u) << "yaw=" << yaw << " pitch=" << pitch;
        }
    }
}

TEST(BlockItemUseContextTest, LookingDirections_AllSixDirectionsPresent)
{
    auto context = makeContext(45.0f, -45.0f, Direction::Up);
    const auto dirs = context.getNearestLookingDirections();

    std::set<Direction> uniqueDirs(dirs.begin(), dirs.end());
    EXPECT_EQ(uniqueDirs.size(), 6u);
    EXPECT_NE(uniqueDirs.find(Direction::Down), uniqueDirs.end());
    EXPECT_NE(uniqueDirs.find(Direction::Up), uniqueDirs.end());
    EXPECT_NE(uniqueDirs.find(Direction::North), uniqueDirs.end());
    EXPECT_NE(uniqueDirs.find(Direction::South), uniqueDirs.end());
    EXPECT_NE(uniqueDirs.find(Direction::West), uniqueDirs.end());
    EXPECT_NE(uniqueDirs.find(Direction::East), uniqueDirs.end());
}

TEST(BlockItemUseContextTest, LookingDirections_OppositePairOrdering)
{
    // orderedByNearest 第 i 个与第 5-i 个互为相反方向
    auto context = makeContext(30.0f, -20.0f, Direction::Up);
    const auto dirs = context.getNearestLookingDirections();
    ASSERT_EQ(dirs.size(), 6u);

    for (std::size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(dirs[i], Directions::opposite(dirs[5 - i]))
            << "Index pair (" << i << ", " << (5 - i) << ") should be opposites";
    }
}

// ============================================================================
// Yaw 影响水平方向优先级
// ============================================================================

TEST(BlockItemUseContextTest, LookingDirections_YawZero_PrioritizesSouth)
{
    // yaw=0（朝南），pitch=0（水平）→ South 应出现在 North 之前
    auto context = makeContext(0.0f, 0.0f, Direction::Up);
    const auto dirs = context.getNearestLookingDirections();

    const int southIdx = indexOf(dirs, Direction::South);
    const int northIdx = indexOf(dirs, Direction::North);
    ASSERT_GE(southIdx, 0);
    ASSERT_GE(northIdx, 0);
    EXPECT_LT(southIdx, northIdx);
}

TEST(BlockItemUseContextTest, LookingDirections_Yaw90_PrioritizesWest)
{
    // yaw=90（朝西），pitch=0 → West 应出现在 East 之前
    auto context = makeContext(90.0f, 0.0f, Direction::Up);
    const auto dirs = context.getNearestLookingDirections();

    const int westIdx = indexOf(dirs, Direction::West);
    const int eastIdx = indexOf(dirs, Direction::East);
    ASSERT_GE(westIdx, 0);
    ASSERT_GE(eastIdx, 0);
    EXPECT_LT(westIdx, eastIdx);
}

} // namespace mc
