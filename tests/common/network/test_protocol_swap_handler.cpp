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

#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/ProtocolSwapHandler.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"

#include <gtest/gtest.h>

using namespace mc::network::pipeline;
using namespace mc::network;
using namespace mc;

namespace {

ir::IrPacket wrapHandshake(ir::HandshakePacket p)
{
    return ir::IrPacket{protocol::ConnectionProtocol::Handshaking, std::move(p)};
}
ir::IrPacket wrapLogin(protocol::ConnectionProtocol phase, ir::LoginPacket p)
{
    return ir::IrPacket{phase, std::move(p)};
}
ir::IrPacket wrapConfig(ir::ConfigurationPacket p)
{
    return ir::IrPacket{protocol::ConnectionProtocol::Configuration, std::move(p)};
}
ir::IrPacket wrapPlay(ir::PlayPacket p)
{
    return ir::IrPacket{protocol::ConnectionProtocol::Play, std::move(p)};
}

} // namespace

// ============================================================================
// 握手阶段 ClientIntention → Status/Login
// ============================================================================

TEST(ProtocolSwapHandler, ClientIntentionStatusTerminal)
{
    ir::handshake::ClientIntention ci;
    ci.protocolVersion = 774;
    ci.hostName = "localhost";
    ci.port = 25565;
    ci.intendedState = 1; // Status
    auto r = ProtocolSwapHandler::check(wrapHandshake(ir::HandshakePacket{ci}), protocol::PacketFlow::Serverbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Status);
}

TEST(ProtocolSwapHandler, ClientIntentionLoginTerminal)
{
    ir::handshake::ClientIntention ci;
    ci.intendedState = 2; // Login
    auto r = ProtocolSwapHandler::check(wrapHandshake(ir::HandshakePacket{ci}), protocol::PacketFlow::Serverbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Login);
}

TEST(ProtocolSwapHandler, ClientIntentionTransferAlsoLogin)
{
    // intendedState=3（Transfer）走 Login
    ir::handshake::ClientIntention ci;
    ci.intendedState = 3;
    auto r = ProtocolSwapHandler::check(wrapHandshake(ir::HandshakePacket{ci}), protocol::PacketFlow::Serverbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Login);
}

// ============================================================================
// 登录阶段 Key/LoginFinished/LoginAcknowledged → Configuration
// ============================================================================

TEST(ProtocolSwapHandler, KeyTerminalToConfiguration)
{
    ir::login::Key k;
    auto r = ProtocolSwapHandler::check(
        wrapLogin(protocol::ConnectionProtocol::Login, ir::LoginPacket{k}), protocol::PacketFlow::Serverbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Configuration);
}

TEST(ProtocolSwapHandler, LoginFinishedTerminalToConfiguration)
{
    ir::login::LoginFinished lf;
    auto r = ProtocolSwapHandler::check(
        wrapLogin(protocol::ConnectionProtocol::Login, ir::LoginPacket{lf}), protocol::PacketFlow::Clientbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Configuration);
}

TEST(ProtocolSwapHandler, LoginAcknowledgedTerminalToConfiguration)
{
    ir::login::LoginAcknowledged la;
    auto r = ProtocolSwapHandler::check(
        wrapLogin(protocol::ConnectionProtocol::Login, ir::LoginPacket{la}), protocol::PacketFlow::Serverbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Configuration);
}

TEST(ProtocolSwapHandler, HelloNotTerminal)
{
    ir::login::Hello h;
    auto r = ProtocolSwapHandler::check(
        wrapLogin(protocol::ConnectionProtocol::Login, ir::LoginPacket{h}), protocol::PacketFlow::Serverbound);
    EXPECT_FALSE(r.isTerminal);
}

// ============================================================================
// 配置阶段 FinishConfiguration → Play
// ============================================================================

TEST(ProtocolSwapHandler, FinishConfigurationTerminalToPlay)
{
    ir::configuration::FinishConfiguration fc;
    auto r = ProtocolSwapHandler::check(wrapConfig(ir::ConfigurationPacket{fc}), protocol::PacketFlow::Serverbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Play);
}

TEST(ProtocolSwapHandler, ConfigurationRegistryDataNotTerminal)
{
    ir::configuration::RegistryData rd;
    auto r = ProtocolSwapHandler::check(wrapConfig(ir::ConfigurationPacket{rd}), protocol::PacketFlow::Clientbound);
    EXPECT_FALSE(r.isTerminal);
}

// ============================================================================
// Play 阶段 ConfigurationAcknowledged → 回 Configuration
// ============================================================================

TEST(ProtocolSwapHandler, ConfigurationAcknowledgedGoesBackToConfiguration)
{
    ir::play::ConfigurationAcknowledged ca;
    auto r = ProtocolSwapHandler::check(wrapPlay(ir::PlayPacket{ca}), protocol::PacketFlow::Serverbound);
    EXPECT_TRUE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Configuration);
}

TEST(ProtocolSwapHandler, PlayKeepAliveNotTerminal)
{
    ir::play::KeepAlive ka;
    ka.id = 1;
    auto r = ProtocolSwapHandler::check(wrapPlay(ir::PlayPacket{ka}), protocol::PacketFlow::Serverbound);
    EXPECT_FALSE(r.isTerminal);
}

TEST(ProtocolSwapHandler, PlayAcceptTeleportationNotTerminal)
{
    ir::play::AcceptTeleportation at;
    auto r = ProtocolSwapHandler::check(wrapPlay(ir::PlayPacket{at}), protocol::PacketFlow::Serverbound);
    EXPECT_FALSE(r.isTerminal);
}

TEST(ProtocolSwapHandler, NonTerminalReturnsCurrentPhase)
{
    // 非 terminal 包的 nextPhase 应等于当前阶段（不变）
    ir::play::KeepAlive ka;
    auto r = ProtocolSwapHandler::check(wrapPlay(ir::PlayPacket{ka}), protocol::PacketFlow::Serverbound);
    EXPECT_FALSE(r.isTerminal);
    EXPECT_EQ(r.nextPhase, protocol::ConnectionProtocol::Play);
}
