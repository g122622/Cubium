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
#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/crypto/Crypt.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mc::network::backend::java {

/**
 * @brief Java 1.21.11 登录握手编排器
 *
 * 编排 Handshaking→Login→Configuration 的握手流程，产出/消费 ir::login / ir::handshake 包。
 * 加密层（RSA+AES-CFB8）与压缩层由 Connection 装入；本类只决定"发什么包、收到包后下一步"。
 *
 * 两种模式：
 * - 离线模式（我方服务端默认）：跳过 RSA 握手，明文 wire。流程：
 *     客户端: ClientIntention(LOGIN) → Hello(username) → [收 LoginCompression] →
 *             [收 LoginFinished] → LoginAcknowledged
 *     服务端: [收 ClientIntention] → [收 Hello] → LoginCompression → LoginFinished
 *             → [收 LoginAcknowledged] → Configuration
 * - 在线模式（真 Java 互通，尽力）：经 RSA 公钥加密共享密钥，AES-CFB8 加密后续流量。
 *     客户端: Hello → [收 HelloBound(publicKey, verifyToken)] →
 *             Key(RSA(secret), RSA(verifyToken)) → 装加密层 → ...
 *     服务端: Hello → HelloBound(publicKey, verifyToken) → [收 Key] →
 *             解密校验 verifyToken → 装加密层 → ...
 *
 * TODO(Phase7): 接入 Connection 端到端跑通（离线模式我方互通必达；在线模式尽力）。
 */
class JavaLoginHandshaker {
public:
    /**
     * @brief 客户端：构造握手包 ClientIntention（intent=LOGIN=2）
     */
    [[nodiscard]] static ir::IrPacket buildClientIntention(const std::string& host, u16 port);

    /**
     * @brief 客户端：构造 Hello（用户名 + 零 UUID 占位）
     */
    [[nodiscard]] static ir::IrPacket buildHello(const std::string& username);

    /**
     * @brief 服务端：构造 LoginCompression（threshold，256 对齐原版；-1 禁用）
     */
    [[nodiscard]] static ir::IrPacket buildLoginCompression(i32 threshold);

    /**
     * @brief 服务端：构造 LoginFinished（离线 UUID + username）
     *
     * @param uuid 离线模式由用户名生成的 UUID（16 字节）
     */
    [[nodiscard]] static ir::IrPacket buildLoginFinished(const std::array<u8, 16>& uuid, const std::string& username);

    /**
     * @brief 客户端：构造 LoginAcknowledged（确认登录完成，切 Configuration）
     */
    [[nodiscard]] static ir::IrPacket buildLoginAcknowledged();

    // === 在线模式加密握手（真 Java 互通，尽力）===

    /**
     * @brief 服务端：生成 RSA 密钥对 + verify token，构造 HelloBound
     *
     * @param shouldAuthenticate true=在线模式（客户端须 joinServer）
     * @return HelloBound 包 + 内部 RSA 私钥（服务端解 Key 用）
     */
    struct ServerEncryptionStart {
        ir::IrPacket helloBound;
        std::vector<u8> serverPrivateKeyDer; // PKCS8，handleKey 时解密用
        std::vector<u8> verifyToken;         // 原始 verify token，校验用
    };
    [[nodiscard]] static Result<ServerEncryptionStart> buildHelloBound(bool shouldAuthenticate);

    /**
     * @brief 客户端：收到 HelloBound 后，生成共享密钥 + 用公钥加密，构造 Key 包
     *
     * @param helloBound 收到的服务端 HelloBound
     * @return Key 包 + 共享密钥（装入 Connection 加密层用）
     */
    struct ClientEncryptionResponse {
        ir::IrPacket keyPacket;
        std::array<u8, crypto::kSharedSecretBytes> sharedSecret;
    };
    [[nodiscard]] static Result<ClientEncryptionResponse> handleHelloBound(const ir::login::HelloBound& helloBound);

    /**
     * @brief 服务端：收到 Key 后，用私钥解出共享密钥 + 校验 verify token
     *
     * @param keyPacket 客户端 Key 包
     * @param serverPrivateKeyDer 服务端 PKCS8 私钥
     * @param expectedVerifyToken 原始 verify token（HelloBound 时生成的）
     * @return 共享密钥（装入 Connection 加密层用）；校验失败返回错误
     */
    [[nodiscard]] static Result<std::array<u8, crypto::kSharedSecretBytes>> handleKey(const ir::login::Key& keyPacket,
        const std::vector<u8>& serverPrivateKeyDer,
        const std::vector<u8>& expectedVerifyToken);
};

} // namespace mc::network::backend::java
