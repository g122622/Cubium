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

    // 背包分配顺序：先找可合并的同类未满槽（选中槽 → 副手槽 → 其余普通槽），
    // 再找第一个空槽（只扫 36 个普通槽位，副手不在其中）。
    // 选中槽已被泥土占住且泥土无法与剑合并，因此剑落入快捷栏第一个空槽（索引 1），
    // 不会进副手槽——副手槽只在"合并"阶段参与，不参与"找空槽"。
    const i32 selected = player->inventory().getSelectedSlot();
    EXPECT_LT(selected, mc::PlayerInventory::HOTBAR_SIZE);
    EXPECT_EQ(player->inventory().getItem(selected).getItem(), mc::Items::DIAMOND_SWORD)
        << "the newly added item must be the one now held in the selected hotbar slot";
    EXPECT_NE(selected, 0) << "slot 0 is occupied by dirt, so the sword must land in a different slot";
    EXPECT_TRUE(player->inventory().getItem(mc::InventorySlots::OFFHAND).isEmpty())
        << "offhand only participates in the merge pass, never in free-slot allocation";
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

// Instant 只定向一次；Continuous 每 tick 重新朝向目标（目标相对位置变化后仍跟随）。
TEST(SimulatedPlayerTest, LookAtBlockDurationInstantVsContinuous)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1011), std::string{"LookBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    mc::test::NullGameTestHelper helper;
    player->setHelper(helper);
    player->setPosition(0.0, 64.0, 0.0);

    // Instant：定向到目标方块中心，之后即使目标方向变了也不重新朝向。
    player->lookAtBlock(BlockPos{0, 64, 10}, mc::test::SimulatedPlayer::LookDuration::Instant);
    const f32 instantYaw = player->yaw();
    player->setPosition(10.0, 64.0, 0.0);
    player->tick();
    EXPECT_FLOAT_EQ(player->yaw(), instantYaw) << "Instant must not reorient on tick";

    // Continuous：换位后 tick() 必须把朝向重新指向目标（yaw 发生变化）。
    player->lookAtBlock(BlockPos{0, 64, 10}, mc::test::SimulatedPlayer::LookDuration::Continuous);
    player->setPosition(10.0, 64.0, 0.0);
    const f32 beforeTick = player->yaw();
    player->setPosition(-10.0, 64.0, 0.0);
    player->tick();
    EXPECT_NE(player->yaw(), beforeTick) << "Continuous must reorient on tick";
}

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
    player->moveToLocation(BlockPos{0, 64, 10}, 1.0f, true);
    EXPECT_FLOAT_EQ(player->movementInputForward(), 1.0f);
    EXPECT_FLOAT_EQ(player->pitch(), 0.0f); // 移动只改 yaw，不改 pitch

    player->moveToLocation(BlockPos{0, 64, 10}, 0.5f, true);
    EXPECT_FLOAT_EQ(player->movementInputForward(), 0.5f);

    // 负倍率：输入方向反转（后退）。
    player->moveToLocation(BlockPos{0, 64, 10}, -1.0f, true);
    EXPECT_FLOAT_EQ(player->movementInputForward(), -1.0f);
}

// faceTarget=false 时必须保持原朝向只平移（基岩 MoveToOptions.faceTarget）。
TEST(SimulatedPlayerTest, MoveToLocation_FaceTargetFalseKeepsRotation)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1010), std::string{"LookAwayBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    mc::test::NullGameTestHelper helper;
    player->setHelper(helper);
    player->setPosition(0.0, 64.0, 0.0);

    // 先固定一个与目标方向不同的朝向，再朝 +Z 走。
    player->setRotation(123.0f, 0.0f);
    player->moveToLocation(BlockPos{0, 64, 10}, 1.0f, false);
    EXPECT_FLOAT_EQ(player->yaw(), 123.0f);
    EXPECT_FLOAT_EQ(player->movementInputForward(), 1.0f);

    // 对照：faceTarget=true 会把 yaw 转向目标。
    player->moveToLocation(BlockPos{0, 64, 10}, 1.0f, true);
    // 目标方块中心在 (0.5, 0.5)，玩家在原点，故朝向并非纯 +Z，而是 atan2(-0.5, 10.5)。
    EXPECT_NEAR(player->yaw(), -2.7263f, 0.01f);
    EXPECT_NE(player->yaw(), 123.0f) << "faceTarget=true must reorient the player";
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

// fly 受 abilities.canFly 门控；stopFlying/swim/stopSwimming 直接改状态。
// move 的方向相对 GameTest 结构：玩家朝向不同时，同一个世界方向要分解成不同的前进/横移输入。
// moveRelative 直接映射输入；rotateBody/setBodyRotation 更新 yaw。
TEST(SimulatedPlayerTest, MoveDecomposesWorldDirectionAndRotateBodyUpdatesYaw)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1013), std::string{"DirBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);
    mc::test::NullGameTestHelper helper;
    player->setHelper(helper);
    player->setPosition(0.0, 64.0, 0.0);

    // yaw=0 看向 +Z：朝南（northSouth=+1）就是纯前进。
    player->setRotation(0.0f, 0.0f);
    player->move(0.0f, 1.0f, 1.0f);
    EXPECT_NEAR(player->movementInputForward(), 1.0f, 0.01f);
    EXPECT_NEAR(player->movementInputStrafe(), 0.0f, 0.01f);

    // 朝东（westEast=+1）在 yaw=0 下是纯右横移。
    player->move(1.0f, 0.0f, 1.0f);
    EXPECT_NEAR(player->movementInputForward(), 0.0f, 0.01f);
    EXPECT_NEAR(player->movementInputStrafe(), 1.0f, 0.01f);

    // moveRelative 直接映射：leftRight=横移、backwardForward=前进。
    player->moveRelative(0.5f, -1.0f, 1.0f);
    EXPECT_NEAR(player->movementInputForward(), -1.0f, 0.01f);
    EXPECT_NEAR(player->movementInputStrafe(), 0.5f, 0.01f);

    // stopMoving 清零输入。
    player->stopMoving();
    EXPECT_NEAR(player->movementInputForward(), 0.0f, 0.01f);
    EXPECT_NEAR(player->movementInputStrafe(), 0.0f, 0.01f);

    // rotateBody 相对当前朝向累加；setBodyRotation 设为绝对值。
    player->setRotation(10.0f, 0.0f);
    player->rotateBody(25.0f);
    EXPECT_NEAR(player->yaw(), 35.0f, 0.01f);
    player->setBodyRotation(120.0f);
    EXPECT_NEAR(player->yaw(), 120.0f, 0.01f);
}

TEST(SimulatedPlayerTest, FlyRespectsAbilityGateAndSwimTogglesState)
{
    mc::Items::ensureInitialized();
    auto player = std::make_unique<mc::test::SimulatedPlayer>(
        static_cast<mc::EntityInstanceId>(1012), std::string{"FlyBot"}, mc::test::testEcsRegistry());
    ASSERT_NE(player, nullptr);

    // 默认无飞行能力：fly() 必须不生效。
    ASSERT_FALSE(player->abilities().canFly);
    player->fly();
    EXPECT_FALSE(player->isFlying()) << "fly must be gated by abilities.canFly";

    // 授予飞行能力后可起飞，stopFlying 可收回。
    player->abilities().canFly = true;
    player->fly();
    EXPECT_TRUE(player->isFlying());
    player->stopFlying();
    EXPECT_FALSE(player->isFlying());

    // 游泳姿态可双向切换。
    player->swim();
    EXPECT_TRUE(player->isSwimming());
    player->stopSwimming();
    EXPECT_FALSE(player->isSwimming());
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
