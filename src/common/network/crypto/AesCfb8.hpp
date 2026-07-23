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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/crypto/Crypt.hpp"

#include <array>
#include <vector>

namespace mc::network::crypto {

/**
 * @brief AES-128-CFB8 流加解密
 *
 * 对应 MC Java Crypt.getCipher("AES/CFB8/NoPadding", key)：
 * - 算法 AES-128-CFB8（CFB8 反馈，每字节一单位，无 padding）。
 * - IV 与密钥相同（Java 用 new IvParameterSpec(key.getEncoded())）。
 * - 流式：Java 用 cipher.update 逐块推进，不调 doFinal；CFB8 天然流式，
 *   输入 n 字节产出 n 字节，无填充膨胀。
 *
 * 一个 AesCfb8 实例对应一个方向（加密或解密）。Connection 装两层：
 * 加密方向一个实例、解密方向一个实例，密钥相同。流式状态跨包保持
 * （CFB8 链式反馈），故实例生命周期与连接加密层一致，不可每包重建。
 *
 * 实现用 OpenSSL EVP_CIPHER_CTX 持久化流式状态，process() 多次调用续推。
 */
class AesCfb8 {
public:
    /**
     * @brief 加密方向构造
     *
     * @param key 16 字节 AES 密钥
     */
    [[nodiscard]] static Result<AesCfb8> forEncryption(const std::array<u8, kSharedSecretBytes>& key);

    /**
     * @brief 解密方向构造
     */
    [[nodiscard]] static Result<AesCfb8> forDecryption(const std::array<u8, kSharedSecretBytes>& key);

    AesCfb8() noexcept = default;
    ~AesCfb8();

    AesCfb8(AesCfb8&& other) noexcept;
    AesCfb8& operator=(AesCfb8&& other) noexcept;

    AesCfb8(const AesCfb8&) = delete;
    AesCfb8& operator=(const AesCfb8&) = delete;

    /**
     * @brief 流式处理：输入 data 产出等长输出（CFB8 无膨胀）
     *
     * 可多次调用续推流式状态。输出长度恒等于输入长度。
     */
    [[nodiscard]] Result<std::vector<u8>> process(const u8* data, usize size);

    [[nodiscard]] bool isValid() const noexcept { return m_ctx != nullptr; }

private:
    void* m_ctx = nullptr; // OpenSSL EVP_CIPHER_CTX*，用 void* 避免 header 泄露 OpenSSL
    bool m_encrypt = false;
};

} // namespace mc::network::crypto
