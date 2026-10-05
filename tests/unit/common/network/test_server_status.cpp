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

// 服务器列表查询（Status）状态机单元测试：ClientIntention(STATUS) → StatusRequest → StatusResponse。
// 覆盖 ServerHandshakeStateMachine::_buildStatusJson 的字段组装与"省略而非 null"语义：
// description/players(max,online)/version 恒存在；sample/favicon/enforcesSecureChat 在空值/默认时省略。
// 另覆盖 StatusRequest 单次守卫与 PingRequest 回显。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/status/StatusPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/network/base/ServerClientConnection.hpp"
#include "server/network/handshake/ServerHandshake.hpp"

#include <gtest/gtest.h>

#include <string>
#include <variant>

#include <nlohmann/json.hpp>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::status;
using namespace mc::network::protocol;
using namespace mc::network::test;
using namespace mc::server::net;
using namespace mc;

namespace {
/// 构造 ClientIntention(intendedState=1 STATUS) IrPacket（驱动握手→Status）。
IrPacket makeIntentionStatus()
{
    handshake::ClientIntention ci{};
    ci.protocolVersion = 774;
    ci.hostName = "localhost";
    ci.port = 25565;
    ci.intendedState = 1; // STATUS
    return IrPacket{ConnectionProtocol::Handshaking, HandshakePacket{std::move(ci)}};
}

/// 构造 C→S StatusRequest IrPacket。
IrPacket makeStatusRequest()
{
    return IrPacket{ConnectionProtocol::Status, StatusPacket{StatusRequest{}}};
}

/// 构造 C→S PingRequest(payload) IrPacket。
IrPacket makePingRequest(i64 payload)
{
    PingRequest pr{};
    pr.payload = payload;
    return IrPacket{ConnectionProtocol::Status, StatusPacket{std::move(pr)}};
}

/// 从服务端发出的 S→C StatusResponse 中取出 JSON（非 StatusResponse 返回空 json）。
nlohmann::json parseStatusResponse(const std::optional<IrPacket>& packet)
{
    if (!packet.has_value() || packet->phase != ConnectionProtocol::Status) {
        return {};
    }
    const auto* st = std::get_if<StatusPacket>(&packet->packet);
    if (st == nullptr || !std::holds_alternative<StatusResponse>(*st)) {
        return {};
    }
    return nlohmann::json::parse(std::get<StatusResponse>(*st).json);
}

} // namespace

// ============================================================================
// 默认状态：sample/favicon/enforcesSecureChat 均省略（对齐 Java lenientOptionalFieldOf）
// ============================================================================

TEST_F(LocalServerFixture, StatusResponseOmitsEmptyOptionalFields)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    hs.onStatusRequest([]() -> StatusInfo {
        StatusInfo info;
        info.motd = "A Cubium Server";
        info.versionName = "1.21.11";
        info.protocolVersion = 774;
        info.maxPlayers = 20;
        info.onlinePlayers = 0;
        // sample/favicon 留空，enforcesSecureChat 默认 false。
        return info;
    });

    clientSend(makeIntentionStatus());
    pumpServer();
    clientSend(makeStatusRequest());
    pumpServer();

    const auto response = pumpClient();
    const nlohmann::json json = parseStatusResponse(response);
    ASSERT_FALSE(json.is_null()) << "未收到 StatusResponse";

    EXPECT_EQ(json["description"]["text"].get<std::string>(), "A Cubium Server");
    EXPECT_EQ(json["players"]["max"].get<i32>(), 20);
    EXPECT_EQ(json["players"]["online"].get<i32>(), 0);
    EXPECT_EQ(json["version"]["name"].get<std::string>(), "1.21.11");
    EXPECT_EQ(json["version"]["protocol"].get<i32>(), 774);

    // 空值/默认值字段必须省略（写 null 会让客户端解析失败）。
    EXPECT_FALSE(json.contains("favicon")) << "空 favicon 应省略";
    EXPECT_FALSE(json.contains("enforcesSecureChat")) << "false 时应省略";
    EXPECT_FALSE(json["players"].contains("sample")) << "空 sample 应省略";
}

