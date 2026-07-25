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
#include "common/network/codec/StreamCodecs.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

using namespace mc::network::buffer;
using namespace mc::network::codec;
using namespace mc::network::codec::stream_codecs;
using namespace mc;
using B = RegistryByteBuf;

// ============================================================================
// 定长原语 codec 往返
// ============================================================================

TEST(StreamCodecs, U8RoundTrip)
{
    auto codec = U8Codec<B>{};
    B buf;
    codec.encode(buf, 0xAB);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), 0xAB);
}

TEST(StreamCodecs, I16RoundTripNegative)
{
    auto codec = I16Codec<B>{};
    B buf;
    codec.encode(buf, -12345);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), -12345);
}

TEST(StreamCodecs, U32RoundTrip)
{
    auto codec = U32Codec<B>{};
    B buf;
    codec.encode(buf, 0xDEADBEEFu);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), 0xDEADBEEFu);
}

TEST(StreamCodecs, I64RoundTrip)
{
    auto codec = I64Codec<B>{};
    B buf;
    codec.encode(buf, -9000000000LL);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), -9000000000LL);
}

TEST(StreamCodecs, F32RoundTrip)
{
    auto codec = F32Codec<B>{};
    B buf;
    codec.encode(buf, 3.14f);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_NEAR(r.value(), 3.14f, 0.0001f);
}

TEST(StreamCodecs, F64RoundTrip)
{
    auto codec = F64Codec<B>{};
    B buf;
    codec.encode(buf, 2.718281828);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_NEAR(r.value(), 2.718281828, 1e-12);
}

TEST(StreamCodecs, BoolRoundTrip)
{
    auto codec = BoolCodec<B>{};
    B buf;
    codec.encode(buf, true);
    codec.encode(buf, false);
    EXPECT_TRUE(codec.decode(buf).value());
    EXPECT_FALSE(codec.decode(buf).value());
}

TEST(StreamCodecs, VarIntRoundTrip)
{
    auto codec = VarIntCodec<B>{};
    B buf;
    codec.encode(buf, 1234567);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), 1234567);
}

TEST(StreamCodecs, VarLongRoundTrip)
{
    auto codec = VarLongCodec<B>{};
    B buf;
    codec.encode(buf, 0x7FFFFFFFFFFFFFFFLL);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), 0x7FFFFFFFFFFFFFFFLL);
}

TEST(StreamCodecs, StringRoundTrip)
{
    auto codec = StringCodec<B>{};
    B buf;
    codec.encode(buf, std::string_view("hello codec"));
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), "hello codec");
}

TEST(StreamCodecs, PrimitiveDecodeEmptyBufFails)
{
    auto codec = U32Codec<B>{};
    B buf;
    auto r = codec.decode(buf);
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::OutOfBounds);
}

// ============================================================================
// optional codec
// ============================================================================

TEST(StreamCodecs, OptionalPresent)
{
    auto codec = optional<B>(VarIntCodec<B>{});
    B buf;
    codec.encode(buf, std::optional<i32>{42});
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    ASSERT_TRUE(r.value().has_value());
    EXPECT_EQ(*r.value(), 42);
}

TEST(StreamCodecs, OptionalAbsent)
{
    auto codec = optional<B>(VarIntCodec<B>{});
    B buf;
    codec.encode(buf, std::optional<i32>{});
    ASSERT_EQ(buf.size(), 1u); // 仅 1 字节 false 标志
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_FALSE(r.value().has_value());
}

// ============================================================================
// collection codec
// ============================================================================

TEST(StreamCodecs, CollectionEmpty)
{
    auto codec = collection<B>(VarIntCodec<B>{});
    B buf;
    codec.encode(buf, std::vector<i32>{});
    ASSERT_EQ(buf.size(), 1u); // VarInt(0)
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_TRUE(r.value().empty());
}

TEST(StreamCodecs, CollectionSingle)
{
    auto codec = collection<B>(VarIntCodec<B>{});
    B buf;
    codec.encode(buf, std::vector<i32>{7});
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    ASSERT_EQ(r.value().size(), 1u);
    EXPECT_EQ(r.value()[0], 7);
}

TEST(StreamCodecs, CollectionMany)
{
    auto codec = collection<B>(U8Codec<B>{});
    B buf;
    std::vector<u8> data(100);
    for (usize i = 0; i < 100; ++i) {
        data[i] = static_cast<u8>(i);
    }
    codec.encode(buf, data);
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), data);
}

TEST(StreamCodecs, CollectionNegativeLengthDecodeFails)
{
    // 手写一个 VarInt(-1) 长度前缀，解码应拒绝
    auto codec = collection<B>(U8Codec<B>{});
    B buf;
    buf.writeVarInt(-1);
    auto r = codec.decode(buf);
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidData);
}

// ============================================================================
// memberCodec + unit
// ============================================================================

namespace {

struct SimpleStruct {
    i32 a = 0;
    std::string b;
};

} // namespace

TEST(StreamCodecs, MemberCodecRoundTrip)
{
    auto codecA = memberCodec<SimpleStruct>(&SimpleStruct::a, VarIntCodec<B>{});
    auto codecB = memberCodec<SimpleStruct>(&SimpleStruct::b, StringCodec<B>{});

    SimpleStruct src;
    src.a = 99;
    src.b = "member";

    B buf;
    codecA.encode(buf, src);
    codecB.encode(buf, src);

    SimpleStruct dst;
    ASSERT_TRUE(codecA.decode(buf, dst).success());
    ASSERT_TRUE(codecB.decode(buf, dst).success());
    EXPECT_EQ(dst.a, 99);
    EXPECT_EQ(dst.b, "member");
}

TEST(StreamCodecs, UnitCodecRoundTrip)
{
    struct Empty {};
    auto codec = unit<Empty>();
    B buf;
    Empty src;
    codec.encode(buf, src);
    ASSERT_EQ(buf.size(), 0u); // unit 写空
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    (void)r.value();
}

TEST(StreamCodecs, UnitCodecDecodesDefaultOnEmptyBuf)
{
    // unit 解码不读任何字节，空缓冲仍应成功返默认值
    auto codec = unit<i32>();
    B buf;
    auto r = codec.decode(buf);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), 0);
}
