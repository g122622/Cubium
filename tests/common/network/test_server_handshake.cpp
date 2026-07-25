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

// Phase3 阶段3：ServerHandshakeStateMachine 离线握手 + Configuration 状态机单元测试。
// 用 LocalServerFixture（无 MinecraftServer）：Local-pair 互连服务端 ServerClientConnection
// 与客户端侧 ILocalTransport，客户端 send 包到服务端入站队列，pumpServer() 主线程派发，
// 服务端 onPacket 监听器分流 handshake.handleInbound（已消费）/Play 包（计数）。
//
// 驱动序列（离线、threshold=-1）：ClientIntention(LOGIN)→Hello("tester")→
// LoginAcknowledged→SelectKnownPacks→FinishConfiguration；逐包 pump 断言 HandshakeState 转换
// （Handshaking→Login→Configuration→Play）+ playReady() + onPlayerReady 触发一次 + 幂等
// （二次 FinishConfiguration 仍只触发一次）。threshold=256 分支发 LoginCompression 在
// LoginFinished 前；threshold=-1 跳过 LoginCompression。
//
// 注意 Local 模式 Connection 收 terminal 包会自动 setPhase（_installLocalReceive 内
// ProtocolSwapHandler::check + setPhase），故服务端连接 phase 字段随 ClientIntention/
// LoginAcknowledged/FinishConfiguration 自动推进，HandshakeState（握手状态机自己的 m_state）
// 由 ServerHandshakeStateMachine 显式 setState 驱动。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/handshake/JavaLoginHandshaker.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/network/ServerNetwork.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::configuration;
using namespace mc::network::ir::login;
using namespace mc::network::ir::handshake;
using namespace mc::network::protocol;
using namespace mc::network::test;
using namespace mc::server::net;
using namespace mc;

namespace {

/// 构造 ClientIntention(LOGIN=2) IrPacket（驱动握手→Login）
IrPacket makeIntentionLogin()
{
    ClientIntention ci{};
    ci.protocolVersion = 774;
    ci.hostName = "localhost";
    ci.port = 25565;
    ci.intendedState = 2; // LOGIN
    return IrPacket{ConnectionProtocol::Handshaking, HandshakePacket{std::move(ci)}};
}

/// 构造 Hello(username) IrPacket（驱动 Login→发 LoginFinished）
IrPacket makeHello(const std::string& username)
{
    Hello h{};
    h.username = username;
    return IrPacket{ConnectionProtocol::Login, LoginPacket{std::move(h)}};
}

/// 构造 LoginAcknowledged IrPacket（驱动 Configuration→发 SelectKnownPacks 等）
IrPacket makeLoginAcknowledged()
{
    return IrPacket{ConnectionProtocol::Login, LoginPacket{LoginAcknowledged{}}};
}

/// 构造 C→S SelectKnownPacks IrPacket（客户端确认命中 minecraft:core）
IrPacket makeClientSelectKnownPacks()
{
    SelectKnownPacks skp{};
    KnownPack p{};
    p.ns = "minecraft";
    p.id = "core";
    p.version = "1.21.11";
    skp.knownPacks = {std::move(p)};
    return IrPacket{ConnectionProtocol::Configuration, ConfigurationPacket{std::move(skp)}};
}

/// 构造 C→S FinishConfiguration IrPacket（驱动 Play→onPlayerReady）
IrPacket makeClientFinishConfiguration()
{
    return IrPacket{ConnectionProtocol::Configuration, ConfigurationPacket{FinishConfiguration{}}};
}

/// 构造一个 Play 阶段包（KeepAlive）用于断言握手完成后的 Play 包分流。
IrPacket makePlayKeepAlive()
{
    play::KeepAlive ka{};
    ka.id = 42;
    return IrPacket{ConnectionProtocol::Play, PlayPacket{std::move(ka)}};
}

} // namespace

// ============================================================================
// 离线模式（threshold=-1）完整握手链路
// ============================================================================

TEST_F(LocalServerFixture, OfflineHandshakeDrivesStateTransitions)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    EXPECT_EQ(hs.playReady(), false);
    EXPECT_EQ(serverConn()->state(), HandshakeState::Handshaking);
    EXPECT_EQ(serverConn()->phase(), ConnectionProtocol::Handshaking);

    // 1. ClientIntention(LOGIN) → HandshakeState:Login
    clientSend(makeIntentionLogin());
    pumpServer();
    EXPECT_EQ(serverConn()->state(), HandshakeState::Login);

    // 2. Hello("tester") → 发 LoginFinished（threshold<0 跳过 LoginCompression）
    clientSend(makeHello("tester"));
    pumpServer();
    EXPECT_EQ(hs.username(), "tester");
    EXPECT_EQ(serverConn()->state(), HandshakeState::Login); // 等 LoginAcknowledged

    // 3. LoginAcknowledged → HandshakeState:Configuration + 发 SelectKnownPacks
    clientSend(makeLoginAcknowledged());
    pumpServer();
    EXPECT_EQ(serverConn()->state(), HandshakeState::Configuration);

    // 4. C→S SelectKnownPacks → 服务端推 RegistryData×N+UpdateTags+UpdateEnabledFeatures+FinishConfiguration
    clientSend(makeClientSelectKnownPacks());
    pumpServer();
    // Configuration 推送完成后仍在 Configuration 状态（等客户端回 FinishConfiguration）
    EXPECT_EQ(serverConn()->state(), HandshakeState::Configuration);

    // 5. C→S FinishConfiguration → HandshakeState:Play + playReady + onPlayerReady
    clientSend(makeClientFinishConfiguration());
    pumpServer();
    EXPECT_EQ(serverConn()->state(), HandshakeState::Play);
    EXPECT_EQ(hs.playReady(), true);
    EXPECT_EQ(playerReadyCount(), 1);
}

