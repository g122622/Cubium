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

#include "common/network/pipeline/VarintFraming.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace mc::network::pipeline;
using namespace mc;

namespace {

std::vector<u8> makePayload(usize n)
{
    std::vector<u8> p(n);
    for (usize i = 0; i < n; ++i) {
        p[i] = static_cast<u8>(i * 3 + 1);
    }
    return p;
}

/// 断言"切出一帧"并返回其 payload。
std::vector<u8> expectFrame(std::vector<u8>& buffer)
{
    std::vector<u8> out;
    auto r = VarintFraming::tryDecodeFrame(buffer, out);
    EXPECT_TRUE(r.success());
    if (r.success()) {
        EXPECT_TRUE(r.value());
    }
    return out;
}

/// 断言"数据不足"（ok(false)，非错误）。
void expectNeedMore(std::vector<u8>& buffer)
{
    std::vector<u8> out;
    auto r = VarintFraming::tryDecodeFrame(buffer, out);
    ASSERT_TRUE(r.success());
    EXPECT_FALSE(r.value());
    EXPECT_TRUE(out.empty());
}

/// 断言"帧结构非法"（返回错误，调用方应据此断开连接）。
void expectFramingError(std::vector<u8>& buffer)
{
    std::vector<u8> out;
    auto r = VarintFraming::tryDecodeFrame(buffer, out);
    EXPECT_FALSE(r.success());
}

/// 手写 VarInt 编码（测试内不依赖 ByteBuf，保持帧层测试自洽）。
std::vector<u8> writeVarUInt(u32 value)
{
    std::vector<u8> out;
    while (true) {
        if ((value & ~static_cast<u32>(0x7F)) == 0) {
            out.push_back(static_cast<u8>(value));
            return out;
        }
        out.push_back(static_cast<u8>((value & 0x7Fu) | 0x80u));
        value >>= 7;
    }
}

} // namespace

TEST(VarintFraming, EncodeDecodeRoundTrip)
{
    auto payload = makePayload(256);
    std::vector<u8> frame;
    VarintFraming::encodeFrame(payload.data(), payload.size(), frame);

    // 帧 = VarInt(256) + 256 字节 = 2 + 256 = 258
    ASSERT_EQ(frame.size(), 258u);

    std::vector<u8> buffer = frame;
    auto out = expectFrame(buffer);
    EXPECT_EQ(out, payload);
    EXPECT_TRUE(buffer.empty());
}

TEST(VarintFraming, HalfFrameReturnsFalseAndRetainsBuffer)
{
    auto payload = makePayload(100);
    std::vector<u8> frame;
    VarintFraming::encodeFrame(payload.data(), payload.size(), frame);

    // 只给前半字节
    std::vector<u8> partial(frame.begin(), frame.begin() + frame.size() / 2);
    expectNeedMore(partial);
    // 缓冲应保留（未消费）
    EXPECT_FALSE(partial.empty());
}

TEST(VarintFraming, MultipleFramesDecodeInOrder)
{
    std::vector<u8> stream;
    std::vector<u8> f1 = makePayload(10);
    std::vector<u8> f2 = makePayload(20);
    std::vector<u8> f3 = makePayload(30);
    VarintFraming::encodeFrame(f1.data(), f1.size(), stream);
    VarintFraming::encodeFrame(f2.data(), f2.size(), stream);
    VarintFraming::encodeFrame(f3.data(), f3.size(), stream);

    EXPECT_EQ(expectFrame(stream), f1);
    EXPECT_EQ(expectFrame(stream), f2);
    EXPECT_EQ(expectFrame(stream), f3);
    EXPECT_TRUE(stream.empty());
}

TEST(VarintFraming, BoundarySizes)
{
    // 127/128 字节边界（VarInt 长度从 1 字节跳 2 字节）
    for (usize n : {1u, 127u, 128u, 129u}) {
        auto payload = makePayload(n);
        std::vector<u8> frame;
        VarintFraming::encodeFrame(payload.data(), payload.size(), frame);
        std::vector<u8> buf = frame;
        auto out = expectFrame(buf);
        EXPECT_EQ(out, payload) << "n=" << n;
    }
}

TEST(VarintFraming, ZeroLengthFrame)
{
    // 零长 payload：帧 = 单字节 VarInt(0) = 0x00
    std::vector<u8> frame;
    VarintFraming::encodeFrame(nullptr, 0, frame);
    ASSERT_EQ(frame.size(), 1u);
    EXPECT_EQ(frame[0], 0x00);

    std::vector<u8> buf = frame;
    auto out = expectFrame(buf);
    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(buf.empty());
}

TEST(VarintFraming, LargeFrame64KB)
{
    std::vector<u8> payload(65536, 0x5A);
    std::vector<u8> frame;
    VarintFraming::encodeFrame(payload.data(), payload.size(), frame);
    std::vector<u8> buf = frame;
    auto out = expectFrame(buf);
    EXPECT_EQ(out, payload);
    EXPECT_TRUE(buf.empty());
}

TEST(VarintFraming, EmptyBufferReturnsFalse)
{
    std::vector<u8> buffer;
    expectNeedMore(buffer);
    EXPECT_TRUE(buffer.empty());
}

TEST(VarintFraming, DecodeConsumesFromBuffer)
{
    auto payload = makePayload(50);
    std::vector<u8> frame;
    VarintFraming::encodeFrame(payload.data(), payload.size(), frame);
    // 帧后追加哨兵字节
    frame.push_back(0xFF);

    auto out = expectFrame(frame);
    EXPECT_EQ(payload, out);
    ASSERT_EQ(frame.size(), 1u); // 仅剩哨兵
    EXPECT_EQ(frame[0], 0xFF);
}

// ============================================================================
// 帧结构非法路径的回归用例
//
// 这两类输入此前被当作"数据不足"（返回 false），调用方会一直等待"足够的数据"，
// 而实际上永远等不到 —— 结果是入站缓冲无上限增长直至内存耗尽（可远程触发）。
// 现在必须返回错误，由调用方断开连接。
// ============================================================================

TEST(VarintFraming, OversizedDeclaredLengthIsRejected)
{
    // VarInt(0x7FFFFFFF) = FF FF FF FF 07，远超声明的帧长上限
    std::vector<u8> buffer{0xFF, 0xFF, 0xFF, 0xFF, 0x07};
    expectFramingError(buffer);
    // 缓冲区不被消费：清理与断连由调用方负责
    EXPECT_EQ(buffer.size(), 5u);
}

TEST(VarintFraming, DeclaredLengthJustAboveLimitIsRejected)
{
    std::vector<u8> buffer = writeVarUInt(VarintFraming::kMaxFramePayloadSize + 1);
    expectFramingError(buffer);
}

TEST(VarintFraming, DeclaredLengthExactlyAtLimitIsNotAnError)
{
    // 恰好等于上限属合法声明，只是数据不足 → ok(false)，不得报错
    std::vector<u8> buffer = writeVarUInt(VarintFraming::kMaxFramePayloadSize);
    expectNeedMore(buffer);
}

TEST(VarintFraming, VarIntPrefixExceedingFiveBytesIsRejected)
{
    // 5 个字节全部带续位 → 非法 VarInt（无法构成合法长度前缀）
    std::vector<u8> buffer{0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
    expectFramingError(buffer);
}

TEST(VarintFraming, TruncatedVarIntPrefixIsNotAnError)
{
    // 前缀被切断（续位仍为 1，但因数据不足而中断）→ 数据不足，等更多字节
    std::vector<u8> buffer{0x80, 0x80};
    expectNeedMore(buffer);
}
