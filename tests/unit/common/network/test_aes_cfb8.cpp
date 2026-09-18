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

#include "common/network/crypto/AesCfb8.hpp"
#include "common/network/crypto/Crypt.hpp"

#include <gtest/gtest.h>

#include <array>
#include <vector>

using namespace mc::network::crypto;
using namespace mc;

namespace {

constexpr std::array<u8, kSharedSecretBytes> kKey = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

std::vector<u8> makeData(usize n)
{
    std::vector<u8> data(n);
    for (usize i = 0; i < n; ++i) {
        data[i] = static_cast<u8>(i * 7 + 3);
    }
    return data;
}

} // namespace

TEST(AesCfb8, RoundTrip256Bytes)
{
    auto enc = AesCfb8::forEncryption(kKey);
    auto dec = AesCfb8::forDecryption(kKey);
    ASSERT_TRUE(enc.success());
    ASSERT_TRUE(dec.success());

    auto data = makeData(256);
    auto cipher = enc.value().process(data.data(), data.size());
    ASSERT_TRUE(cipher.success());
    EXPECT_EQ(cipher.value().size(), 256u);

    auto plain = dec.value().process(cipher.value().data(), cipher.value().size());
    ASSERT_TRUE(plain.success());
    EXPECT_EQ(plain.value(), data);
}

TEST(AesCfb8, StreamContinuityAcrossBlocks)
{
    // 两块顺序加密 == 拼接后一次加密（CFB8 流式状态跨 process 保持）
    auto enc1 = AesCfb8::forEncryption(kKey);
    auto enc2 = AesCfb8::forEncryption(kKey);
    ASSERT_TRUE(enc1.success() && enc2.success());

    auto block1 = makeData(100);
    auto block2 = makeData(60);
    auto concat = block1;
    concat.insert(concat.end(), block2.begin(), block2.end());

    auto c1 = enc1.value().process(block1.data(), block1.size());
    auto c2 = enc1.value().process(block2.data(), block2.size());
    ASSERT_TRUE(c1.success() && c2.success());

    auto cAll = enc2.value().process(concat.data(), concat.size());
    ASSERT_TRUE(cAll.success());

    std::vector<u8> split;
    split.insert(split.end(), c1.value().begin(), c1.value().end());
    split.insert(split.end(), c2.value().begin(), c2.value().end());
    EXPECT_EQ(split, cAll.value());
}

TEST(AesCfb8, EmptyInputReturnsEmpty)
{
    auto enc = AesCfb8::forEncryption(kKey);
    ASSERT_TRUE(enc.success());
    auto r = enc.value().process(nullptr, 0);
    ASSERT_TRUE(r.success());
    EXPECT_TRUE(r.value().empty());
}

TEST(AesCfb8, BlockBoundarySizes)
{
    // CFB8 字节流，无需块对齐：1/15/16/17/31/32/33 字节均应正常往返
    const usize sizes[] = {1, 15, 16, 17, 31, 32, 33};
    for (usize n : sizes) {
        auto enc = AesCfb8::forEncryption(kKey);
        auto dec = AesCfb8::forDecryption(kKey);
        ASSERT_TRUE(enc.success() && dec.success());

        auto data = makeData(n);
        auto cipher = enc.value().process(data.data(), data.size());
        ASSERT_TRUE(cipher.success()) << "encrypt size=" << n;
        ASSERT_EQ(cipher.value().size(), n);

        auto plain = dec.value().process(cipher.value().data(), cipher.value().size());
        ASSERT_TRUE(plain.success()) << "decrypt size=" << n;
        EXPECT_EQ(plain.value(), data) << "roundtrip size=" << n;
    }
}

TEST(AesCfb8, WrongKeyProducesGarbageNotError)
{
    auto enc = AesCfb8::forEncryption(kKey);
    std::array<u8, kSharedSecretBytes> wrongKey = kKey;
    wrongKey[0] ^= 0xFF;
    auto dec = AesCfb8::forDecryption(wrongKey);
    ASSERT_TRUE(enc.success() && dec.success());

    auto data = makeData(32);
    auto cipher = enc.value().process(data.data(), data.size());
    ASSERT_TRUE(cipher.success());
    auto plain = dec.value().process(cipher.value().data(), cipher.value().size());
    ASSERT_TRUE(plain.success());
    // 错 key 解出垃圾（非错误返回），且不等于原文
    EXPECT_NE(plain.value(), data);
}

TEST(AesCfb8, SharedSecretIs16Bytes)
{
    EXPECT_EQ(kSharedSecretBytes, 16u);
}
