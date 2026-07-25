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

// 批 22：非 play 阶段 Java wire codec 往返（handshake 1 + status 4 + login 7 + configuration 10 = ~22 例）
// 表级往返（roundTripGeneric）同时验证 packetID 分发 + payload codec。各包 (id, flow, altIndex)
// 映射取自 JavaProtocolTables.cpp buildHandshakeSb/buildStatusSb/Cb/buildLoginSb/Cb/
// buildConfigurationSb/Cb。altIndex 取自 IrPacket.hpp variant 声明顺序。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::test;
using namespace mc;

// ============================================================================
// Handshake（Sb，1 例）
// ============================================================================

TEST_F(NetworkTestBase, HandshakeClientIntention)
{
    handshake::ClientIntention in{};
    in.protocolVersion = 774;
    in.hostName = "localhost";
    in.port = 25565;
    in.intendedState = 2; // Login

    auto out = roundTripGeneric(*tables()->handshakeSb, HandshakePacket{in});
    ASSERT_EQ(out.index(), 0u);
    EXPECT_EQ(std::get<handshake::ClientIntention>(out), in);
}

// ============================================================================
// Status（Sb 2 + Cb 2 = 4 例）
// ============================================================================

TEST_F(NetworkTestBase, StatusRequestEmpty)
{
    status::StatusRequest in{};
    auto out = roundTripGeneric(*tables()->statusSb, StatusPacket{in});
    ASSERT_EQ(out.index(), 0u); // altIndex 0
    EXPECT_EQ(std::get<status::StatusRequest>(out), in);
}

TEST_F(NetworkTestBase, StatusPingRequest)
{
    status::PingRequest in{};
    in.payload = 0x123456789ABCDEF0LL;
    auto out = roundTripGeneric(*tables()->statusSb, StatusPacket{in});
    ASSERT_EQ(out.index(), 2u); // altIndex 2
    EXPECT_EQ(std::get<status::PingRequest>(out), in);
}

TEST_F(NetworkTestBase, StatusResponseJson)
{
    status::StatusResponse in{};
    in.json = R"({"version":{"name":"1.21.11","protocol":774},"players":{"max":20,"online":0}})";
    auto out = roundTripGeneric(*tables()->statusCb, StatusPacket{in});
    ASSERT_EQ(out.index(), 1u); // altIndex 1
    EXPECT_EQ(std::get<status::StatusResponse>(out), in);
}

TEST_F(NetworkTestBase, StatusPongResponse)
{
    status::PingResponse in{};
    in.payload = -1;
    auto out = roundTripGeneric(*tables()->statusCb, StatusPacket{in});
    ASSERT_EQ(out.index(), 3u); // altIndex 3
    EXPECT_EQ(std::get<status::PingResponse>(out), in);
}

// ============================================================================
// Login（Sb 3: Hello/Key/LoginAcknowledged + Cb 4: Disconnect/HelloBound/
//       LoginFinished/LoginCompression = 7 例）
// ============================================================================

TEST_F(NetworkTestBase, LoginHelloOffline)
{
    login::Hello in{};
    in.username = "tester";
    in.publicKey = std::nullopt;
    in.keySignature = std::nullopt;
    auto out = roundTripGeneric(*tables()->loginSb, LoginPacket{in});
    ASSERT_EQ(out.index(), 0u);
    EXPECT_EQ(std::get<login::Hello>(out), in);
}

// helloCodec 当前是离线模式实现：只编 username + 零 UUID 占位，publicKey/keySignature
// 不上线（JavaCodecs.hpp:140-164 明记 TODO(Phase3/在线模式)）。故本例验证"离线模式 codec
// 丢弃 publicKey"这一已知行为——解码后 publicKey 必为 nullopt，username 保留。在线模式
// profile 公钥往返待 Phase3 codec 补全后改为完整断言。
TEST_F(NetworkTestBase, LoginHelloWithKey)
{
    login::Hello in{};
    in.username = "Alice";
    std::array<u8, 32> key{};
    for (usize i = 0; i < 32; ++i) {
        key[i] = static_cast<u8>(i);
    }
    in.publicKey = key;
    in.keySignature = std::nullopt;
    auto out = roundTripGeneric(*tables()->loginSb, LoginPacket{in});
    ASSERT_EQ(out.index(), 0u);
    const auto& decoded = std::get<login::Hello>(out);
    EXPECT_EQ(decoded.username, in.username);
    EXPECT_EQ(decoded.publicKey, std::nullopt); // 离线模式 codec 不传 publicKey
    EXPECT_EQ(decoded.keySignature, std::nullopt);
}

TEST_F(NetworkTestBase, LoginKey)
{
    login::Key in{};
    in.encryptedSharedSecret = std::vector<u8>(128, 0xAB);
    in.encryptedVerifyToken = std::vector<u8>{0x01, 0x02, 0x03, 0x04};
    auto out = roundTripGeneric(*tables()->loginSb, LoginPacket{in});
    ASSERT_EQ(out.index(), 2u);
    EXPECT_EQ(std::get<login::Key>(out), in);
}

TEST_F(NetworkTestBase, LoginAcknowledged)
{
    login::LoginAcknowledged in{};
    auto out = roundTripGeneric(*tables()->loginSb, LoginPacket{in});
    ASSERT_EQ(out.index(), 5u);
    EXPECT_EQ(std::get<login::LoginAcknowledged>(out), in);
}

