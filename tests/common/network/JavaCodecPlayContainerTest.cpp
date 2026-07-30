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

// 批 3/7：容器交互 C→S + 容器同步 S→C Java wire codec 往返。
// 表级往返（roundTripGeneric）兼测 packetID 分发 + payload codec。altIndex 取自
// IrPacket.hpp variant 顺序：ContainerClick=2/ContainerClose=3（Sb）；
// ContainerSetContent=39/ContainerSetSlot=40/OpenScreen=41/ContainerSetData=42（Cb）。
// ItemStackView 空(count<=0→VarInt 0)/非空(count+itemId+DataComponentPatch wire)；
// patch 无外层长度前缀，以自身 VarInt(addedCount)+VarInt(removedCount) 自终止（空 patch=0x00 0x00）；
// HashedStack present=false(Bool false)/true(Bool+itemId+count+空 HashedPatchMap)。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;
using namespace mc;

namespace {

// 构造一个空 ItemStackView（count<=0）。codec 写 VarInt(0)，解码回全默认。
ItemStackView emptyStack()
{
    return ItemStackView{};
}

// 构造一个非空 ItemStackView：count>0 + itemId + 空 componentsPatch（空 patch 的 wire = 0x00 0x00）。
ItemStackView filledStack(u32 itemId, i32 count)
{
    ItemStackView v{};
    v.itemId = itemId;
    v.count = count;
    v.componentsPatch = {0x00, 0x00}; // 空 patch：VarInt(added=0)+VarInt(removed=0)
    return v;
}

// 构造一个非空 ItemStackView 带 componentsPatch 字节（须为合法 DataComponentPatch wire）。
ItemStackView filledStackWithPatch(u32 itemId, i32 count, std::vector<u8> patch)
{
    ItemStackView v{};
    v.itemId = itemId;
    v.count = count;
    v.componentsPatch = std::move(patch);
    return v;
}

// 构造一个 present=true 的 HashedStack。
HashedStack presentHashed(u32 itemId, i32 count)
{
    HashedStack v{};
    v.present = true;
    v.itemId = itemId;
    v.count = count;
    return v;
}

} // namespace

// ============================================================================
// 批 3：容器交互 C→S（ContainerClick/ContainerClose）
// ============================================================================

TEST_F(NetworkTestBase, PlayContainerClickPickupEmpty)
{
    ContainerClick in{};
    in.containerId = 1;
    in.stateId = 5;
    in.slotNum = 10;
    in.buttonNum = 0;
    in.clickType = 0; // PICKUP
    in.changedSlots = {};
    in.carriedItem = HashedStack{}; // present=false
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 2u);
    EXPECT_EQ(std::get<ContainerClick>(out), in);
}

TEST_F(NetworkTestBase, PlayContainerClickWithCarriedAndChangedSlots)
{
    ContainerClick in{};
    in.containerId = 2;
    in.stateId = 100;
    in.slotNum = -1; // -1 哨兵（容器外）
    in.buttonNum = 1;
    in.clickType = 1; // QUICK_MOVE
    ChangedSlot cs{};
    cs.slot = 5;
    cs.stack = presentHashed(1, 64); // 携带 stone×64
    in.changedSlots = {cs, ChangedSlot{12, presentHashed(17, 32)}};
    in.carriedItem = presentHashed(3, 1);
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 2u);
    EXPECT_EQ(std::get<ContainerClick>(out), in);
}

TEST_F(NetworkTestBase, PlayContainerClose)
{
    ContainerClose in{};
    in.containerId = 7;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 3u);
    EXPECT_EQ(std::get<ContainerClose>(out), in);
}

// ============================================================================
// 批 7：容器同步 S→C（ContainerSetContent/ContainerSetSlot/OpenScreen/ContainerSetData）
// ============================================================================

TEST_F(NetworkTestBase, PlayContainerSetContentEmpty)
{
    ContainerSetContent in{};
    in.containerId = 0;
    in.stateId = 0;
    in.items = {};
    in.carriedItem = emptyStack();
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 39u);
    EXPECT_EQ(std::get<ContainerSetContent>(out), in);
}

TEST_F(NetworkTestBase, PlayContainerSetContentFilled)
{
    ContainerSetContent in{};
    in.containerId = 3;
    in.stateId = 42;
    in.items = {filledStack(1, 64), emptyStack(), filledStackWithPatch(17, 1, {0x00, 0x01, 0x0B})};
    in.carriedItem = filledStack(4, 2);
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 39u);
    EXPECT_EQ(std::get<ContainerSetContent>(out), in);
}

TEST_F(NetworkTestBase, PlayContainerSetSlotEmpty)
{
    ContainerSetSlot in{};
    in.containerId = 1;
    in.stateId = 9;
    in.slot = 15;
    in.item = emptyStack();
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 40u);
    EXPECT_EQ(std::get<ContainerSetSlot>(out), in);
}

TEST_F(NetworkTestBase, PlayContainerSetSlotFilled)
{
    ContainerSetSlot in{};
    in.containerId = 2;
    in.stateId = 33;
    in.slot = -999; // -999 哨兵（容器外丢弃）
    in.item = filledStackWithPatch(264, 1, {0x00, 0x02, 0x07, 0x09});
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 40u);
    EXPECT_EQ(std::get<ContainerSetSlot>(out), in);
}

TEST_F(NetworkTestBase, PlayOpenScreen)
{
    OpenScreen in{};
    in.containerId = 5;
    in.menuType = 1; // 通用 9 格容器
    in.title = R"({"text":"Chest"})";
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 41u);
    EXPECT_EQ(std::get<OpenScreen>(out), in);
}

TEST_F(NetworkTestBase, PlayContainerSetData)
{
    ContainerSetData in{};
    in.containerId = 4;
    in.property = 0; // 熔炉烧制进度
    in.value = 200;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 42u);
    EXPECT_EQ(std::get<ContainerSetData>(out), in);
}

TEST_F(NetworkTestBase, PlayContainerSetDataNegativeValue)
{
    ContainerSetData in{};
    in.containerId = 4;
    in.property = 1;
    in.value = -1; // i16 边界
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 42u);
    EXPECT_EQ(std::get<ContainerSetData>(out), in);
}
