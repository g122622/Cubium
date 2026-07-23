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

#include <vector>

namespace mc::network::crypto {

/**
 * @brief RSA-1024/PKCS1 登录握手加解密
 *
 * 对应 MC Java net.minecraft.util.Crypt 的 RSA 部分：
 * - 密钥长度 1024 位（Java: KeyPairGenerator("RSA").initialize(1024)）。
 * - 公钥 X509 编码（getPublic().getEncoded()），私钥 PKCS8。
 * - padding：Java "RSA" 裸算法名在 SunJCE 下等价 RSA/ECB/PKCS1Padding，
 *   故 OpenSSL 用 RSA_PKCS1_PADDING。1024 位密钥密文固定 128 字节。
 *
 * 离线模式默认不走加密握手；真 Java 在线互通时服务端发公钥、客户端加密共享密钥回传。
 * 本类封装：生成密钥对、公钥 DER 编解码、公钥加密、私钥解密。
 */
class RsaHandshake {
public:
    /**
     * @brief 生成 1024 位 RSA 密钥对
     *
     * @return publicKeyDer / privateKeyDer 两段 DER
     */
    struct KeyPair {
        std::vector<u8> publicKeyDer;  // X509 SubjectPublicKeyInfo
        std::vector<u8> privateKeyDer; // PKCS8
    };

    [[nodiscard]] static Result<KeyPair> generateKeyPair();

    /**
     * @brief 用公钥 DER 加密明文（客户端用，加密共享密钥/verify token）
     *
     * @param publicKeyDer X509 编码公钥
     * @param plaintext 待加密字节（须 < 密钥字节数 - 11，PKCS1 padding 开销）
     * @return 密文（1024 位密钥下 128 字节）
     */
    [[nodiscard]] static Result<std::vector<u8>> encryptWithPublicKey(
        const std::vector<u8>& publicKeyDer, const u8* plaintext, usize size);

    /**
     * @brief 用私钥 DER 解密密文（服务端用，解出共享密钥/verify token）
     *
     * @param privateKeyDer PKCS8 编码私钥
     * @param ciphertext 密文（1024 位密钥下 128 字节）
     * @return 明文
     */
    [[nodiscard]] static Result<std::vector<u8>> decryptWithPrivateKey(
        const std::vector<u8>& privateKeyDer, const u8* ciphertext, usize size);
};

} // namespace mc::network::crypto