TEST_F(NetworkTestBase, LoginDisconnect)
{
    login::Disconnect in{};
    in.reason = R"({"text":"Banned!"})";
    auto out = roundTripGeneric(*tables()->loginCb, LoginPacket{in});
    ASSERT_EQ(out.index(), 6u);
    EXPECT_EQ(std::get<login::Disconnect>(out), in);
}

TEST_F(NetworkTestBase, LoginHelloBound)
{
    login::HelloBound in{};
    in.serverId = "";
    in.publicKey = std::vector<u8>(162, 0x5A); // RSA-1024 DER
    in.verifyToken = std::vector<u8>{0xDE, 0xAD, 0xBE, 0xEF};
    in.shouldAuthenticate = true;
    auto out = roundTripGeneric(*tables()->loginCb, LoginPacket{in});
    ASSERT_EQ(out.index(), 1u);
    EXPECT_EQ(std::get<login::HelloBound>(out), in);
}

TEST_F(NetworkTestBase, LoginFinished)
{
    login::LoginFinished in{};
    in.uuid = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    in.username = "Bob";
    in.properties = {{"textures", "value123"}};
    auto out = roundTripGeneric(*tables()->loginCb, LoginPacket{in});
    ASSERT_EQ(out.index(), 3u);
    EXPECT_EQ(std::get<login::LoginFinished>(out), in);
}

TEST_F(NetworkTestBase, LoginCompressionThreshold)
{
    login::LoginCompression in{};
    in.threshold = 256;
    auto out = roundTripGeneric(*tables()->loginCb, LoginPacket{in});
    ASSERT_EQ(out.index(), 4u);
    EXPECT_EQ(std::get<login::LoginCompression>(out), in);
}

// ============================================================================
// Configuration（Sb 6 + Cb 9 = 10 例，去重后实测代表性子集）
// ============================================================================

TEST_F(NetworkTestBase, ConfigurationClientInformation)
{
    configuration::ClientInformation in{};
    in.language = "en_us";
    in.viewDistance = 12;
    in.chatVisibility = 0;
    in.chatColors = true;
    in.modelCustomisation = 0x7F;
    in.mainHand = 1;
    in.textFilteringEnabled = false;
    in.allowsListing = true;
    in.particleStatus = 0;
    auto out = roundTripGeneric(*tables()->configurationSb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 0u);
    EXPECT_EQ(std::get<configuration::ClientInformation>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationCustomPayloadSb)
{
    configuration::CustomPayload in{};
    in.identifier = "minecraft:brand";
    in.payload = std::vector<u8>{'v', 'a', 'n', 'i', 'l', 'l', 'a'};
    auto out = roundTripGeneric(*tables()->configurationSb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 1u);
    EXPECT_EQ(std::get<configuration::CustomPayload>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationFinishConfigurationSb)
{
    configuration::FinishConfiguration in{};
    auto out = roundTripGeneric(*tables()->configurationSb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 3u);
    EXPECT_EQ(std::get<configuration::FinishConfiguration>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationKeepAliveSb)
{
    configuration::KeepAlive in{};
    in.id = 99999;
    auto out = roundTripGeneric(*tables()->configurationSb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 4u);
    EXPECT_EQ(std::get<configuration::KeepAlive>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationPingSb)
{
    configuration::Ping in{};
    in.parameter = 42;
    auto out = roundTripGeneric(*tables()->configurationSb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 5u);
    EXPECT_EQ(std::get<configuration::Ping>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationSelectKnownPacksSb)
{
    configuration::SelectKnownPacks in{};
    in.knownPacks = {{"minecraft", "core", "1.21.11"}};
    auto out = roundTripGeneric(*tables()->configurationSb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 7u);
    EXPECT_EQ(std::get<configuration::SelectKnownPacks>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationDisconnectCb)
{
    configuration::Disconnect in{};
    in.reason = R"({"text":"Config error"})";
    auto out = roundTripGeneric(*tables()->configurationCb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 2u);
    EXPECT_EQ(std::get<configuration::Disconnect>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationRegistryDataCb)
{
    configuration::RegistryData in{};
    in.registryKey = "minecraft:dimension_type";
    in.entries = {{"minecraft:overworld", std::nullopt}, {"minecraft:the_nether", std::nullopt}};
    auto out = roundTripGeneric(*tables()->configurationCb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 6u);
    EXPECT_EQ(std::get<configuration::RegistryData>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationUpdateEnabledFeaturesCb)
{
    configuration::UpdateEnabledFeatures in{};
    in.features = {"minecraft:vanilla", "minecraft:bundle"};
    auto out = roundTripGeneric(*tables()->configurationCb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 8u);
    EXPECT_EQ(std::get<configuration::UpdateEnabledFeatures>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationUpdateTagsCb)
{
    configuration::UpdateTags in{};
    configuration::TagRegistry reg{};
    reg.registryKey = "minecraft:block";
    configuration::TagListEntry entry{};
    entry.tagName = "minecraft:logs";
    entry.elementIds = {1, 2, 3, 17};
    reg.tags = {entry};
    in.registries = {reg};
    auto out = roundTripGeneric(*tables()->configurationCb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 9u);
    EXPECT_EQ(std::get<configuration::UpdateTags>(out), in);
}

TEST_F(NetworkTestBase, ConfigurationSelectKnownPacksCb)
{
    configuration::SelectKnownPacks in{};
    in.knownPacks = {{"minecraft", "core", "1.21.11"}};
    auto out = roundTripGeneric(*tables()->configurationCb, ConfigurationPacket{in});
    ASSERT_EQ(out.index(), 7u);
    EXPECT_EQ(std::get<configuration::SelectKnownPacks>(out), in);
}
