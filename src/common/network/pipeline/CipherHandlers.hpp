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
#include "common/network/crypto/AesCfb8.hpp"
#include "common/network/crypto/Crypt.hpp"

#include <vector>

namespace mc::network::pipeline {

/**
 * @brief 加密层 handler（出站：明文 → AES-CFB8 密文）
 *
 * 对应 MC Java CipherEncoder。持有 AesCfb8（加密方向）实例，流式状态跨包保持。
 * 装在压缩层之外：出站先压缩再加长度前缀再加密（加密最外层，含长度前缀）。
 *
 * 离线模式默认不装本 handler（明文 wire）。真 Java 在线互通时握手后装上。
 */
class CipherEncoder {
public:
    CipherEncoder() = default;

    /**
     * @brief 用共享密钥初始化加密 cipher
     */
    [[nodiscard]] Result<void> init(const std::array<u8, crypto::kSharedSecretBytes>& key);

    [[nodiscard]] bool isActive() const noexcept { return m_cipher.isValid(); }

    /**
     * @brief 加密 input，输出等长密文
     */
    [[nodiscard]] Result<void> encode(const std::vector<u8>& input, std::vector<u8>& output);

private:
    crypto::AesCfb8 m_cipher;
};

/**
 * @brief 解密层 handler（入站：AES-CFB8 密文 → 明文）
 *
 * 对应 MC Java CipherDecoder。装在解帧之前：入站先解密再解帧再解压。
 */
class CipherDecoder {
public:
    CipherDecoder() = default;

    [[nodiscard]] Result<void> init(const std::array<u8, crypto::kSharedSecretBytes>& key);

    [[nodiscard]] bool isActive() const noexcept { return m_cipher.isValid(); }

    [[nodiscard]] Result<void> decode(const std::vector<u8>& input, std::vector<u8>& output);

private:
    crypto::AesCfb8 m_cipher;
};

} // namespace mc::network::pipeline
