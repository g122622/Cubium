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
#include "common/network/crypto/RsaHandshake.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace mc::network::crypto;
using namespace mc;

namespace {

// 生成一对密钥供多个测试复用（RSA keygen 较慢，避免每个 TEST 重复生成）。
struct RsaFixture {
    RsaHandshake::KeyPair kp;
    static RsaFixture& instance()
    {
        static RsaFixture f = []() {
            RsaFixture out;
            auto r = RsaHandshake::generateKeyPair();
            EXPECT_TRUE(r.success()) << "RSA keygen 失败";
            out.kp = std::move(r).value();
            return out;
        }();
        return f;
    }
};

} // namespace

TEST(RsaHandshake, GenerateKeyPairProducesNonEmptyDer)
{
    const auto& kp = RsaFixture::instance().kp;
    EXPECT_FALSE(kp.publicKeyDer.empty());
    EXPECT_FALSE(kp.privateKeyDer.empty());
    // X509 公钥 DER 通常 ~140-160 字节；PKCS8 私钥 DER ~640 字节
    EXPECT_GT(kp.publicKeyDer.size(), 100u);
    EXPECT_GT(kp.privateKeyDer.size(), 300u);
}

TEST(RsaHandshake, EncryptDecryptRoundTrip)
{
    const auto& kp = RsaFixture::instance().kp;
    std::vector<u8> plaintext(32, 0);
    for (usize i = 0; i < 32; ++i) {
        plaintext[i] = static_cast<u8>(i);
    }

    auto cipher = RsaHandshake::encryptWithPublicKey(kp.publicKeyDer, plaintext.data(), plaintext.size());
    ASSERT_TRUE(cipher.success());
    // RSA-1024 密文固定 128 字节
    EXPECT_EQ(cipher.value().size(), 128u);

    auto plain = RsaHandshake::decryptWithPrivateKey(kp.privateKeyDer, cipher.value().data(), cipher.value().size());
    ASSERT_TRUE(plain.success());
    EXPECT_EQ(plain.value(), plaintext);
}

TEST(RsaHandshake, CiphertextIs128Bytes)
{
    // 验证 RSA-1024 PKCS1 密文恒为 128 字节（密钥长度 / 8）
    const auto& kp = RsaFixture::instance().kp;
    std::vector<u8> small(16, 0xAB);
    auto cipher = RsaHandshake::encryptWithPublicKey(kp.publicKeyDer, small.data(), small.size());
    ASSERT_TRUE(cipher.success());
    EXPECT_EQ(cipher.value().size(), 128u);
}

TEST(RsaHandshake, WrongPrivateKeyFailsOrGarbage)
{
    // 用第二对密钥的私钥解密第一对加密的密文，应失败（PKCS1 padding 校验）或解出垃圾
    auto kp2 = RsaHandshake::generateKeyPair();
    ASSERT_TRUE(kp2.success());
    const auto& kp1 = RsaFixture::instance().kp;

    std::vector<u8> plaintext(16, 0x55);
    auto cipher = RsaHandshake::encryptWithPublicKey(kp1.publicKeyDer, plaintext.data(), plaintext.size());
    ASSERT_TRUE(cipher.success());

    auto plain =
        RsaHandshake::decryptWithPrivateKey(kp2.value().privateKeyDer, cipher.value().data(), cipher.value().size());
    // PKCS1 padding 校验失败 → 错误返回；偶发"伪成功"也是垃圾而非原文
    if (plain.success()) {
        EXPECT_NE(plain.value(), plaintext);
    } else {
        EXPECT_EQ(plain.error().code(), ErrorCode::Unknown);
    }
}

TEST(RsaHandshake, PlaintextOverPkcs1LimitRejected)
{
    // RSA-1024 PKCS1 最大明文 = 128 - 11 = 117 字节；超出应加密失败
    const auto& kp = RsaFixture::instance().kp;
    std::vector<u8> tooBig(118, 0x42); // 117 是上限，118 超限
    auto cipher = RsaHandshake::encryptWithPublicKey(kp.publicKeyDer, tooBig.data(), tooBig.size());
    ASSERT_FALSE(cipher.success());
}

TEST(RsaHandshake, MaxAllowedPlaintextRoundTrips)
{
    // 117 字节（PKCS1 上限）应正常往返
    const auto& kp = RsaFixture::instance().kp;
    std::vector<u8> maxPlaintext(117);
    for (usize i = 0; i < 117; ++i) {
        maxPlaintext[i] = static_cast<u8>(i);
    }
    auto cipher = RsaHandshake::encryptWithPublicKey(kp.publicKeyDer, maxPlaintext.data(), maxPlaintext.size());
    ASSERT_TRUE(cipher.success());
    auto plain = RsaHandshake::decryptWithPrivateKey(kp.privateKeyDer, cipher.value().data(), cipher.value().size());
    ASSERT_TRUE(plain.success());
    EXPECT_EQ(plain.value(), maxPlaintext);
}

TEST(RsaHandshake, SharedSecretIs16Bytes)
{
    auto secret = generateSharedSecret();
    ASSERT_TRUE(secret.success());
    EXPECT_EQ(secret.value().size(), kSharedSecretBytes);
}