// ============================================================================
// 样本/favicon/enforcesSecureChat 提供时写入 JSON
// ============================================================================

TEST_F(LocalServerFixture, StatusResponseIncludesSampleFaviconAndSecureChat)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    hs.onStatusRequest([]() -> StatusInfo {
        StatusInfo info;
        info.motd = "Test";
        info.versionName = "1.21.11";
        info.protocolVersion = 774;
        info.maxPlayers = 20;
        info.onlinePlayers = 2;
        info.sample = {
            PlayerSampleEntry{"Alice", "11111111-1111-1111-1111-111111111111"},
            PlayerSampleEntry{"Bob", "22222222-2222-2222-2222-222222222222"},
        };
        info.favicon = "data:image/png;base64,AAAA";
        info.enforcesSecureChat = true;
        return info;
    });

    clientSend(makeIntentionStatus());
    pumpServer();
    clientSend(makeStatusRequest());
    pumpServer();

    const nlohmann::json json = parseStatusResponse(pumpClient());
    ASSERT_FALSE(json.is_null());

    ASSERT_TRUE(json["players"].contains("sample"));
    const auto& sample = json["players"]["sample"];
    ASSERT_EQ(sample.size(), 2u);
    EXPECT_EQ(sample[0]["name"].get<std::string>(), "Alice");
    EXPECT_EQ(sample[0]["id"].get<std::string>(), "11111111-1111-1111-1111-111111111111");
    EXPECT_EQ(sample[1]["name"].get<std::string>(), "Bob");

    EXPECT_EQ(json["favicon"].get<std::string>(), "data:image/png;base64,AAAA");
    EXPECT_TRUE(json["enforcesSecureChat"].get<bool>());
}

// ============================================================================
// StatusRequest 单次守卫：二次请求断连（对齐 Java ServerStatusPacketListenerImpl）
// ============================================================================

TEST_F(LocalServerFixture, SecondStatusRequestDisconnects)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    hs.onStatusRequest([]() -> StatusInfo {
        StatusInfo info;
        info.motd = "x";
        info.versionName = "1.21.11";
        info.protocolVersion = 774;
        info.maxPlayers = 1;
        info.onlinePlayers = 0;
        return info;
    });

    clientSend(makeIntentionStatus());
    pumpServer();
    clientSend(makeStatusRequest());
    pumpServer();
    ASSERT_TRUE(serverConn()->isConnected());
    (void)pumpClient(); // 丢弃首个响应

    clientSend(makeStatusRequest());
    pumpServer();
    EXPECT_FALSE(serverConn()->isConnected()) << "二次 StatusRequest 应断连";
}

// ============================================================================
// PingRequest：原样回显 payload 后断连
// ============================================================================

TEST_F(LocalServerFixture, PingRequestEchoesPayloadAndDisconnects)
{
    auto& hs = installOfflineHandshake(/*compressionThreshold=*/-1);
    hs.onStatusRequest([]() -> StatusInfo {
        StatusInfo info;
        info.motd = "x";
        info.versionName = "1.21.11";
        info.protocolVersion = 774;
        info.maxPlayers = 1;
        info.onlinePlayers = 0;
        return info;
    });

    clientSend(makeIntentionStatus());
    pumpServer();
    clientSend(makePingRequest(0x1122334455667788LL));
    pumpServer();

    const auto response = pumpClient();
    ASSERT_TRUE(response.has_value());
    const auto* st = std::get_if<StatusPacket>(&response->packet);
    ASSERT_NE(st, nullptr);
    ASSERT_TRUE(std::holds_alternative<PingResponse>(*st));
    EXPECT_EQ(std::get<PingResponse>(*st).payload, 0x1122334455667788LL);
    EXPECT_FALSE(serverConn()->isConnected()) << "Pong 后应断连";
}
