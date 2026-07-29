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

#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <gtest/gtest.h>

#include <cstring>

using namespace mc::network::buffer;
using namespace mc::network::buffer::nbt_io;
using namespace mc::nbt::tags;
using namespace mc;

namespace {

// 在 parent 下挂一个空的子 compound，返回其引用（compound_tag 的 tag<T>() 模板按
// tag_of<T::value_type> 解析，传 tag 类型本身不可用，故直接 value.emplace）。
compound_tag& emplaceCompound(compound_tag& parent, std::string name)
{
    auto child = std::make_unique<compound_tag>();
    auto& ref = *child;
    parent.value.emplace(std::move(name), std::move(child));
    return ref;
}

// 构造一个含多种字段的复合标签。网络 NBT 用非根 compound（is_root=false）——根 compound
// 是 Java 存档层概念（带 name），wire 层往返用非根才与 writeCompound/readCompound 对称
// （非根写出含 End 终止符，根写出无终止符导致 readCompound 游标结算异常）。
// put("k", v) 按值类型推断 tag（int→int_tag, std::string→string_tag），勿用 put<int_tag>。
std::unique_ptr<compound_tag> makeSampleCompound()
{
    auto tag = std::make_unique<compound_tag>();
    tag->put("value", static_cast<i32>(42));
    tag->put("name", std::string("tester"));
    auto& nested = emplaceCompound(*tag, "nested");
    nested.put("inner", static_cast<i32>(-7));
    return tag;
}

// Result<unique_ptr<T>>::value() 按值返回 unique_ptr 且每次调用都 takeValue（清空内部
// 裸指针）。故对同一 Result 多次调 value() 第二次必得 nullptr。这里一次性取走所有权到
// 局部 unique_ptr，后续断言都基于该局部变量，避免 use-after-free / 空指针解引用。
std::unique_ptr<compound_tag> takeCompound(ByteBuf& buf)
{
    auto r = readCompound(buf);
    EXPECT_TRUE(r.success());
    return r.success() ? r.value() : nullptr;
}

} // namespace

TEST(NbtIo, WriteReadRoundTripScalars)
{
    auto original = makeSampleCompound();

    ByteBuf buf;
    ASSERT_TRUE(writeCompound(buf, *original).success());

    auto decoded = takeCompound(buf);
    ASSERT_NE(decoded, nullptr);
    EXPECT_TRUE(decoded->equals(*original));
}

TEST(NbtIo, RoundTripPreservesIntValue)
{
    compound_tag src;
    src.put("v", static_cast<i32>(123456));

    ByteBuf buf;
    ASSERT_TRUE(writeCompound(buf, src).success());
    auto decoded = takeCompound(buf);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->get<int_tag>("v"), 123456);
}

TEST(NbtIo, RoundTripPreservesString)
{
    compound_tag src;
    src.put("s", std::string("hello nbt"));

    ByteBuf buf;
    ASSERT_TRUE(writeCompound(buf, src).success());
    auto decoded = takeCompound(buf);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->get<string_tag>("s"), "hello nbt");
}

TEST(NbtIo, RoundTripNestedCompound)
{
    compound_tag src;
    auto& nested = emplaceCompound(src, "child");
    nested.put("x", static_cast<i32>(99));

    ByteBuf buf;
    ASSERT_TRUE(writeCompound(buf, src).success());
    auto decoded = takeCompound(buf);
    ASSERT_NE(decoded, nullptr);
    // equals 递归比较嵌套
    EXPECT_TRUE(decoded->equals(src));
    // 验证嵌套字段可达
    const auto& child = dynamic_cast<const compound_tag&>(*decoded->value.at("child"));
    EXPECT_EQ(child.get<int_tag>("x"), 99);
}

TEST(NbtIo, SkipCompoundAdvancesCursor)
{
    auto first = makeSampleCompound();
    compound_tag second;
    second.put("after", static_cast<i32>(7));

    ByteBuf buf;
    ASSERT_TRUE(writeCompound(buf, *first).success());
    ASSERT_TRUE(writeCompound(buf, second).success());

    // 跳过第一个，读第二个应得到 "after"
    ASSERT_TRUE(skipCompound(buf).success());
    auto decoded = takeCompound(buf);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->get<int_tag>("after"), 7);
}

TEST(NbtIo, EmptyCompoundRoundTrip)
{
    compound_tag src; // 空非根 compound（写出 = 单字节 End）

    ByteBuf buf;
    ASSERT_TRUE(writeCompound(buf, src).success());
    auto decoded = takeCompound(buf);
    ASSERT_NE(decoded, nullptr);
    EXPECT_TRUE(decoded->value.empty());
}

