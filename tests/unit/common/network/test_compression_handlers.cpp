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

#include "common/network/pipeline/CompressionHandlers.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace mc::network::pipeline;
using namespace mc;

namespace {

std::vector<u8> makePayload(usize n)
{
    std::vector<u8> p(n);
    for (usize i = 0; i < n; ++i) {
        p[i] = static_cast<u8>(i * 5 + 2);
    }
    return p;
}

} // namespace

TEST(CompressionHandlers, RoundTrip1KB)
{
    CompressionEncoder enc(256);
    CompressionDecoder dec(256);

    auto input = makePayload(1024);
    std::vector<u8> compressed;
    ASSERT_TRUE(enc.encode(input, compressed).success());

    std::vector<u8> restored;
    ASSERT_TRUE(dec.decode(compressed, restored).success());
    EXPECT_EQ(restored, input);
}

TEST(CompressionHandlers, BelowThresholdPassesThrough)
{
    // input 100 < threshold 256 → 不压缩（dataLength=0）
    CompressionEncoder enc(256);
    CompressionDecoder dec(256);

    auto input = makePayload(100);
    std::vector<u8> compressed;
    ASSERT_TRUE(enc.encode(input, compressed).success());
    // VarInt(0) + 100 字节 = 101
    EXPECT_EQ(compressed.size(), 101u);

    std::vector<u8> restored;
    ASSERT_TRUE(dec.decode(compressed, restored).success());
    EXPECT_EQ(restored, input);
}

TEST(CompressionHandlers, AboveThresholdCompresses)
{
    CompressionEncoder enc(256);
    CompressionDecoder dec(256);

    auto input = makePayload(512);
    std::vector<u8> compressed;
    ASSERT_TRUE(enc.encode(input, compressed).success());
    // 压缩包首字节 VarInt(512) 非 0
    EXPECT_NE(compressed[0], 0x00);

    std::vector<u8> restored;
    ASSERT_TRUE(dec.decode(compressed, restored).success());
    EXPECT_EQ(restored, input);
}

TEST(CompressionHandlers, NegativeThresholdIdentity)
{
    // threshold=-1 禁用压缩：encode 写 VarInt(0)+原文，decode 直返
    CompressionEncoder encoder(-1);
    CompressionDecoder decoder(-1);

    auto input = makePayload(500);
    std::vector<u8> out1;
    ASSERT_TRUE(encoder.encode(input, out1).success());
    EXPECT_EQ(out1.size(), 501u); // VarInt(0) + 500

    std::vector<u8> restored;
    ASSERT_TRUE(decoder.decode(out1, restored).success());
    EXPECT_EQ(restored, input);
}

TEST(CompressionHandlers, ThresholdAccessor)
{
    CompressionEncoder enc(256);
    CompressionDecoder dec(256);
    EXPECT_EQ(enc.threshold(), 256);
    EXPECT_EQ(dec.threshold(), 256);
}
