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

#include "common/network/crypto/RsaHandshake.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <memory>

namespace mc::network::crypto {

namespace {

// OpenSSL 3.x 用 EVP_PKEY 系列；RSA_* 旧 API 已 deprecated。统一走 EVP。

struct EvpPkeyDeleter {
    void operator()(EVP_PKEY* p) const
    {
        if (p != nullptr) {
            EVP_PKEY_free(p);
        }
    }
};
using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;

struct EvpPkeyCtxDeleter {
    void operator()(EVP_PKEY_CTX* p) const
    {
        if (p != nullptr) {
            EVP_PKEY_CTX_free(p);
        }
    }
};
using EvpPkeyCtxPtr = std::unique_ptr<EVP_PKEY_CTX, EvpPkeyCtxDeleter>;

/**
 * @brief 从 X509 DER 公钥解码出 EVP_PKEY
 */
Result<EvpPkeyPtr> loadPublicKey(const u8* der, usize size)
{
    const u8* cursor = der;
    EVP_PKEY* pkey = d2i_PUBKEY(nullptr, &cursor, static_cast<long>(size));
    if (pkey == nullptr) {
        return Error(ErrorCode::InvalidData, "X509 公钥 DER 解码失败", "RsaHandshake::loadPublicKey");
    }
    return EvpPkeyPtr(pkey);
}

/**
 * @brief 从 PKCS8 DER 私钥解码出 EVP_PKEY
 */
Result<EvpPkeyPtr> loadPrivateKey(const u8* der, usize size)
{
    const u8* cursor = der;
    // d2i_AutoPrivateKey 接受 PKCS8 或传统 PKCS1 私钥 DER。
    EVP_PKEY* pkey = d2i_AutoPrivateKey(nullptr, &cursor, static_cast<long>(size));
    if (pkey == nullptr) {
        return Error(ErrorCode::InvalidData, "PKCS8 私钥 DER 解码失败", "RsaHandshake::loadPrivateKey");
    }
    return EvpPkeyPtr(pkey);
}

} // namespace

Result<RsaHandshake::KeyPair> RsaHandshake::generateKeyPair()
{
    EvpPkeyCtxPtr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
    if (ctx == nullptr) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_CTX_new_id 失败", "RsaHandshake::generateKeyPair");
    }
    if (EVP_PKEY_keygen_init(ctx.get()) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_keygen_init 失败", "RsaHandshake::generateKeyPair");
    }
    // 1024 位，对齐 MC Java NetworkEncryptionUtils 的 RSA key size。
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 1024) <= 0) {
        return Error(ErrorCode::Unknown, "set_rsa_keygen_bits 失败", "RsaHandshake::generateKeyPair");
    }
    EVP_PKEY* rawPkey = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &rawPkey) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_keygen 失败", "RsaHandshake::generateKeyPair");
    }
    EvpPkeyPtr pkey(rawPkey);

    // 编码公钥为 X509 SubjectPublicKeyInfo DER。
    u8* pubDer = nullptr;
    int pubLen = i2d_PUBKEY(pkey.get(), &pubDer);
    if (pubLen <= 0) {
        return Error(ErrorCode::Unknown, "i2d_PUBKEY 失败", "RsaHandshake::generateKeyPair");
    }
    KeyPair kp;
    kp.publicKeyDer.assign(pubDer, pubDer + pubLen);
    OPENSSL_free(pubDer);

    // 编码私钥为 PKCS8 DER。
    u8* privDer = nullptr;
    int privLen = i2d_PrivateKey(pkey.get(), &privDer);
    if (privLen <= 0) {
        OPENSSL_free(privDer);
        return Error(ErrorCode::Unknown, "i2d_PrivateKey 失败", "RsaHandshake::generateKeyPair");
    }
    kp.privateKeyDer.assign(privDer, privDer + privLen);
    OPENSSL_free(privDer);

    return kp;
}

Result<std::vector<u8>> RsaHandshake::encryptWithPublicKey(
    const std::vector<u8>& publicKeyDer, const u8* plaintext, usize size)
{
    auto keyResult = loadPublicKey(publicKeyDer.data(), publicKeyDer.size());
    if (!keyResult.success()) {
        return keyResult.error();
    }
    EvpPkeyPtr pkey = std::move(keyResult).value();

    EvpPkeyCtxPtr ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
    if (ctx == nullptr) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_CTX_new 失败", "RsaHandshake::encryptWithPublicKey");
    }
    if (EVP_PKEY_encrypt_init(ctx.get()) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_encrypt_init 失败", "RsaHandshake::encryptWithPublicKey");
    }
    // PKCS1 padding（对齐 Java "RSA" 默认 RSA/ECB/PKCS1Padding）。
    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0) {
        return Error(ErrorCode::Unknown, "set_rsa_padding 失败", "RsaHandshake::encryptWithPublicKey");
    }

    size_t outLen = 0;
    if (EVP_PKEY_encrypt(ctx.get(), nullptr, &outLen, plaintext, size) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_encrypt 探测长度失败", "RsaHandshake::encryptWithPublicKey");
    }
    std::vector<u8> out(outLen);
    if (EVP_PKEY_encrypt(ctx.get(), out.data(), &outLen, plaintext, size) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_encrypt 失败", "RsaHandshake::encryptWithPublicKey");
    }
    out.resize(outLen);
    return out;
}

Result<std::vector<u8>> RsaHandshake::decryptWithPrivateKey(
    const std::vector<u8>& privateKeyDer, const u8* ciphertext, usize size)
{
    auto keyResult = loadPrivateKey(privateKeyDer.data(), privateKeyDer.size());
    if (!keyResult.success()) {
        return keyResult.error();
    }
    EvpPkeyPtr pkey = std::move(keyResult).value();

    EvpPkeyCtxPtr ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
    if (ctx == nullptr) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_CTX_new 失败", "RsaHandshake::decryptWithPrivateKey");
    }
    if (EVP_PKEY_decrypt_init(ctx.get()) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_decrypt_init 失败", "RsaHandshake::decryptWithPrivateKey");
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0) {
        return Error(ErrorCode::Unknown, "set_rsa_padding 失败", "RsaHandshake::decryptWithPrivateKey");
    }

    size_t outLen = 0;
    if (EVP_PKEY_decrypt(ctx.get(), nullptr, &outLen, ciphertext, size) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_decrypt 探测长度失败", "RsaHandshake::decryptWithPrivateKey");
    }
    std::vector<u8> out(outLen);
    if (EVP_PKEY_decrypt(ctx.get(), out.data(), &outLen, ciphertext, size) <= 0) {
        return Error(ErrorCode::Unknown, "EVP_PKEY_decrypt 失败", "RsaHandshake::decryptWithPrivateKey");
    }
    out.resize(outLen);
    return out;
}

} // namespace mc::network::crypto
