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

// 批 1/2/17/20：玩家移动 + 动作/交互 + 告示牌 + 载具/交互 C→S Java wire codec 往返。
// 表级往返（roundTripGeneric）同时验证 packetID 分发 + payload codec。各包 (id, flow, altIndex)
// 映射取自 JavaProtocolTables.cpp buildPlaySb/buildPlayCb。altIndex 取自 IrPacket.hpp variant
// 声明顺序。MovePlayer 四变体共享 MovePlayerFlags（bit0=onGround, bit1=horizontalCollision），
// 位打包往返精确。Interact 按 action 条件编 hand/hitVec（action 0/2），故每 action 一例。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;
using namespace mc;

namespace {

// 构造一个全置位的 MovePlayerFlags，验证两位都能往返（非默认值才显出 codec bug）。
MovePlayerFlags makeFlagsBoth()
{
    return MovePlayerFlags{true, true};
}

} // namespace

// ============================================================================
// 批 1：通用 + 玩家移动 C→S（KeepAlive/MovePlayer 四变体）
// KeepAlive: altIndex 5（Sb id=27）。MovePlayerPos/PosRot/Rot/StatusOnly: 7/8/9/10。
// ============================================================================

TEST_F(NetworkTestBase, PlayKeepAliveRoundtrip)
{
    play::KeepAlive in{};
    in.id = 0;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 5u);
    EXPECT_EQ(std::get<play::KeepAlive>(out), in);
}

TEST_F(NetworkTestBase, PlayKeepAliveMaxId)
{
    play::KeepAlive in{};
    in.id = static_cast<i64>(0x7FFFFFFFFFFFFFFFLL);
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 5u);
    EXPECT_EQ(std::get<play::KeepAlive>(out), in);
}

TEST_F(NetworkTestBase, PlayKeepAliveNegativeId)
{
    play::KeepAlive in{};
    in.id = -1;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 5u);
    EXPECT_EQ(std::get<play::KeepAlive>(out), in);
}

TEST_F(NetworkTestBase, PlayMovePlayerPosZero)
{
    play::MovePlayerPos in{};
    in.x = 0.0;
    in.y = 0.0;
    in.z = 0.0;
    in.flags = MovePlayerFlags{false, false};
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 7u);
    EXPECT_EQ(std::get<play::MovePlayerPos>(out), in);
}

TEST_F(NetworkTestBase, PlayMovePlayerPosNegative)
{
    play::MovePlayerPos in{};
    in.x = -123.5;
    in.y = 64.0;
    in.z = -77.25;
    in.flags = makeFlagsBoth();
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 7u);
    EXPECT_EQ(std::get<play::MovePlayerPos>(out), in);
}

TEST_F(NetworkTestBase, PlayMovePlayerPosRot)
{
    play::MovePlayerPosRot in{};
    in.x = 1.5;
    in.y = 70.0;
    in.z = 2.5;
    in.yRot = 90.0f;
    in.xRot = -45.0f;
    in.flags = MovePlayerFlags{true, false};
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 8u);
    EXPECT_EQ(std::get<play::MovePlayerPosRot>(out), in);
}

TEST_F(NetworkTestBase, PlayMovePlayerRot)
{
    play::MovePlayerRot in{};
    in.yRot = 180.0f;
    in.xRot = 0.0f;
    in.flags = makeFlagsBoth();
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 9u);
    EXPECT_EQ(std::get<play::MovePlayerRot>(out), in);
}

TEST_F(NetworkTestBase, PlayMovePlayerStatusOnly)
{
    play::MovePlayerStatusOnly in{};
    in.flags = MovePlayerFlags{true, true};
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 10u);
    EXPECT_EQ(std::get<play::MovePlayerStatusOnly>(out), in);
}

TEST_F(NetworkTestBase, PlayAcceptTeleportation)
{
    play::AcceptTeleportation in{};
    in.teleportId = 42;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 0u);
    EXPECT_EQ(std::get<play::AcceptTeleportation>(out), in);
}

// ============================================================================
// 批 2：动作/交互 C→S（Chat/PlayerAction/UseItemOn/UseItem）
// Chat: altIndex 4（Sb id=8）。PlayerAction: 11（id=40）。UseItemOn: 15（id=63）。
// UseItem: 14（id=64）。
// ============================================================================

TEST_F(NetworkTestBase, PlayChatShort)
{
    play::Chat in{};
    in.message = "hi";
    in.timestamp = 0;
    in.salt = 0;
    in.signature = std::nullopt;
    in.lastSeenOffset = 0;
    in.lastSeenAcknowledged = {0, 0, 0};
    in.lastSeenChecksum = 0;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 4u);
    EXPECT_EQ(std::get<play::Chat>(out), in);
}

TEST_F(NetworkTestBase, PlayChatWithSignature)
{
    play::Chat in{};
    in.message = "signed message";
    in.timestamp = 1700000000000LL;
    in.salt = 0xCAFEBABE;
    in.signature = std::vector<u8>(256, 0xAB);
    in.lastSeenOffset = 5;
    in.lastSeenAcknowledged = {0x01, 0x02, 0x03};
    in.lastSeenChecksum = 0x7F;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 4u);
    EXPECT_EQ(std::get<play::Chat>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerAction)
{
    play::PlayerAction in{};
    in.action = 0; // START_DESTROY_BLOCK
    in.blockPosPacked = 0x123456789ABCDEF0LL;
    in.direction = 1; // Direction ordinal
    in.sequence = 7;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 11u);
    EXPECT_EQ(std::get<play::PlayerAction>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerActionDropItem)
{
    play::PlayerAction in{};
    in.action = 4; // DROP_ITEM
    in.blockPosPacked = 0;
    in.direction = 255; // 验证 direction 写 U8 读 i32 的边界（0..255 精确）
    in.sequence = 100;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 11u);
    EXPECT_EQ(std::get<play::PlayerAction>(out), in);
}

