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

// ClientTickEnd（C→S，id=12，空 payload）Java wire codec 往返 + wire id 字节级锁定。
// altIndex 取自 IrPacket.hpp 的 PlayPacket variant 顺序（末尾追加，故为 115）。
// 线格式：整帧只有 VarInt(packetID)=12，无 payload。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <type_traits>
#include <variant>

using namespace mc;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;

// 编译期锁死 altIndex：变体末位必须是本包，防止后续有人插队导致登记错位
// （addPacket 的 matches 按 variant index 匹配，错位会静默匹配到别的包）。
static_assert(std::is_same_v<std::variant_alternative_t<115, PlayPacket>, ClientTickEnd>,
    "ClientTickEnd 必须是 PlayPacket 的第 115 个备选项（altIndex=115）");

TEST_F(NetworkTestBase, PlayClientTickEndRoundTrip)
{
    ClientTickEnd in{};
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 115u);
    EXPECT_EQ(std::get<ClientTickEnd>(out), in);
}

TEST_F(NetworkTestBase, PlayClientTickEndWireIdIs12AndPayloadEmpty)
{
    // 表级往返用同一张表编解码，wire id 登记错了也能通过，故此处做字节级锁定：
    // id=12 是单字节 VarInt，空 payload ⇒ 整帧恰好 1 字节，值为 0x0C。
    ClientTickEnd in{};
    auto encodeBuf = makeBoundBuf();
    auto enc = tables()->playSb->encode(encodeBuf, PlayPacket{in});
    ASSERT_TRUE(enc.success()) << enc.error().toString();

    ASSERT_EQ(encodeBuf.size(), 1u);
    EXPECT_EQ(encodeBuf.bytes()[0], 0x0Cu);

    buffer::RegistryByteBuf readBuf(encodeBuf.data(), encodeBuf.size(), RegistryAccess::instance());
    auto id = readBuf.readVarInt();
    ASSERT_TRUE(id.success()) << id.error().toString();
    EXPECT_EQ(id.value(), 12);
    EXPECT_EQ(readBuf.readableBytes(), 0u);
}

TEST_F(NetworkTestBase, PlayClientTickEndTableEntryBindsWireId12ToAltIndex115)
{
    // 直接查包表：id=12 这一项必须认领本备选项（空 packet id 已被上面的字节级测试覆盖）。
    const auto& entries = tables()->playSb->dispatch().entries();
    const PlayPacket probe{ClientTickEnd{}};
    bool foundId12 = false;
    for (const auto& entry : entries) {
        if (entry.id == 12) {
            foundId12 = true;
            EXPECT_TRUE(entry.matches(probe));
        }
    }
    EXPECT_TRUE(foundId12);
}
