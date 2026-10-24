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

// SimulatedPlayer 单元测试（最小可用集，不 spawn 进真实世界）。
//
// SimulatedPlayer 是 ServerPlayer 子类，构造 ctor 不需世界指针。完整 spawn + 移动路径需
// GameTestHelper 绑定的真实 ServerWorld（由 test_gametest_server.cpp 端到端覆盖）。
// 本测试验证：
//   - 公开 ctor 不崩溃
//   - setHelper/helper 回指 getter 对称
//   - TODO stub 方法（flyToLocation）调用不崩溃（占位实现）
//   - giveItem/setItem 的 selectSlot 语义与槽位越界判定
//   - moveToLocation 的 speed 倍率体现在移动输入
//   - jump 返回是否真的起跳
//
// 注：构造的 SimulatedPlayer 不挂世界，故仅测元数据 + 回指，不测 moveToLocation（会经
// m_helper->worldBlockPosition 触发，NullGameTestHelper 下虽可走通但语义无意义，留端到端测）。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/Vector3.hpp"
#include "server/test/framework/helper/NullGameTestHelper.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp"

#include <memory>
#include <string>

using mc::BlockPos; // BlockPos 属 mc::（非 mc::test），测试内简写
using mc::f32;
using mc::i32;

TEST(SimulatedPlayerTest, ConstructWithIdAndName)
{
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1001), std::string{"TestBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->helper(), nullptr);
}

TEST(SimulatedPlayerTest, SetHelperRoundTrip)
{
    mc::test::NullGameTestHelper helper;
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1002), std::string{"HelperBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);

    player->setHelper(helper);
    EXPECT_EQ(player->helper(), &helper);
}

TEST(SimulatedPlayerTest, TodoStubsDoNotCrash)
{
    // flyToLocation/attack 为 TODO stub（占位实现），验证调用不崩溃。
    // TODO: 待原生侧实现飞行/攻击后改为行为断言。
    mc::test::NullGameTestHelper helper;
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1003), std::string{"StubBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    player->setHelper(helper);

    player->flyToLocation(BlockPos{0, 0, 0}, 1.0f);
    SUCCEED();
}

// ============================================================================
// 背包便捷方法：selectSlot 与越界判定
// ============================================================================

TEST(SimulatedPlayerTest, GiveItem_SelectSlotPicksSlotReceivingItem)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1004), std::string{"GiveBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    // 先占住选中槽（不改变选中），确保物品只能落到新的空槽，选中槽必须被改写。
    ASSERT_TRUE(player->setItem(mc::ItemStack(mc::Items::DIRT, 1), 0, false));
    player->inventory().setSelectedSlot(0);

    EXPECT_TRUE(player->giveItem(mc::ItemStack(mc::Items::DIAMOND_SWORD, 1), true));

    // 背包分配顺序为 选中槽 → 副手 → 快捷栏 → 背包：选中槽被占用时新物品落入副手槽；
    // 副手槽在快捷栏之外，故 selectSlot 钳制到快捷栏范围（选中槽仍为持有物品的 0）。
    const i32 offhandSlot = mc::InventorySlots::OFFHAND;
    EXPECT_EQ(player->inventory().getItem(offhandSlot).getItem(), mc::Items::DIAMOND_SWORD);
    const i32 selected = player->inventory().getSelectedSlot();
    EXPECT_LT(selected, mc::PlayerInventory::HOTBAR_SIZE);
}

TEST(SimulatedPlayerTest, GiveItem_MergeIntoExistingStack_SelectsThatSlot)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1005), std::string{"MergeBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);

    // 石头只放在快捷栏最后一格：背包其它槽为空，add 会先把石头合并进该格而非另开空槽。
    constexpr i32 kStoneSlot = mc::PlayerInventory::HOTBAR_SIZE - 1;
    ASSERT_TRUE(player->setItem(mc::ItemStack(mc::Items::STONE, 1), kStoneSlot, false));
    ASSERT_TRUE(player->setItem(mc::ItemStack(mc::Items::DIRT, 1), 0, false));
    player->inventory().setSelectedSlot(0);

    ASSERT_TRUE(player->giveItem(mc::ItemStack(mc::Items::STONE, 1), true));
    EXPECT_EQ(player->inventory().getSelectedSlot(), kStoneSlot);
    EXPECT_EQ(player->inventory().getItem(kStoneSlot).getCount(), 2);
}

TEST(SimulatedPlayerTest, GiveItem_WithoutSelectSlot_KeepsSelection)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1006), std::string{"NoSelectBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    player->inventory().setSelectedSlot(8);

    EXPECT_TRUE(player->giveItem(mc::ItemStack(mc::Items::DIRT, 1), false));
    EXPECT_EQ(player->inventory().getSelectedSlot(), 8);
}

