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

// Phase3 阶段3 CAPSTONE：ClientNetwork Local 模式端到端握手单元测试。
// 验证门已过（plan-locked 决策#3）：ClientNetwork.cpp 不实例化 ClientApplication，
// 仅持 ClientPlayVisitor* 裸指针（setPlayVisitor 注入），故可传 nullptr 编入 mc_tests
// 单测——客户端握手状态机自驱动，play::Login 内部处理触发 onLoginReady，非 Login 的
// Play 包委托 visitor（nullptr 时静默丢弃，本测不发此类包）。
//
// 配对：ServerNetwork::createLocalClientSide 取服务端 ServerClientConnection + 客户端侧
// ILocalTransport；后者注入 ClientNetwork::connectLocal。服务端 onClientConnect 装
// ServerHandshakeStateMachine（离线、threshold=-1）+ onPacket 分流（handshake/Play）；
// onPlayerReady 回调发 play::Login（应用层职责，镜像 IntegratedServer）。
// 驱动：交替 serverNetwork.tick()/clientNetwork.tick() pump 双向 Local 队列至客户端 Playing。

#include "client/network/ClientNetwork.hpp"
#include "common/item/Items.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/network/ServerHandshake.hpp"
#include "server/network/ServerNetwork.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace mc::client::net;
using namespace mc::server::net;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::protocol;
using namespace mc;

namespace {

/// 构造最小 play::Login（服务端 onPlayerReady 后发，驱动客户端到 Playing + onLoginReady）。
IrPacket makePlayLogin(i32 playerId)
{
    Login login{};
    login.playerId = playerId;
    login.hardcore = false;
    login.levels = {"minecraft:overworld"};
    login.maxPlayers = 10;
    login.chunkRadius = 8;
    login.simulationDistance = 8;
    login.reducedDebugInfo = false;
    login.showDeathScreen = true;
    login.doLimitedCrafting = false;
    login.spawnInfo.dimensionType = 0;
    login.spawnInfo.dimension = "minecraft:overworld";
    login.spawnInfo.seed = 0;
    login.spawnInfo.gameType = GameMode::Survival;
    login.spawnInfo.previousGameType = -1;
    login.spawnInfo.isDebug = false;
    login.spawnInfo.isFlat = false;
    login.spawnInfo.lastDeathLocation = std::nullopt;
    login.spawnInfo.portalCooldown = 0;
    login.spawnInfo.seaLevel = 63;
    login.enforcesSecureChat = false;
    return IrPacket{ConnectionProtocol::Play, PlayPacket{std::move(login)}};
}

} // namespace

class ClientNetworkLocalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        m_serverNetwork = std::make_unique<ServerNetwork>();
        m_serverNetwork->onClientConnect([this](ServerClientConnection& conn) { _installServerSide(conn); });

        std::unique_ptr<transport::ILocalTransport> clientSideTransport;
        m_serverConn = m_serverNetwork->createLocalClientSide(&clientSideTransport);

        m_client = std::make_unique<ClientNetwork>();
        m_client->onLoginReady([this](i32 playerId, const std::string& dimension, const std::array<u8, 16>& uuid) {
            ++m_loginReadyCount;
            m_loginReadyPlayerId = playerId;
            m_loginReadyDimension = dimension;
            m_loginReadyUuid = uuid;
        });
        // visitor 传 nullptr：客户端不发非 Login 的 Play 包，无需 visitor。
        m_client->setPlayVisitor(nullptr);

        auto r = m_client->connectLocal(std::move(clientSideTransport), "capstone_tester");
        ASSERT_TRUE(r.success()) << r.error().toString();
    }

    void TearDown() override
    {
        m_client.reset();
        m_serverHandshake.reset();
        m_serverNetwork.reset();
    }

    /// 交替 pump 服务端与客户端若干轮，驱动双向 Local 队列推进握手。
    void pumpRounds(int rounds)
    {
        for (int i = 0; i < rounds; ++i) {
            m_serverNetwork->tick();
            m_client->tick();
        }
    }

    std::unique_ptr<ServerNetwork> m_serverNetwork;
    ServerClientConnection* m_serverConn = nullptr;
    std::unique_ptr<ServerHandshakeStateMachine> m_serverHandshake;
    std::unique_ptr<ClientNetwork> m_client;

    int m_loginReadyCount = 0;
    i32 m_loginReadyPlayerId = 0;
    std::string m_loginReadyDimension;
    std::array<u8, 16> m_loginReadyUuid{};

    static constexpr i32 kCapstonePlayerId = 42;

