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

// Base64 编解码（RFC 4648 §4）与 PNG 头解析单元测试。
// 覆盖：编码对齐 Java Base64.getEncoder()（含 '=' 填充）；解码往返；空白容忍；非法输入整体失败；
// PNG 签名/IHDR 宽高解析与非法输入错误。

#include "common/util/Base64.hpp"
#include "common/util/PngInfo.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

using namespace mc;
using namespace mc::util;

namespace {

std::vector<u8> toBytes(std::string_view text)
{
    return std::vector<u8>(text.begin(), text.end());
}

} // namespace

// ============================================================================
// Base64 编码
// ============================================================================

TEST(Base64Test, EncodesRfc4648Vectors)
{
    // RFC 4648 §10 测试向量。
    EXPECT_EQ(base64Encode(toBytes("")), "");
    EXPECT_EQ(base64Encode(toBytes("f")), "Zg==");
    EXPECT_EQ(base64Encode(toBytes("fo")), "Zm8=");
    EXPECT_EQ(base64Encode(toBytes("foo")), "Zm9v");
    EXPECT_EQ(base64Encode(toBytes("foob")), "Zm9vYg==");
    EXPECT_EQ(base64Encode(toBytes("fooba")), "Zm9vYmE=");
    EXPECT_EQ(base64Encode(toBytes("foobar")), "Zm9vYmFy");
}

TEST(Base64Test, EncodesAllByteValuesRoundTrip)
{
    // 0x00..0xFF 全覆盖，验证任意字节（含高位）编码后再解码无损。
    std::vector<u8> data(256);
    for (usize i = 0; i < data.size(); ++i) {
        data[i] = static_cast<u8>(i);
    }
    const std::string encoded = base64Encode(data);
    EXPECT_EQ(encoded.size() % 4, 0u);
    EXPECT_EQ(base64Decode(encoded), data);
}

// ============================================================================
// Base64 解码
// ============================================================================

TEST(Base64Test, DecodesRoundTrip)
{
    for (const std::string& text : {"", "f", "fo", "foo", "foob", "fooba", "foobar", "Hello, Minecraft!"}) {
        EXPECT_EQ(base64Decode(base64Encode(toBytes(text))), toBytes(text)) << "text=" << text;
    }
}

TEST(Base64Test, DecodeIgnoresAsciiWhitespace)
{
    // 空白字符被忽略（与皮肤签名解码的宽容语义一致）。
    EXPECT_EQ(base64Decode("Zm9v\nYmFy"), base64Decode("Zm9vYmFy"));
    EXPECT_EQ(base64Decode(" Zm9v\r\n YmFy "), base64Decode("Zm9vYmFy"));
}

TEST(Base64Test, DecodeFailsOnInvalidInput)
{
    // 长度非 4 的倍数、非法字符均整体失败（返回空），避免部分解码出非空字节。
    EXPECT_TRUE(base64Decode("Zm9vYmF").empty());
    EXPECT_TRUE(base64Decode("!!!!").empty());
    EXPECT_TRUE(base64Decode("Zm9v!mFy").empty());
    EXPECT_TRUE(base64Decode("").empty());
}

// ============================================================================
// PNG 头解析
// ============================================================================

namespace {

/// 构造一个最小 PNG 头：签名 + IHDR(length=13,type="IHDR",width,height)。
std::vector<u8> makePngHeader(i32 width, i32 height)
{
    std::vector<u8> data = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}; // 签名
    // IHDR 长度 = 13
    data.insert(data.end(), {0x00, 0x00, 0x00, 0x0D});
    // IHDR 类型
    data.insert(data.end(), {'I', 'H', 'D', 'R'});
    // 宽（大端）
    data.push_back(static_cast<u8>((width >> 24) & 0xFF));
    data.push_back(static_cast<u8>((width >> 16) & 0xFF));
    data.push_back(static_cast<u8>((width >> 8) & 0xFF));
    data.push_back(static_cast<u8>(width & 0xFF));
    // 高（大端）
    data.push_back(static_cast<u8>((height >> 24) & 0xFF));
    data.push_back(static_cast<u8>((height >> 16) & 0xFF));
    data.push_back(static_cast<u8>((height >> 8) & 0xFF));
    data.push_back(static_cast<u8>(height & 0xFF));
    return data;
}

} // namespace

TEST(PngInfoTest, ParsesWidthHeight)
{
    const auto data = makePngHeader(64, 64);
    auto result = parsePngInfo(data);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().width, 64);
    EXPECT_EQ(result.value().height, 64);

    const auto other = makePngHeader(16, 32);
    auto result2 = parsePngInfo(other);
    ASSERT_TRUE(result2.success());
    EXPECT_EQ(result2.value().width, 16);
    EXPECT_EQ(result2.value().height, 32);
}

TEST(PngInfoTest, RejectsShortOrBadData)
{
    // 过短。
    EXPECT_TRUE(parsePngInfo(std::vector<u8>{0x89, 0x50}).failed());

    // 签名错误。
    auto badSignature = makePngHeader(64, 64);
    badSignature[0] = 0x00;
    EXPECT_TRUE(parsePngInfo(badSignature).failed());

    // IHDR 长度非 13。
    auto badLength = makePngHeader(64, 64);
    badLength[11] = 0x0C;
    EXPECT_TRUE(parsePngInfo(badLength).failed());

    // IHDR 类型错误。
    auto badType = makePngHeader(64, 64);
    badType[12] = 'X';
    EXPECT_TRUE(parsePngInfo(badType).failed());
}
