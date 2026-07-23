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

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacketBase.hpp"

#include <array>
#include <string>
#include <vector>

namespace mc::network::ir::login {

/**
 * @brief Hello（C→S，登录开始）
 *
 * 客户端发送用户名（离线模式据此生成 UUID）与可选 profile 公钥（在线模式）。
 */
struct Hello {
    std::string username;
    std::optional<std::array<u8, 32>> publicKey; // 在线模式玩家 profile 公钥；离线模式 nullopt
    std::optional<u64> keySignature;
    BedrockMeta bedrock{};
};

/**
 * @brief Hello（S→C，服务端登录开始）
 *
 * 服务端发 serverId（空串=离线模式/ECDH）+ RSA 公钥 + verify token。
 * 客户端用此公钥加密共享密钥回 Key。
 */
struct HelloBound {
    std::string serverId;
    std::vector<u8> publicKey; // RSA 公钥 DER
    std::vector<u8> verifyToken;
    bool shouldAuthenticate;
    BedrockMeta bedrock{};
};

/**
 * @brief Key（C→S，加密响应）
 *
 * 客户端发服务端 RSA 公钥加密的共享密钥 + 加密的 verify token。
 * 服务端解密后开启 AES-CFB8 加密。这是 terminal 包（处理后 login 阶段切换）。
 */
struct Key {
    static constexpr bool kTerminal = true;

    std::vector<u8> encryptedSharedSecret;
    std::vector<u8> encryptedVerifyToken;
    BedrockMeta bedrock{};
};

/**
 * @brief LoginFinished（S→C，登录成功，terminal）
 *
 * 服务端发玩家 GameProfile（UUID + name + properties）。处理后切到 Configuration 阶段。
 */
struct LoginFinished {
    static constexpr bool kTerminal = true;

    std::array<u8, 16> uuid;
    std::string username;
    std::vector<std::pair<std::string, std::string>> properties; // name→value（省略 signature）
    BedrockMeta bedrock{};
};

/**
 * @brief LoginCompression（S→C，启用压缩通知）
 *
 * 服务端发压缩阈值 threshold；后续包长度 < threshold 不压缩，>= threshold 压缩。
 * 阈值 -1 表示禁用压缩。
 */
struct LoginCompression {
    i32 threshold;
    BedrockMeta bedrock{};
};

/**
 * @brief LoginAcknowledged（C→S，确认登录完成，terminal）
 *
 * 客户端收到 LoginFinished 后回此包，确认进入 Configuration 阶段。
 */
struct LoginAcknowledged {
    static constexpr bool kTerminal = true;

    BedrockMeta bedrock{};
};

/**
 * @brief Disconnect（S→C，登录阶段踢出）
 */
struct Disconnect {
    std::string reason; // JSON 文本组件
    BedrockMeta bedrock{};
};

} // namespace mc::network::ir::login