TEST(NbtIo, ReadCompoundMalformedThrows)
{
    // 底层 NBT 解析器（read_compound_bin）遇未知 tag id（>LongArray）抛 std::out_of_range，
    // nbt_io::readCompound 未捕获——故畸形输入表现为抛异常而非 Result 错误。
    ByteBuf buf;
    buf.writeU8(0xFF); // 非法 tag id
    buf.writeU8(0x00); // 1 字节 key 长度（不足以完整解析，但 id 校验先抛）
    EXPECT_THROW(readCompound(buf), std::exception);
}

TEST(NbtIo, WriteCompoundDoesNotConsumeReadCursor)
{
    // writeCompound 只追加字节，不应影响读游标（ByteBuf 单缓冲，写后读从头）
    compound_tag src;
    src.put("v", static_cast<i32>(1));

    ByteBuf buf;
    buf.writeU8(0xAA); // 先写一字节哨兵
    ASSERT_TRUE(writeCompound(buf, src).success());

    EXPECT_EQ(buf.readU8().value(), 0xAA); // 哨兵仍在
    auto decoded = takeCompound(buf);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->get<int_tag>("v"), 1);
}

TEST(NbtIo, SerializeRootCompoundProducesJavaWireFormat)
{
    // serializeRootCompoundToBytes 须输出 Java ByteBufCodecs.TAG 线格式：
    //   0x0A（compound 类型字节）+ entries + 0x00（End）。**无 root name 前缀**。
    // 对齐 FriendlyByteBuf.writeNbt = NbtIo.writeAnyTag：writeByte(0x0A) + tag.write()，
    // compound 的 write 写 entries+End 不写 name，readAnyTag 对称不读 name。早先误加 0x00 0x00
    // 空 root name 致客户端 "Expected non-null compound tag"（disconnect-2026-07-29_13.51.13）。
    // writeCompound 仅写 body（无 0x0A），二者必须区分。
    compound_tag src;
    src.put("v", static_cast<i32>(42));

    const std::vector<u8> bytes = serializeRootCompoundToBytes(src);
    ASSERT_GE(bytes.size(), 2u);
    // 根 NBT 前缀：仅类型字节 0x0A（无 root name）
    EXPECT_EQ(bytes[0], 0x0A) << "缺 compound 类型字节 0x0A";
    // 第二字节应是第一个 entry 的 tag id（int=3），**非** root name 长度字节
    EXPECT_EQ(bytes[1], static_cast<u8>(mc::nbt::TagId::Int)) << "第二字节应为首个 entry 的类型（int=3）";
    // 尾部 End 0x00
    EXPECT_EQ(bytes.back(), 0x00) << "应以 End 0x00 结尾";

    // writeRootCompound 写入 ByteBuf 应等价于直接 writeBytes(bytes)。
    ByteBuf buf;
    ASSERT_TRUE(writeRootCompound(buf, src).success());
    ASSERT_EQ(buf.readableBytes(), bytes.size());
    const auto* bufData = buf.bytes().data() + buf.readPosition();
    EXPECT_EQ(std::memcmp(bufData, bytes.data(), bytes.size()), 0);
}

TEST(NbtIo, SerializeRootEmptyCompoundIsPrefixPlusEnd)
{
    // 空 compound 的根 NBT = 0x0A 0x00（类型字节 + End，无 entries、无 root name）。
    compound_tag src; // 空非根 compound
    const std::vector<u8> bytes = serializeRootCompoundToBytes(src);
    ASSERT_EQ(bytes.size(), 2u);
    EXPECT_EQ(bytes[0], 0x0A);
    EXPECT_EQ(bytes[1], 0x00); // End
}

TEST(NbtIo, ReadRootCompoundMatchesJavaWriteAnyTag)
{
    // 验证 readRootCompound 与 serializeRootCompoundToBytes 对称（无 root name），且读回的
    // 字节范围与写入逐字节相等——锁定 registryDataCodec 切片契约。
    compound_tag src;
    src.put("anvil_cost", static_cast<i32>(5));
    src.put("name", std::string("x"));
    const std::vector<u8> bytes = serializeRootCompoundToBytes(src);

    ByteBuf buf;
    buf.writeBytes(bytes.data(), bytes.size());
    const usize start = buf.readPosition();
    auto r = readRootCompound(buf);
    ASSERT_TRUE(r.success());
    const usize end = buf.readPosition();
    ASSERT_EQ(end - start, bytes.size()) << "readRootCompound 须恰好消费完整根 NBT 字节";
    // 已消费全部，无剩余
    EXPECT_EQ(buf.readableBytes(), 0u);

    auto root = r.success() ? r.value() : nullptr;
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->get<int_tag>("anvil_cost"), 5);
}
