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

// 批 21：命令/配方 mixed（Commands S→C + PlaceRecipe C→S）Java wire codec 往返。
// 表级往返（roundTripGeneric）兼测 packetID 分发 + payload codec。altIndex 取自
// IrPacket.hpp variant 顺序：Commands=87（S→C，opaque 命令树透传）/PlaceRecipe=88（C→S）。
// Commands.payload 是 opaque 字节（VarInt 长度前缀 + 字节），任意字节均验。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecsExtended.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;
using namespace mc;

// ============================================================================
// 批 21：命令/配方 mixed
// Commands（S→C，id=11）：opaque 命令树 payload，VarInt(len)+bytes 透传。
// PlaceRecipe（C→S，id=38）：VarInt(containerId)+VarInt(recipe)+Bool(useMaxItems)。
// ============================================================================

TEST_F(NetworkTestBase, PlayCommandsEmpty)
{
    Commands in{};
    in.payload = {}; // 空命令树（仅 VarInt(0) 长度前缀）
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 87u);
    EXPECT_EQ(std::get<Commands>(out), in);
}

TEST_F(NetworkTestBase, PlayCommandsFilled)
{
    Commands in{};
    // 任意非空 opaque 字节（codec 透传，命令树 Node 本体我方不解析，真 Java 互通标 Phase6）
    in.payload = {0x01, 0x02, 0x03, 0x0A, 0x0B, 0x0C, 0x10, 0x20, 0xFF, 0x7F, 0x80, 0x00};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 87u);
    EXPECT_EQ(std::get<Commands>(out), in);
}

TEST_F(NetworkTestBase, PlayPlaceRecipe)
{
    PlaceRecipe in{};
    in.containerId = 2;
    in.recipe = 15;
    in.useMaxItems = true;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 88u);
    EXPECT_EQ(std::get<PlaceRecipe>(out), in);
}

TEST_F(NetworkTestBase, PlayPlaceRecipeZeroIdNoShift)
{
    PlaceRecipe in{};
    in.containerId = 0;
    in.recipe = 0;
    in.useMaxItems = false;
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 88u);
    EXPECT_EQ(std::get<PlaceRecipe>(out), in);
}