private:
    void _installServerSide(ServerClientConnection& conn)
    {
        m_serverHandshake = std::make_unique<ServerHandshakeStateMachine>(conn, /*isOfflineMode=*/true, -1);
        m_serverHandshake->onPlayerReady([this](const std::string& username, const std::array<u8, 16>& offlineUuid) {
            ++m_playerReadyCount;
            m_readyUsername = username;
            m_readyUuid = offlineUuid;
            // 应用层职责：发 play::Login 驱动客户端到 Playing（镜像 IntegratedServer::sendLoginResponse）。
            if (m_serverConn != nullptr) {
                m_serverConn->send(makePlayLogin(kCapstonePlayerId));
            }
        });
        conn.onPacket([this](const IrPacket& packet) {
            if (m_serverHandshake != nullptr) {
                auto r = m_serverHandshake->handleInbound(packet);
                if (!r.success()) {
                    ADD_FAILURE() << "server handshake failed: " << r.error().toString();
                    return;
                }
                if (r.value()) {
                    return; // 握手/Configuration 已消费
                }
            }
            // Play 包：本测客户端不发 Play 包，此处不期望到达。
        });
    }

    int m_playerReadyCount = 0;
    std::string m_readyUsername;
    std::array<u8, 16> m_readyUuid{};
};

// ============================================================================
// 端到端：客户端经 Local-pair 完成握手到 Playing + onLoginReady 触发
// ============================================================================

TEST_F(ClientNetworkLocalTest, ConnectLocalDrivesClientToPlaying)
{
    // connectLocal 已发 ClientIntention+Hello；pump 驱动双向握手至 Play。
    // 服务端 onPlayerReady 发 play::Login → 客户端 _handlePlayPacket 切 Playing + onLoginReady。
    pumpRounds(20);

    EXPECT_EQ(m_client->state(), ClientConnState::Playing);
    EXPECT_EQ(m_client->playerId(), kCapstonePlayerId);
    EXPECT_EQ(m_loginReadyCount, 1);
    EXPECT_EQ(m_loginReadyPlayerId, kCapstonePlayerId);
    EXPECT_EQ(m_loginReadyDimension, "minecraft:overworld");
}

TEST_F(ClientNetworkLocalTest, ClientPacketsSentReceivedNonZero)
{
    pumpRounds(20);
    // 客户端至少发了 ClientIntention + Hello + LoginAcknowledged + SelectKnownPacks + FinishConfiguration
    EXPECT_GT(m_client->packetsSent(), 0u);
    // 客户端至少收了 LoginCompression(跳过,threshold=-1 不发) + LoginFinished +
    // SelectKnownPacks + RegistryData×8 + UpdateTags + UpdateEnabledFeatures +
    // FinishConfiguration + play::Login
    EXPECT_GT(m_client->packetsReceived(), 0u);
}

TEST_F(ClientNetworkLocalTest, ClientSendDeliversToServerInbound)
{
    pumpRounds(20);
    ASSERT_EQ(m_client->state(), ClientConnState::Playing);

    // 客户端 send 一个 Play 包（KeepAlive）→ 经 Local-pair 投到服务端入站队列。
    // 服务端 onPacket 监听器收到后 handleInbound 返回 false（Play 包）。
    // 本测不装 Play 路由器，仅验证 send 不错且 pump 不崩。
    KeepAlive ka{};
    ka.id = 7;
    auto r = m_client->send(IrPacket{ConnectionProtocol::Play, PlayPacket{std::move(ka)}});
    EXPECT_TRUE(r.success());
    pumpRounds(2);
    EXPECT_GT(m_client->packetsSent(), 0u);
}

TEST_F(ClientNetworkLocalTest, DisconnectTransitionsToDisconnected)
{
    pumpRounds(20);
    ASSERT_EQ(m_client->state(), ClientConnState::Playing);

    m_client->disconnect("test disconnect");
    EXPECT_EQ(m_client->state(), ClientConnState::Disconnected);
    EXPECT_FALSE(m_client->isConnected());
}
