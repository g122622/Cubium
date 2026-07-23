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

#include <array>
#include <vector>

namespace mc::network::crypto {

/**
 * @brief Java 1.21.11 登录加密公共常量与共享密钥工具
 *
 * 对应 MC Java net.minecraft.util.Crypt 的共享密钥生成部分：
 * - 共享密钥 16 字节（AES-128），客户端随机生成，用服务端 RSA 公钥加密回传。
 * - 离线模式跳过加密握手，本工具仅提供密钥生成（真 Java 在线互通时用）。
 */
inline constexpr usize kSharedSecretBytes = 16;

/**
 * @brief 生成 16 字节随机 AES 共享密钥
 *
 * 用 OpenSSL RAND_bytes 生成密码学安全随机字节。失败返回错误。
 */
[[nodiscard]] Result<std::array<u8, kSharedSecretBytes>> generateSharedSecret();

/**
 * @brief 生成 n 字节密码学安全随机字节（verify token 等）
 */
[[nodiscard]] Result<std::vector<u8>> generateRandomBytes(usize n);

} // namespace mc::network::crypto