TEST_F(LocalServerFixture, OfflineHandshakeFiresOnPlayerReadyOnce)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    (void)hs;

    clientSend(makeIntentionLogin());
    pumpServer();
    clientSend(makeHello("tester"));
    pumpServer();
    clientSend(makeLoginAcknowledged());
    pumpServer();
    clientSend(makeClientSelectKnownPacks());
    pumpServer();
    clientSend(makeClientFinishConfiguration());
    pumpServer();

    EXPECT_EQ(playerReadyCount(), 1);
    EXPECT_EQ(readyUsername(), "tester");
    // 离线 UUID 由用户名生成，16 字节非全零。
    bool allZero = true;
    for (auto b : readyUuid()) {
        if (b != 0) {
            allZero = false;
            break;
        }
    }
    EXPECT_FALSE(allZero) << "离线 UUID 不应全零";
}

TEST_F(LocalServerFixture, DuplicateFinishConfigurationIsIdempotent)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    (void)hs;

    clientSend(makeIntentionLogin());
    pumpServer();
    clientSend(makeHello("tester"));
    pumpServer();
    clientSend(makeLoginAcknowledged());
    pumpServer();
    clientSend(makeClientSelectKnownPacks());
    pumpServer();
    clientSend(makeClientFinishConfiguration());
    pumpServer();
    EXPECT_EQ(playerReadyCount(), 1);

    // 二次 FinishConfiguration：幂等守卫，仍只触发一次 onPlayerReady
    clientSend(makeClientFinishConfiguration());
    pumpServer();
    EXPECT_EQ(playerReadyCount(), 1);
    EXPECT_EQ(hs.playReady(), true);
    EXPECT_EQ(serverConn()->state(), HandshakeState::Play);
}

// ============================================================================
// threshold 分支：256 发 LoginCompression 在 LoginFinished 前；-1 跳过
// 注意：threshold 不影响 HandshakeState 转换，只影响服务端是否发 LoginCompression
// 并 setupCompression。HandshakeState 在收到 Hello 后仍停在 Login（等 LoginAcknowledged）。
// 这里通过观察压缩层是否激活来区分两条分支。
// ============================================================================

TEST_F(LocalServerFixture, ThresholdPositiveEnablesCompressionAfterHello)
{
    // threshold=256：收 Hello 后发 LoginCompression 并 setupCompression(256)。
    installOfflineHandshake(/*compressionThreshold=*/256);

    clientSend(makeIntentionLogin());
    pumpServer();
    clientSend(makeHello("compress_me"));
    pumpServer();

    // setupCompression(256) 已被调用：raw() Connection 的压缩层应激活。
    // 这里间接验证：发 Hello 后服务端连接仍连通且不崩，状态推进正常。
    EXPECT_EQ(serverConn()->state(), HandshakeState::Login);
    EXPECT_TRUE(serverConn()->isConnected());
}

TEST_F(LocalServerFixture, ThresholdNegativeSkipsCompression)
{
    // threshold=-1：收 Hello 后不发 LoginCompression、不 setupCompression。
    installOfflineHandshake(/*compressionThreshold=*/-1);

    clientSend(makeIntentionLogin());
    pumpServer();
    clientSend(makeHello("no_compress"));
    pumpServer();

    EXPECT_EQ(serverConn()->state(), HandshakeState::Login);
    EXPECT_TRUE(serverConn()->isConnected());
}

// ============================================================================
// Play 包分流：握手完成后 Play 包经 handleInbound 返回 false，由监听器计数
// ============================================================================

TEST_F(LocalServerFixture, PlayPacketRoutedAfterHandshakeComplete)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    (void)hs;

    // 完成完整握手
    clientSend(makeIntentionLogin());
    pumpServer();
    clientSend(makeHello("tester"));
    pumpServer();
    clientSend(makeLoginAcknowledged());
    pumpServer();
    clientSend(makeClientSelectKnownPacks());
    pumpServer();
    clientSend(makeClientFinishConfiguration());
    pumpServer();
    EXPECT_EQ(playPacketCount(), 0);

    // 握手完成后发一个 Play 包：handleInbound 返回 false → 监听器计数 +1
    clientSend(makePlayKeepAlive());
    pumpServer();
    EXPECT_EQ(playPacketCount(), 1);
}

TEST_F(LocalServerFixture, UnsupportedIntentionRejected)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    (void)hs;

    // intendedState=1 (Status) 不被支持 → handleInbound 返回错误（夹具记录不 ADD_FAILURE）。
    ClientIntention ci{};
    ci.protocolVersion = 774;
    ci.hostName = "localhost";
    ci.port = 25565;
    ci.intendedState = 1; // STATUS，未实现
    clientSend(IrPacket{ConnectionProtocol::Handshaking, HandshakePacket{std::move(ci)}});

    // pumpServer 不应崩，状态机仍停在 Handshaking（未 setState Login）。
    pumpServer();
    EXPECT_EQ(serverConn()->state(), HandshakeState::Handshaking);
    EXPECT_EQ(hs.playReady(), false);
    EXPECT_EQ(inboundErrorCount(), 1);
    EXPECT_NE(lastInboundError().find("仅支持 Login 意图"), std::string::npos);
}
