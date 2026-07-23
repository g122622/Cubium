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

#include <openssl/evp.h>

namespace mc::network::crypto {

namespace {

/**
 * @brief 构造一个方向的 CFB8 cipher ctx
 *
 * IV 取密钥本身（Java: new IvParameterSpec(key.getEncoded())）。
 * CFB8 流式：不调 EVP_EncryptFinal（无 padding 膨胀）。
 */
Result<void*> createCtx(bool encrypt, const u8* keyBytes)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        return Error(ErrorCode::Unknown, "EVP_CIPHER_CTX_new 失败", "AesCfb8::createCtx");
    }

    const EVP_CIPHER* cipher = EVP_aes_128_cfb8();
    // IV 用密钥本身（16 字节）。EVP_EncryptInit_ex 内部拷贝 key/iv，调用后可释放。
    if (EVP_CipherInit_ex(ctx, cipher, nullptr, keyBytes, keyBytes, encrypt ? 1 : 0) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error(ErrorCode::Unknown, "EVP_CipherInit_ex 失败", "AesCfb8::createCtx");
    }
    // 关闭 padding（CFB8 流式无填充）。
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    return static_cast<void*>(ctx);
}

} // namespace

Result<AesCfb8> AesCfb8::forEncryption(const std::array<u8, kSharedSecretBytes>& key)
{
    AesCfb8 out;
    auto r = createCtx(true, key.data());
    if (!r.success()) {
        return r.error();
    }
    out.m_ctx = r.value();
    out.m_encrypt = true;
    return out;
}

Result<AesCfb8> AesCfb8::forDecryption(const std::array<u8, kSharedSecretBytes>& key)
{
    AesCfb8 out;
    auto r = createCtx(false, key.data());
    if (!r.success()) {
        return r.error();
    }
    out.m_ctx = r.value();
    out.m_encrypt = false;
    return out;
}

AesCfb8::~AesCfb8()
{
    if (m_ctx != nullptr) {
        EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX*>(m_ctx));
        m_ctx = nullptr;
    }
}

AesCfb8::AesCfb8(AesCfb8&& other) noexcept
    : m_ctx(other.m_ctx)
    , m_encrypt(other.m_encrypt)
{
    other.m_ctx = nullptr;
}

AesCfb8& AesCfb8::operator=(AesCfb8&& other) noexcept
{
    if (this != &other) {
        if (m_ctx != nullptr) {
            EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX*>(m_ctx));
        }
        m_ctx = other.m_ctx;
        m_encrypt = other.m_encrypt;
        other.m_ctx = nullptr;
    }
    return *this;
}

Result<std::vector<u8>> AesCfb8::process(const u8* data, usize size)
{
    if (m_ctx == nullptr) {
        return Error(ErrorCode::InvalidState, "CFB8 ctx 未初始化", "AesCfb8::process");
    }
    if (size == 0) {
        return std::vector<u8>{};
    }

    std::vector<u8> out(size);
    // CFB8 流式：输入 n 字节产出 n 字节。EVP_CipherUpdate 输出长度 ≤ inl + block，CFB8 无膨胀。
    int outLen = 0;
    if (EVP_CipherUpdate(static_cast<EVP_CIPHER_CTX*>(m_ctx), out.data(), &outLen, data, static_cast<int>(size)) != 1) {
        return Error(ErrorCode::Unknown, "EVP_CipherUpdate 失败", "AesCfb8::process");
    }
    // CFB8 不调 EVP_CipherFinal（无 padding）。outLen 应等于 size。
    if (static_cast<usize>(outLen) != size) {
        out.resize(static_cast<usize>(outLen));
    }
    return out;
}

} // namespace mc::network::crypto