TEST_F(NetworkTestBase, PlayUseItemOnZeroHit)
{
    play::UseItemOn in{};
    in.hand = 0; // MAIN_HAND
    in.blockHit.blockPosPacked = 0;
    in.blockHit.direction = 0;
    in.blockHit.hitX = 0.0f;
    in.blockHit.hitY = 0.0f;
    in.blockHit.hitZ = 0.0f;
    in.blockHit.inside = false;
    in.blockHit.worldBorderHit = false;
    in.sequence = 0;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 15u);
    EXPECT_EQ(std::get<play::UseItemOn>(out), in);
}

TEST_F(NetworkTestBase, PlayUseItemOnFullHit)
{
    play::UseItemOn in{};
    in.hand = 1; // OFF_HAND
    in.blockHit.blockPosPacked = -100LL;
    in.blockHit.direction = 3;
    in.blockHit.hitX = 0.5f;
    in.blockHit.hitY = 0.25f;
    in.blockHit.hitZ = 0.75f;
    in.blockHit.inside = true;
    in.blockHit.worldBorderHit = true;
    in.sequence = 99;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 15u);
    EXPECT_EQ(std::get<play::UseItemOn>(out), in);
}

TEST_F(NetworkTestBase, PlayUseItem)
{
    play::UseItem in{};
    in.hand = 0;
    in.sequence = 3;
    in.yRot = 12.5f;
    in.xRot = -3.25f;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 14u);
    EXPECT_EQ(std::get<play::UseItem>(out), in);
}

// ============================================================================
// 批 17：告示牌 mixed（OpenSignEditor S→C + SignUpdate C→S）
// OpenSignEditor: altIndex 68（Cb id=58）。SignUpdate: 69（Sb id=59）。
// ============================================================================

TEST_F(NetworkTestBase, PlayOpenSignEditor)
{
    play::OpenSignEditor in{};
    in.blockPosPacked = 0xABCDEFLL;
    in.isFrontText = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 68u);
    EXPECT_EQ(std::get<play::OpenSignEditor>(out), in);
}

TEST_F(NetworkTestBase, PlaySignUpdateBackText)
{
    play::SignUpdate in{};
    in.blockPosPacked = 0x100LL;
    in.isFrontText = false;
    in.lines = {"line0", "line1", "line2", "line3"};
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 69u);
    EXPECT_EQ(std::get<play::SignUpdate>(out), in);
}

// ============================================================================
// 批 20：载具/交互 C→S（ServerboundMoveVehicle + PaddleBoat + Interact 3 action）
// ServerboundMoveVehicle: altIndex 83（Sb id=33）。PaddleBoat: 85（id=34）。
// Interact: 86（id=25）。
// 注：ClientboundMoveVehicle（altIndex 84）放在此文件一并测，因结构同源。
// ============================================================================

TEST_F(NetworkTestBase, PlayServerboundMoveVehicle)
{
    play::ServerboundMoveVehicle in{};
    in.x = 10.5;
    in.y = 64.0;
    in.z = -20.25;
    in.yRot = 45.0f;
    in.xRot = 0.0f;
    in.onGround = true;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 83u);
    EXPECT_EQ(std::get<play::ServerboundMoveVehicle>(out), in);
}

TEST_F(NetworkTestBase, PlayClientboundMoveVehicle)
{
    play::ClientboundMoveVehicle in{};
    in.x = -1.5;
    in.y = 100.0;
    in.z = 200.75;
    in.yRot = 180.0f;
    in.xRot = 90.0f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 84u);
    EXPECT_EQ(std::get<play::ClientboundMoveVehicle>(out), in);
}

TEST_F(NetworkTestBase, PlayPaddleBoat)
{
    play::PaddleBoat in{};
    in.left = true;
    in.right = false;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 85u);
    EXPECT_EQ(std::get<play::PaddleBoat>(out), in);
}

TEST_F(NetworkTestBase, PlayPaddleBoatBoth)
{
    play::PaddleBoat in{};
    in.left = true;
    in.right = true;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 85u);
    EXPECT_EQ(std::get<play::PaddleBoat>(out), in);
}

// Interact action=0 (INTERACT)：写 hand，不写 hitVec。
TEST_F(NetworkTestBase, PlayInteractAt)
{
    play::Interact in{};
    in.entityId = 42;
    in.action = 0; // INTERACT
    in.hand = 0;   // MAIN_HAND
    in.usingSecondaryAction = false;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 86u);
    EXPECT_EQ(std::get<play::Interact>(out), in);
}

// Interact action=1 (ATTACK)：既不写 hand 也不写 hitVec，hand/hitVec 须保持默认。
TEST_F(NetworkTestBase, PlayInteractAttack)
{
    play::Interact in{};
    in.entityId = 7;
    in.action = 1; // ATTACK
    in.usingSecondaryAction = true;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 86u);
    EXPECT_EQ(std::get<play::Interact>(out), in);
}

// Interact action=2 (INTERACT_AT)：写 hand + hitVec。
TEST_F(NetworkTestBase, PlayInteractAtWithHitVec)
{
    play::Interact in{};
    in.entityId = 100;
    in.action = 2; // INTERACT_AT
    in.hand = 1;   // OFF_HAND
    in.hitX = 0.5f;
    in.hitY = 1.0f;
    in.hitZ = 1.5f;
    in.usingSecondaryAction = false;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 86u);
    EXPECT_EQ(std::get<play::Interact>(out), in);
}
