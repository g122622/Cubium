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

#include "common/network/backend/java/handshake/JavaLoginHandshaker.hpp"

#include "common/network/crypto/RsaHandshake.hpp"
#include "common/network/ir/packets/handshake/HandshakePackets.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/UuidUtils.hpp"

#include <algorithm>

namespace mc::network::backend::java {

namespace {

ir::IrPacket makePacket(protocol::ConnectionProtocol phase, ir::HandshakePacket p)
{
    return ir::IrPacket{phase, std::move(p)};
}
ir::IrPacket makePacket(protocol::ConnectionProtocol phase, ir::LoginPacket p)
{
    return ir::IrPacket{phase, std::move(p)};
}

} // namespace

ir::IrPacket JavaLoginHandshaker::buildClientIntention(const std::string& host, u16 port)
{
    ir::handshake::ClientIntention intention{};
    intention.protocolVersion = kJavaProtocolVersion; // 774
    intention.hostName = host;
    intention.port = port;
    intention.intendedState = 2; // LOGIN
    return makePacket(protocol::ConnectionProtocol::Handshaking, ir::HandshakePacket{std::move(intention)});
}

ir::IrPacket JavaLoginHandshaker::buildHello(const std::string& username)
{
    ir::login::Hello hello{};
    hello.username = username;
    return makePacket(protocol::ConnectionProtocol::Login, ir::LoginPacket{std::move(hello)});
}

ir::IrPacket JavaLoginHandshaker::buildLoginCompression(i32 threshold)
{
    ir::login::LoginCompression pkt{};
    pkt.threshold = threshold;
    return makePacket(protocol::ConnectionProtocol::Login, ir::LoginPacket{std::move(pkt)});
}

ir::IrPacket JavaLoginHandshaker::buildLoginFinished(const std::array<u8, 16>& uuid, const std::string& username)
{
    ir::login::LoginFinished pkt{};
    pkt.uuid = uuid;
    pkt.username = username;
    return makePacket(protocol::ConnectionProtocol::Login, ir::LoginPacket{std::move(pkt)});
}

ir::IrPacket JavaLoginHandshaker::buildLoginAcknowledged()
{
    ir::login::LoginAcknowledged pkt{};
    return makePacket(protocol::ConnectionProtocol::Login, ir::LoginPacket{std::move(pkt)});
}

Result<JavaLoginHandshaker::ServerEncryptionStart> JavaLoginHandshaker::buildHelloBound(bool shouldAuthenticate)
{
    auto keyPair = crypto::RsaHandshake::generateKeyPair();
    if (!keyPair.success()) {
        return keyPair.error();
    }
    auto verifyToken = crypto::generateRandomBytes(4); // 4 字节 verify token（对齐 Java）
    if (!verifyToken.success()) {
        return verifyToken.error();
    }

    ir::login::HelloBound helloBound{};
    helloBound.serverId = ""; // 离线 serverId 为空串
    helloBound.publicKey = keyPair.value().publicKeyDer;
    helloBound.verifyToken = verifyToken.value();
    helloBound.shouldAuthenticate = shouldAuthenticate;

    ServerEncryptionStart start;
    start.helloBound = makePacket(protocol::ConnectionProtocol::Login, ir::LoginPacket{std::move(helloBound)});
    start.serverPrivateKeyDer = std::move(keyPair.value().privateKeyDer);
    start.verifyToken = std::move(verifyToken.value());
    return start;
}

Result<JavaLoginHandshaker::ClientEncryptionResponse> JavaLoginHandshaker::handleHelloBound(
    const ir::login::HelloBound& helloBound)
{
    // 生成 16 字节共享密钥。
    auto secret = crypto::generateSharedSecret();
    if (!secret.success()) {
        return secret.error();
    }

    // 用服务端公钥加密共享密钥 + verify token。
    auto encSecret =
        crypto::RsaHandshake::encryptWithPublicKey(helloBound.publicKey, secret.value().data(), secret.value().size());
    if (!encSecret.success()) {
        return encSecret.error();
    }
    auto encToken = crypto::RsaHandshake::encryptWithPublicKey(
        helloBound.verifyToken, helloBound.verifyToken.data(), helloBound.verifyToken.size());
    if (!encToken.success()) {
        return encToken.error();
    }

    ir::login::Key key{};
    key.encryptedSharedSecret = std::move(encSecret.value());
    key.encryptedVerifyToken = std::move(encToken.value());

    ClientEncryptionResponse resp;
    resp.keyPacket = makePacket(protocol::ConnectionProtocol::Login, ir::LoginPacket{std::move(key)});
    resp.sharedSecret = secret.value();
    return resp;
}

Result<std::array<u8, crypto::kSharedSecretBytes>> JavaLoginHandshaker::handleKey(const ir::login::Key& keyPacket,
    const std::vector<u8>& serverPrivateKeyDer,
    const std::vector<u8>& expectedVerifyToken)
{
    // 用私钥解出共享密钥。
    auto secret = crypto::RsaHandshake::decryptWithPrivateKey(
        serverPrivateKeyDer, keyPacket.encryptedSharedSecret.data(), keyPacket.encryptedSharedSecret.size());
    if (!secret.success()) {
        return secret.error();
    }
    if (secret.value().size() != crypto::kSharedSecretBytes) {
        return Error(ErrorCode::InvalidData, "解出的共享密钥长度非 16 字节", "JavaLoginHandshaker::handleKey");
    }

    // 解出 verify token 并校验。
    auto token = crypto::RsaHandshake::decryptWithPrivateKey(
        serverPrivateKeyDer, keyPacket.encryptedVerifyToken.data(), keyPacket.encryptedVerifyToken.size());
    if (!token.success()) {
        return token.error();
    }
    if (token.value() != expectedVerifyToken) {
        return Error(ErrorCode::InvalidData, "verify token 校验失败", "JavaLoginHandshaker::handleKey");
    }

    std::array<u8, crypto::kSharedSecretBytes> out{};
    std::copy(secret.value().begin(), secret.value().end(), out.begin());
    return out;
}

} // namespace mc::network::backend::java
