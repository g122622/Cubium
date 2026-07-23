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

#include "common/network/backend/java/JavaProtocolTables.hpp"
#include "common/network/backend/java/codecs/JavaCodecs.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/PacketType.hpp"
#include "common/network/protocol/ProtocolInfoBuilder.hpp"

namespace mc::network::backend::java {

namespace {

using protocol::ConnectionProtocol;
using protocol::PacketFlow;
using protocol::PacketType;
using protocol::ProtocolInfoBuilder;

using B = buffer::RegistryByteBuf;

// ============================================================================
// 各阶段包表构建（addPacket 显式 id 严格对齐 GameProtocols.java 注册顺序）
// 在用包子集：只登记当前 IR 已有的包，id 与 Java 一致；未登记 id 解码报错由调用方跳过。
// ============================================================================

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::HandshakePacket>> buildHandshakeSb()
{
    ProtocolInfoBuilder<B, ir::HandshakePacket> b(ConnectionProtocol::Handshaking, PacketFlow::Serverbound);
    // id=0 ClientIntention（握手阶段唯一包）。altIndex 由 IrPacket.hpp variant 顺序定。
    b.addPacket<ir::handshake::ClientIntention>(
        0, PacketType{PacketFlow::Serverbound, "client_intention"}, 0, codecs::clientIntentionCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::StatusPacket>> buildStatusSb()
{
    ProtocolInfoBuilder<B, ir::StatusPacket> b(ConnectionProtocol::Status, PacketFlow::Serverbound);
    b.addPacket<ir::status::StatusRequest>(
        0, PacketType{PacketFlow::Serverbound, "status_request"}, 0, codecs::statusRequestCodec());
    b.addPacket<ir::status::PingRequest>(
        1, PacketType{PacketFlow::Serverbound, "ping_request"}, 2, codecs::pingRequestCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::StatusPacket>> buildStatusCb()
{
    ProtocolInfoBuilder<B, ir::StatusPacket> b(ConnectionProtocol::Status, PacketFlow::Clientbound);
    b.addPacket<ir::status::StatusResponse>(
        0, PacketType{PacketFlow::Clientbound, "status_response"}, 1, codecs::statusResponseCodec());
    b.addPacket<ir::status::PingResponse>(
        1, PacketType{PacketFlow::Clientbound, "pong_response"}, 3, codecs::pingResponseCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::LoginPacket>> buildLoginSb()
{
    ProtocolInfoBuilder<B, ir::LoginPacket> b(ConnectionProtocol::Login, PacketFlow::Serverbound);
    // LoginPacket variant: Hello(0) HelloBound(1) Key(2) LoginFinished(3) LoginCompression(4)
    //                     LoginAcknowledged(5) Disconnect(6)
    b.addPacket<ir::login::Hello>(0, PacketType{PacketFlow::Serverbound, "hello"}, 0, codecs::helloCodec());
    b.addPacket<ir::login::Key>(1, PacketType{PacketFlow::Serverbound, "key"}, 2, codecs::keyCodec());
    // id=2 custom_query_answer 未登记（IR 暂无），跳过保持 id 对齐。
    b.addPacket<ir::login::LoginAcknowledged>(
        3, PacketType{PacketFlow::Serverbound, "login_acknowledged"}, 5, codecs::loginAcknowledgedCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::LoginPacket>> buildLoginCb()
{
    ProtocolInfoBuilder<B, ir::LoginPacket> b(ConnectionProtocol::Login, PacketFlow::Clientbound);
    b.addPacket<ir::login::Disconnect>(
        0, PacketType{PacketFlow::Clientbound, "login_disconnect"}, 6, codecs::loginDisconnectCodec());
    b.addPacket<ir::login::HelloBound>(1, PacketType{PacketFlow::Clientbound, "hello"}, 1, codecs::helloBoundCodec());
    b.addPacket<ir::login::LoginFinished>(
        2, PacketType{PacketFlow::Clientbound, "login_finished"}, 3, codecs::loginFinishedCodec());
    b.addPacket<ir::login::LoginCompression>(
        3, PacketType{PacketFlow::Clientbound, "login_compression"}, 4, codecs::loginCompressionCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::ConfigurationPacket>> buildConfigurationSb()
{
    ProtocolInfoBuilder<B, ir::ConfigurationPacket> b(ConnectionProtocol::Configuration, PacketFlow::Serverbound);
    // ConfigurationPacket variant: RegistryData(0) FinishConfiguration(1)
    // Java Sb: finish_configuration id=3（前面 client_information/cookie/custom_payload 跳过）。
    b.addPacket<ir::configuration::FinishConfiguration>(
        3, PacketType{PacketFlow::Serverbound, "finish_configuration"}, 1, codecs::finishConfigurationCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::ConfigurationPacket>> buildConfigurationCb()
{
    ProtocolInfoBuilder<B, ir::ConfigurationPacket> b(ConnectionProtocol::Configuration, PacketFlow::Clientbound);
    // Java Cb: finish_configuration id=3, registry_data id=7。
    b.addPacket<ir::configuration::FinishConfiguration>(
        3, PacketType{PacketFlow::Clientbound, "finish_configuration"}, 1, codecs::finishConfigurationCodec());
    b.addPacket<ir::configuration::RegistryData>(
        7, PacketType{PacketFlow::Clientbound, "registry_data"}, 0, codecs::registryDataCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::PlayPacket>> buildPlaySb()
{
    ProtocolInfoBuilder<B, ir::PlayPacket> b(ConnectionProtocol::Play, PacketFlow::Serverbound);
    // PlayPacket variant: KeepAlive(0) Disconnect(1) MovePlayerPos(2) Chat(3)
    // Java Sb id: chat=8, keep_alive=27, move_player_pos=29。
    b.addPacket<ir::play::Chat>(8, PacketType{PacketFlow::Serverbound, "chat"}, 3, codecs::chatCodec());
    b.addPacket<ir::play::KeepAlive>(
        27, PacketType{PacketFlow::Serverbound, "keep_alive"}, 0, codecs::keepAliveCodec());
    b.addPacket<ir::play::MovePlayerPos>(
        29, PacketType{PacketFlow::Serverbound, "move_player_pos"}, 2, codecs::movePlayerPosCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::PlayPacket>> buildPlayCb()
{
    ProtocolInfoBuilder<B, ir::PlayPacket> b(ConnectionProtocol::Play, PacketFlow::Clientbound);
    // Java Cb id: disconnect=31, keep_alive=42。
    b.addPacket<ir::play::Disconnect>(
        31, PacketType{PacketFlow::Clientbound, "disconnect"}, 1, codecs::playDisconnectCodec());
    b.addPacket<ir::play::KeepAlive>(
        42, PacketType{PacketFlow::Clientbound, "keep_alive"}, 0, codecs::keepAliveCodec());
    return b.build();
}

} // namespace

std::shared_ptr<pipeline::ProtocolTableSet<B>> JavaProtocolTables::build()
{
    auto tables = std::make_shared<pipeline::ProtocolTableSet<B>>();
    tables->handshakeSb = buildHandshakeSb();
    tables->statusSb = buildStatusSb();
    tables->statusCb = buildStatusCb();
    tables->loginSb = buildLoginSb();
    tables->loginCb = buildLoginCb();
    tables->configurationSb = buildConfigurationSb();
    tables->configurationCb = buildConfigurationCb();
    tables->playSb = buildPlaySb();
    tables->playCb = buildPlayCb();
    // handshakeCb 在 Java 协议中不存在（握手只有 C→S），留空。
    return tables;
}

} // namespace mc::network::backend::java
