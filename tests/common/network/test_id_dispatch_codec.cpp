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

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/codec/IdDispatchCodec.hpp"

#include <gtest/gtest.h>

#include <string>
#include <variant>

using namespace mc::network::buffer;
using namespace mc::network::codec;
using namespace mc;
using B = RegistryByteBuf;

namespace {

// 三种哑变体类型
struct VarA {
    i32 x = 0;
};
struct VarB {
    std::string s;
};
struct VarC {
    u8 b = 0;
};

using DummyVariant = std::variant<VarA, VarB, VarC>;

// matches 判定 + encode/decode lambda
IdDispatchCodec<B, DummyVariant> makeThreeEntryCodec()
{
    IdDispatchCodec<B, DummyVariant> codec;
    codec.addPacket(
        /*id*/
        0,
        [](const DummyVariant& v) { return std::holds_alternative<VarA>(v); },
        [](B& buf, const DummyVariant& v) { buf.writeI32(std::get<VarA>(v).x); },
        [](B& buf) -> Result<DummyVariant> {
            auto r = buf.readI32();
            if (!r.success()) {
                return r.error();
            }
            return DummyVariant{VarA{r.value()}};
        });
    codec.addPacket(
        /*id*/
        1,
        [](const DummyVariant& v) { return std::holds_alternative<VarB>(v); },
        [](B& buf, const DummyVariant& v) { buf.writeString(std::get<VarB>(v).s); },
        [](B& buf) -> Result<DummyVariant> {
            auto r = buf.readString();
            if (!r.success()) {
                return r.error();
            }
            return DummyVariant{VarB{r.value()}};
        });
    codec.addPacket(
        /*id*/
        2,
        [](const DummyVariant& v) { return std::holds_alternative<VarC>(v); },
        [](B& buf, const DummyVariant& v) { buf.writeU8(std::get<VarC>(v).b); },
        [](B& buf) -> Result<DummyVariant> {
            auto r = buf.readU8();
            if (!r.success()) {
                return r.error();
            }
            return DummyVariant{VarC{r.value()}};
        });
    return codec;
}

} // namespace

TEST(IdDispatchCodec, ThreeEntriesRoundTrip)
{
    auto codec = makeThreeEntryCodec();
    ASSERT_EQ(codec.size(), 3u);

    // VarA
    {
        B buf;
        DummyVariant in = VarA{42};
        ASSERT_TRUE(codec.encode(buf, in).success());
        auto r = codec.decode(buf);
        ASSERT_TRUE(r.success());
        ASSERT_TRUE(std::holds_alternative<VarA>(r.value()));
        EXPECT_EQ(std::get<VarA>(r.value()).x, 42);
    }
    // VarB
    {
        B buf;
        DummyVariant in = VarB{std::string("hi")};
        ASSERT_TRUE(codec.encode(buf, in).success());
        auto r = codec.decode(buf);
        ASSERT_TRUE(r.success());
        ASSERT_TRUE(std::holds_alternative<VarB>(r.value()));
        EXPECT_EQ(std::get<VarB>(r.value()).s, "hi");
    }
    // VarC
    {
        B buf;
        DummyVariant in = VarC{0xCD};
        ASSERT_TRUE(codec.encode(buf, in).success());
        auto r = codec.decode(buf);
        ASSERT_TRUE(r.success());
        ASSERT_TRUE(std::holds_alternative<VarC>(r.value()));
        EXPECT_EQ(std::get<VarC>(r.value()).b, 0xCD);
    }
}

TEST(IdDispatchCodec, WritesIdPrefixBeforePayload)
{
    // 验证编码格式 = VarInt(id) + payload：VarA id=0 写 1 字节 id(0) + 4 字节 i32
    auto codec = makeThreeEntryCodec();
    B buf;
    ASSERT_TRUE(codec.encode(buf, DummyVariant{VarA{1}}).success());
    ASSERT_EQ(buf.size(), 5u);       // 1(id=0) + 4(i32)
    EXPECT_EQ(buf.bytes()[0], 0x00); // id=0
}

TEST(IdDispatchCodec, DuplicateIdRejected)
{
    IdDispatchCodec<B, DummyVariant> codec;
    ASSERT_TRUE(codec.addPacket(
        0,
        [](const DummyVariant&) { return false; },
        [](B&, const DummyVariant&) {},
        [](B&) -> Result<DummyVariant> { return DummyVariant{}; }));
    // 重复 id=0 应返回 false
    EXPECT_FALSE(codec.addPacket(
        0,
        [](const DummyVariant&) { return false; },
        [](B&, const DummyVariant&) {},
        [](B&) -> Result<DummyVariant> { return DummyVariant{}; }));
    EXPECT_EQ(codec.size(), 1u);
}

TEST(IdDispatchCodec, UnknownIdDecodeFails)
{
    auto codec = makeThreeEntryCodec();
    B buf;
    buf.writeVarInt(99); // 未登记的 id
    auto r = codec.decode(buf);
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::ProtocolError);
}

TEST(IdDispatchCodec, UnregisteredTypeEncodeFails)
{
    // codec 只登记 VarA/VarB/VarC；用一个不在表里的第 4 类型测试 encode 失败路径
    // 这里 VarA/VarB/VarC 全登记了，故改用空 codec + 任一类型验证
    IdDispatchCodec<B, DummyVariant> empty; // 无登记
    B buf;
    auto r = empty.encode(buf, DummyVariant{VarA{1}});
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::ProtocolError);
}

TEST(IdDispatchCodec, OrderIndependentByIdNotByIndex)
{
    // 乱序注册：先 id=2 再 id=0 再 id=1，仍按 id 正确分发
    IdDispatchCodec<B, DummyVariant> codec;
    codec.addPacket(
        2,
        [](const DummyVariant& v) { return std::holds_alternative<VarC>(v); },
        [](B& buf, const DummyVariant& v) { buf.writeU8(std::get<VarC>(v).b); },
        [](B& buf) -> Result<DummyVariant> {
            auto r = buf.readU8();
            return r.success() ? Result<DummyVariant>(DummyVariant{VarC{r.value()}}) : Result<DummyVariant>(r.error());
        });
    codec.addPacket(
        0,
        [](const DummyVariant& v) { return std::holds_alternative<VarA>(v); },
        [](B& buf, const DummyVariant& v) { buf.writeI32(std::get<VarA>(v).x); },
        [](B& buf) -> Result<DummyVariant> {
            auto r = buf.readI32();
            return r.success() ? Result<DummyVariant>(DummyVariant{VarA{r.value()}}) : Result<DummyVariant>(r.error());
        });

    // encode VarA(id=0) → decode 应得 VarA
    B buf;
    ASSERT_TRUE(codec.encode(buf, DummyVariant{VarA{55}}).success());
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    ASSERT_TRUE(std::holds_alternative<VarA>(r.value()));
    EXPECT_EQ(std::get<VarA>(r.value()).x, 55);

    // encode VarC(id=2) → decode 应得 VarC
    B buf2;
    ASSERT_TRUE(codec.encode(buf2, DummyVariant{VarC{0xEE}}).success());
    auto r2 = codec.decode(buf2);
    ASSERT_TRUE(r2.success());
    ASSERT_TRUE(std::holds_alternative<VarC>(r2.value()));
    EXPECT_EQ(std::get<VarC>(r2.value()).b, 0xEE);
}