TEST(SimulatedPlayerTest, SetItem_OutOfRangeReturnsFalse)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1007), std::string{"RangeBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);

    EXPECT_FALSE(player->setItem(mc::ItemStack(mc::Items::DIRT, 1), -1, false));
    EXPECT_FALSE(player->setItem(mc::ItemStack(mc::Items::DIRT, 1), mc::PlayerInventory::TOTAL_SIZE, false));
}

TEST(SimulatedPlayerTest, SetItem_SelectSlotOnlyForHotbar)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1008), std::string{"SlotBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    player->inventory().setSelectedSlot(0);

    // 快捷栏槽位：写入并选中。
    ASSERT_TRUE(player->setItem(mc::ItemStack(mc::Items::IRON_SWORD, 1), 3, true));
    EXPECT_EQ(player->inventory().getSelectedSlot(), 3);
    EXPECT_EQ(player->inventory().getItem(3).getItem(), mc::Items::IRON_SWORD);

    // 背包槽位：写入但不可选中（选中槽保持 3）。
    ASSERT_TRUE(player->setItem(mc::ItemStack(mc::Items::GOLDEN_SWORD, 1), mc::PlayerInventory::HOTBAR_SIZE, true));
    EXPECT_EQ(player->inventory().getSelectedSlot(), 3);
    EXPECT_EQ(player->inventory().getItem(mc::PlayerInventory::HOTBAR_SIZE).getItem(), mc::Items::GOLDEN_SWORD);
}

// ============================================================================
// 移动输入：speed 倍率
// ============================================================================

TEST(SimulatedPlayerTest, MoveToLocation_SpeedScalesMovementInput)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1009), std::string{"WalkBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    mc::test::NullGameTestHelper helper;
    player->setHelper(helper);
    player->setPosition(0.0, 64.0, 0.0);

    // 目标在 +Z 方向 10 格。speed 作为移动输入倍率传入 Player::handleMovementInput：
    // 1.0 → 前进输入 1；0.5 → 0.5；负值 → 输入方向反转（后退）。
    player->moveToLocation(BlockPos{0, 64, 10}, 1.0f);
    EXPECT_FLOAT_EQ(player->movementInputForward(), 1.0f);
    EXPECT_FLOAT_EQ(player->pitch(), 0.0f); // 移动只改 yaw，不改 pitch

    player->moveToLocation(BlockPos{0, 64, 10}, 0.5f);
    EXPECT_FLOAT_EQ(player->movementInputForward(), 0.5f);

    // 负倍率：输入方向反转（后退）。
    player->moveToLocation(BlockPos{0, 64, 10}, -1.0f);
    EXPECT_FLOAT_EQ(player->movementInputForward(), -1.0f);
}

// ============================================================================
// 跳跃返回值
// ============================================================================

// ============================================================================
// 飞行：flyToLocation
// ============================================================================

TEST(SimulatedPlayerTest, FlyToLocation_IgnoredWithoutFlightAbility)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1011), std::string{"SurvivalBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    mc::test::NullGameTestHelper helper;
    player->setHelper(helper);
    player->setGameMode(mc::GameMode::Survival);

    ASSERT_FALSE(player->canFly());
    player->flyToLocation(BlockPos{0, 70, 10}, 1.0f);
    // 无飞行能力时不开飞行、不产生移动输入。
    EXPECT_FALSE(player->isFlying());
    EXPECT_FLOAT_EQ(player->movementInputForward(), 0.0f);
}

TEST(SimulatedPlayerTest, FlyToLocation_CreativeEntersFlightTowardTarget)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1012), std::string{"FlyBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    mc::test::NullGameTestHelper helper;
    player->setHelper(helper);
    player->setGameMode(mc::GameMode::Creative);
    player->setPosition(0.5, 64.0, 0.5);

    ASSERT_TRUE(player->canFly());
    // 目标在 +Z 且更高：应开飞行、向前并有上升输入。
    player->flyToLocation(BlockPos{0, 70, 10}, 1.0f);
    EXPECT_TRUE(player->isFlying());
    EXPECT_FLOAT_EQ(player->movementInputForward(), 1.0f);

    // 已到达目标附近：直接返回，不再改写移动输入（缓存保持上一轮的值），仍保持飞行状态。
    player->setPosition(0.5, 70.5, 10.5);
    player->flyToLocation(BlockPos{0, 70, 10}, 1.0f);
    EXPECT_FLOAT_EQ(player->movementInputForward(), 1.0f);
    EXPECT_TRUE(player->isFlying());
}

TEST(SimulatedPlayerTest, Jump_ReportsWhetherJumpHappened)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1010), std::string{"JumpBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    player->setPosition(0.0, 64.0, 0.0);

    // 离地时不会起跳；落地后跳一次成功，紧接着的第二次被跳跃冷却拒绝。
    player->setOnGround(false);
    EXPECT_FALSE(player->jump());
    player->setOnGround(true);
    EXPECT_TRUE(player->jump());
    player->setOnGround(true);
    EXPECT_FALSE(player->jump());
}
