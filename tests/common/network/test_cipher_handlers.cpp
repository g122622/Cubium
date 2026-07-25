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

#include "common/network/crypto/Crypt.hpp"
#include "common/network/pipeline/CipherHandlers.hpp"

#include <gtest/gtest.h>

#include <array>
#include <vector>

using namespace mc::network::pipeline;
using namespace mc::network::crypto;
using namespace mc;

namespace {

constexpr std::array<u8, kSharedSecretBytes> kKey = {
    0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98, 0xA9, 0xBA, 0xCB, 0xDC, 0xED, 0xFE, 0x0F};

std::vector<u8> makeData(usize n)
{
    std::vector<u8> d(n);
    for (usize i = 0; i < n; ++i) {
        d[i] = static_cast<u8>(i * 11 + 5);
    }
    return d;
}

} // namespace

TEST(CipherHandlers, RoundTrip512Bytes)
{
    CipherEncoder enc;
    CipherDecoder dec;
    ASSERT_TRUE(enc.init(kKey).success());
    ASSERT_TRUE(dec.init(kKey).success());

    auto plain = makeData(512);
    std::vector<u8> cipher;
    ASSERT_TRUE(enc.encode(plain, cipher).success());
    EXPECT_EQ(cipher.size(), 512u);

    std::vector<u8> restored;
    ASSERT_TRUE(dec.decode(cipher, restored).success());
    EXPECT_EQ(restored, plain);
}

TEST(CipherHandlers, InactiveBeforeInitPassesThrough)
{
    // 未 init：明文直通（离线模式）
    CipherEncoder enc;
    CipherDecoder dec;
    EXPECT_FALSE(enc.isActive());
    EXPECT_FALSE(dec.isActive());

    auto plain = makeData(64);
    std::vector<u8> out;
    ASSERT_TRUE(enc.encode(plain, out).success());
    EXPECT_EQ(out, plain); // 直通不变
}

TEST(CipherHandlers, ActiveAfterInit)
{
    CipherEncoder enc;
    EXPECT_FALSE(enc.isActive());
    ASSERT_TRUE(enc.init(kKey).success());
    EXPECT_TRUE(enc.isActive());
}

TEST(CipherHandlers, StreamContinuity)
{
    // 两块顺序加密 + 顺序解密 == 拼接后往返（CFB8 流式跨调用保持）
    CipherEncoder enc;
    CipherDecoder dec;
    ASSERT_TRUE(enc.init(kKey).success());
    ASSERT_TRUE(dec.init(kKey).success());

    auto b1 = makeData(100);
    auto b2 = makeData(60);
    std::vector<u8> c1, c2;
    ASSERT_TRUE(enc.encode(b1, c1).success());
    ASSERT_TRUE(enc.encode(b2, c2).success());

    std::vector<u8> r1, r2;
    ASSERT_TRUE(dec.decode(c1, r1).success());
    ASSERT_TRUE(dec.decode(c2, r2).success());
    EXPECT_EQ(r1, b1);
    EXPECT_EQ(r2, b2);
}

TEST(CipherHandlers, EmptyInput)
{
    CipherEncoder enc;
    CipherDecoder dec;
    ASSERT_TRUE(enc.init(kKey).success());
    ASSERT_TRUE(dec.init(kKey).success());

    std::vector<u8> empty;
    std::vector<u8> cipher;
    ASSERT_TRUE(enc.encode(empty, cipher).success());
    EXPECT_TRUE(cipher.empty());

    std::vector<u8> restored;
    ASSERT_TRUE(dec.decode(cipher, restored).success());
    EXPECT_TRUE(restored.empty());
}

TEST(CipherHandlers, WrongKeyProducesGarbage)
{
    CipherEncoder enc;
    ASSERT_TRUE(enc.init(kKey).success());

    std::array<u8, kSharedSecretBytes> wrongKey = kKey;
    wrongKey[5] ^= 0xFF;
    CipherDecoder dec;
    ASSERT_TRUE(dec.init(wrongKey).success());

    auto plain = makeData(32);
    std::vector<u8> cipher;
    ASSERT_TRUE(enc.encode(plain, cipher).success());
    std::vector<u8> restored;
    ASSERT_TRUE(dec.decode(cipher, restored).success());
    EXPECT_NE(restored, plain);
}
