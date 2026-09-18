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

} // namespace

TEST(VarintFraming, EncodeDecodeRoundTrip)
{
    auto payload = makePayload(256);
    std::vector<u8> frame;
    VarintFraming::encodeFrame(payload.data(), payload.size(), frame);

    // 帧 = VarInt(256) + 256 字节 = 2 + 256 = 258
    ASSERT_EQ(frame.size(), 258u);

    std::vector<u8> out;
    std::vector<u8> buffer = frame;
    ASSERT_TRUE(VarintFraming::tryDecodeFrame(buffer, out));
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
    std::vector<u8> out;
    EXPECT_FALSE(VarintFraming::tryDecodeFrame(partial, out));
    // 缓冲应保留（未消费）
    EXPECT_FALSE(partial.empty());
    EXPECT_TRUE(out.empty());
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

    std::vector<u8> out;
    ASSERT_TRUE(VarintFraming::tryDecodeFrame(stream, out));
    EXPECT_EQ(out, f1);
    ASSERT_TRUE(VarintFraming::tryDecodeFrame(stream, out));
    EXPECT_EQ(out, f2);
    ASSERT_TRUE(VarintFraming::tryDecodeFrame(stream, out));
    EXPECT_EQ(out, f3);
    EXPECT_TRUE(stream.empty());
}

TEST(VarintFraming, BoundarySizes)
{
    // 127/128 字节边界（VarInt 长度从 1 字节跳 2 字节）
    for (usize n : {1u, 127u, 128u, 129u}) {
        auto payload = makePayload(n);
        std::vector<u8> frame;
        VarintFraming::encodeFrame(payload.data(), payload.size(), frame);
        std::vector<u8> out;
        std::vector<u8> buf = frame;
        ASSERT_TRUE(VarintFraming::tryDecodeFrame(buf, out)) << "n=" << n;
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

    std::vector<u8> out;
    std::vector<u8> buf = frame;
    ASSERT_TRUE(VarintFraming::tryDecodeFrame(buf, out));
    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(buf.empty());
}

TEST(VarintFraming, LargeFrame64KB)
{
    std::vector<u8> payload(65536, 0x5A);
    std::vector<u8> frame;
    VarintFraming::encodeFrame(payload.data(), payload.size(), frame);
    std::vector<u8> out;
    std::vector<u8> buf = frame;
    ASSERT_TRUE(VarintFraming::tryDecodeFrame(buf, out));
    EXPECT_EQ(out, payload);
    EXPECT_TRUE(buf.empty());
}

TEST(VarintFraming, EmptyBufferReturnsFalse)
{
    std::vector<u8> buffer;
    std::vector<u8> out;
    EXPECT_FALSE(VarintFraming::tryDecodeFrame(buffer, out));
    EXPECT_TRUE(buffer.empty());
}

TEST(VarintFraming, DecodeConsumesFromBuffer)
{
    auto payload = makePayload(50);
    std::vector<u8> frame;
    VarintFraming::encodeFrame(payload.data(), payload.size(), frame);
    // 帧后追加哨兵字节
    frame.push_back(0xFF);

    std::vector<u8> out;
    ASSERT_TRUE(VarintFraming::tryDecodeFrame(frame, out));
    EXPECT_EQ(out, payload);
    ASSERT_EQ(frame.size(), 1u); // 仅剩哨兵
    EXPECT_EQ(frame[0], 0xFF);
}
