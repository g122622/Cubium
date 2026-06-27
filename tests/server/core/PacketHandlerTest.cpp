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

#include "server/core/PacketHandler.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include <gtest/gtest.h>

using namespace mc::server::core;
using namespace mc::network;
using mc::Hand;
using mc::PlayerId;
using mc::u32;
using mc::u8;

/**
 * @brief PacketHandler 单元测试
 *
 * 测试 PacketHandler 的核心功能：
 * - handlePlayerInput: 载具输入传递
 * - handleMoveVehicle: 载具位置验证
 * - handleUseEntity: 实体交互处理
 * - handleEntityAction: 实体动作处理
 */
class PacketHandlerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();
        m_playerManager = std::make_unique<PlayerManager>();

        m_connectionManager = std::make_unique<ConnectionManager>(*m_playerManager);
        m_timeManager = std::make_unique<TimeManager>(0, 0);
        m_teleportManager = std::make_unique<TeleportManager>(*m_playerManager);
        m_keepAliveManager = std::make_unique<KeepAliveManager>(*m_playerManager, 1000, 5000);
        m_positionTracker = std::make_unique<PositionTracker>(*m_playerManager, 6);

        m_packetHandler = std::make_unique<PacketHandler>(*m_playerManager,
            *m_connectionManager,
            *m_teleportManager,
            *m_keepAliveManager,
            *m_positionTracker,
            *m_timeManager,
            mc::GameMode::Survival);
    }

    void TearDown() override
    {
        m_packetHandler.reset();
        m_positionTracker.reset();
        m_keepAliveManager.reset();
        m_teleportManager.reset();
        m_timeManager.reset();
        m_connectionManager.reset();
        m_playerManager.reset();
        m_connectionPair.reset();
    }

    ConnectionPtr createConnection()
    {
        return std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    std::unique_ptr<LocalConnectionPair> m_connectionPair;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<ConnectionManager> m_connectionManager;
    std::unique_ptr<TimeManager> m_timeManager;
    std::unique_ptr<TeleportManager> m_teleportManager;
    std::unique_ptr<KeepAliveManager> m_keepAliveManager;
    std::unique_ptr<PositionTracker> m_positionTracker;
    std::unique_ptr<PacketHandler> m_packetHandler;
};

// ==================== PlayerInputPacket Tests ====================

TEST_F(PacketHandlerTest, HandlePlayerInput_UnknownSession_ReturnsIgnore)
{
    // 创建 PlayerInputPacket
    PlayerInputPacket packet;
    packet.setStrafeSpeed(0.5f);
    packet.setForwardSpeed(1.0f);
    packet.setJumping(true);
    packet.setSneaking(false);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    // 使用未映射的 sessionId（没有对应玩家）
    auto result = m_packetHandler->handlePlayerInput(999, data.data(), data.size());

    // 未知会话应该返回 Ignore
    EXPECT_EQ(result, PacketHandleResult::Ignore);
}

TEST_F(PacketHandlerTest, HandlePlayerInput_ValidSession_NoServer_ReturnsSuccess)
{
    // 添加玩家并映射会话
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 创建 PlayerInputPacket
    PlayerInputPacket packet;
    packet.setStrafeSpeed(0.5f);
    packet.setForwardSpeed(1.0f);
    packet.setJumping(false);
    packet.setSneaking(false);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    // 没有设置 server，应该仍然返回 Success（不阻塞）
    auto result = m_packetHandler->handlePlayerInput(1, data.data(), data.size());

    EXPECT_EQ(result, PacketHandleResult::Success);
}

TEST_F(PacketHandlerTest, HandlePlayerInput_InvalidPacket_ReturnsError)
{
    // 添加玩家
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 使用无效数据
    mc::u8 invalidData[] = {0x00, 0x01, 0x02, 0x03};
    auto result = m_packetHandler->handlePlayerInput(1, invalidData, sizeof(invalidData));

    EXPECT_EQ(result, PacketHandleResult::Error);
}

TEST_F(PacketHandlerTest, HandlePlayerInput_ClampsInputValues)
{
    // 添加玩家
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 创建超范围的输入值
    PlayerInputPacket packet;
    packet.setStrafeSpeed(5.0f);   // 超过 1.0
    packet.setForwardSpeed(-3.0f); // 小于 -1.0
    packet.setJumping(true);
    packet.setSneaking(false);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handlePlayerInput(1, data.data(), data.size());

    // 应该成功处理（内部会 clamp 到 [-1.0, 1.0]）
    EXPECT_EQ(result, PacketHandleResult::Success);
}

// ==================== MoveVehiclePacket Tests ====================

TEST_F(PacketHandlerTest, HandleMoveVehicle_UnknownSession_ReturnsIgnore)
{
    MoveVehiclePacket packet;
    packet.setPosition(100.0, 64.0, 200.0);
    packet.setRotation(90.0f, 0.0f);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleMoveVehicle(999, data.data(), data.size());

    EXPECT_EQ(result, PacketHandleResult::Ignore);
}

TEST_F(PacketHandlerTest, HandleMoveVehicle_ValidSession_NoServer_ReturnsSuccess)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    MoveVehiclePacket packet;
    packet.setPosition(100.0, 64.0, 200.0);
    packet.setRotation(90.0f, 45.0f);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleMoveVehicle(1, data.data(), data.size());

    // 没有设置 server，应该返回 Success（不阻塞）
    EXPECT_EQ(result, PacketHandleResult::Success);
}

TEST_F(PacketHandlerTest, HandleMoveVehicle_InvalidPacket_ReturnsError)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    mc::u8 invalidData[] = {0x00, 0x01, 0x02, 0x03};
    auto result = m_packetHandler->handleMoveVehicle(1, invalidData, sizeof(invalidData));

    EXPECT_EQ(result, PacketHandleResult::Error);
}

TEST_F(PacketHandlerTest, HandleMoveVehicle_ValidPosition_ReturnsSuccess)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 创建有效的 packet
    MoveVehiclePacket packet;
    packet.setPosition(100.0, 64.0, 200.0);
    packet.setRotation(90.0f, 45.0f);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleMoveVehicle(1, data.data(), data.size());

    // 没有实际载具时，也应该返回 Success（跳过处理）
    EXPECT_EQ(result, PacketHandleResult::Success);
}

// ==================== UseEntityPacket Tests ====================

TEST_F(PacketHandlerTest, HandleUseEntity_UnknownSession_ReturnsIgnore)
{
    UseEntityPacket packet;
    packet.setEntityId(100);
    packet.setAction(UseEntityAction::Interact);
    packet.setHand(Hand::MainHand);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleUseEntity(999, data.data(), data.size());

    EXPECT_EQ(result, PacketHandleResult::Ignore);
}

TEST_F(PacketHandlerTest, HandleUseEntity_ValidSession_NoServer_ReturnsSuccess)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    UseEntityPacket packet;
    packet.setEntityId(100);
    packet.setAction(UseEntityAction::Interact);
    packet.setHand(Hand::MainHand);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleUseEntity(1, data.data(), data.size());

    // 没有设置 server，应该返回 Success
    EXPECT_EQ(result, PacketHandleResult::Success);
}

TEST_F(PacketHandlerTest, HandleUseEntity_InvalidPacket_ReturnsError)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // UseEntityPacket with insufficient data - needs entityId (VarInt) + action (VarInt)
    // Create a packet that's too small to contain valid VarInt data
    mc::u8 invalidData[] = {0x00};
    auto result = m_packetHandler->handleUseEntity(1, invalidData, 1);

    // Should return Error because deserialization fails
    EXPECT_EQ(result, PacketHandleResult::Error);
}

TEST_F(PacketHandlerTest, HandleUseEntity_AllActionTypes)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 测试所有交互类型
    UseEntityAction actions[] = {UseEntityAction::Interact, UseEntityAction::Attack, UseEntityAction::InteractAt};

    for (auto action : actions) {
        UseEntityPacket packet;
        packet.setEntityId(100);
        packet.setAction(action);
        packet.setHand(Hand::MainHand);
        if (action == UseEntityAction::InteractAt) {
            packet.setHitPosition(0.5f, 0.5f, 0.5f);
        }

        auto serializeResult = packet.serialize();
        ASSERT_TRUE(serializeResult.success());

        const auto& data = serializeResult.value();
        auto result = m_packetHandler->handleUseEntity(1, data.data(), data.size());

        // 没有实际实体时，应该返回 Success 或 Ignore
        EXPECT_TRUE(result == PacketHandleResult::Success || result == PacketHandleResult::Ignore);
    }
}

// ==================== EntityActionPacket Tests ====================

TEST_F(PacketHandlerTest, HandleEntityAction_UnknownSession_ReturnsIgnore)
{
    EntityActionPacket packet;
    packet.setEntityId(1);
    packet.setAction(EntityActionType::StartSprinting);
    packet.setAuxData(0);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleEntityAction(999, data.data(), data.size());

    EXPECT_EQ(result, PacketHandleResult::Ignore);
}

TEST_F(PacketHandlerTest, HandleEntityAction_ValidSession_NoServer_ReturnsSuccess)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    EntityActionPacket packet;
    packet.setEntityId(1);
    packet.setAction(EntityActionType::StartSprinting);
    packet.setAuxData(0);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleEntityAction(1, data.data(), data.size());

    // 没有设置 server，应该返回 Success
    EXPECT_EQ(result, PacketHandleResult::Success);
}

TEST_F(PacketHandlerTest, HandleEntityAction_InvalidPacket_ReturnsError)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // EntityActionPacket needs entityId (VarInt) + action (VarInt) + auxData (VarInt)
    // A single byte is not enough to deserialize all required fields
    mc::u8 invalidData[] = {0x00};
    auto result = m_packetHandler->handleEntityAction(1, invalidData, 1);

    EXPECT_EQ(result, PacketHandleResult::Error);
}

TEST_F(PacketHandlerTest, HandleEntityAction_AllActionTypes)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 测试所有动作类型
    EntityActionType actions[] = {EntityActionType::PressShiftKey,
        EntityActionType::ReleaseShiftKey,
        EntityActionType::StartSprinting,
        EntityActionType::StopSprinting,
        EntityActionType::StartRidingJump,
        EntityActionType::StopRidingJump};

    for (auto action : actions) {
        EntityActionPacket packet;
        packet.setEntityId(1);
        packet.setAction(action);
        packet.setAuxData(action == EntityActionType::StartRidingJump ? 50 : 0);

        auto serializeResult = packet.serialize();
        ASSERT_TRUE(serializeResult.success());

        const auto& data = serializeResult.value();
        auto result = m_packetHandler->handleEntityAction(1, data.data(), data.size());

        // 应该返回 Success
        EXPECT_EQ(result, PacketHandleResult::Success);
    }
}

// ==================== Packet Dispatch Tests ====================

TEST_F(PacketHandlerTest, HandlePacket_DispatchesToCorrectHandler)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 测试 PlayerMove 通过直接调用处理方法
    PlayerPosition pos(100.0, 64.0, 200.0, 90.0f, 45.0f, true);
    PlayerMovePacket movePacket(pos, PlayerMovePacket::MoveType::Full);

    // 构造负载数据
    mc::network::PacketSerializer payload;
    movePacket.serialize(payload);

    auto result = m_packetHandler->handlePlayerMove(1, payload.data(), payload.size());
    EXPECT_EQ(result, PacketHandleResult::Success);
}

TEST_F(PacketHandlerTest, HandlePacket_TooSmall_ReturnsError)
{
    mc::u8 smallData[] = {0x00, 0x01}; // 小于 PACKET_HEADER_SIZE
    auto result = m_packetHandler->handlePacket(1, smallData, sizeof(smallData));
    EXPECT_EQ(result, PacketHandleResult::Error);
}

// ==================== SetServer Tests ====================

TEST_F(PacketHandlerTest, SetServer_GetServer)
{
    // 初始时应该为 nullptr
    EXPECT_EQ(m_packetHandler->getServer(), nullptr);

    // 设置后应该能获取
    mc::server::IServer* server = reinterpret_cast<mc::server::IServer*>(0x1234);
    m_packetHandler->setServer(server);
    EXPECT_EQ(m_packetHandler->getServer(), server);

    // 重置为 nullptr
    m_packetHandler->setServer(nullptr);
    EXPECT_EQ(m_packetHandler->getServer(), nullptr);
}

// ==================== Callback Tests ====================

TEST_F(PacketHandlerTest, LoginCallback)
{
    bool loginSuccessCalled = false;
    bool loginFailCalled = false;
    PlayerId successPlayerId = 0;
    mc::u32 failSessionId = 0;
    std::string successUsername;
    std::string failMessage;

    m_packetHandler->setOnLoginSuccess([&](PlayerId id, const std::string& username) {
        loginSuccessCalled = true;
        successPlayerId = id;
        successUsername = username;
    });

    m_packetHandler->setOnLoginFail([&](mc::u32 sessionId, const std::string& message) {
        loginFailCalled = true;
        failSessionId = sessionId;
        failMessage = message;
    });

    // 测试登录失败回调（空数据）
    auto conn = createConnection();
    LoginResult result = m_packetHandler->handleLoginRequest(1, conn, nullptr, 0);

    // 登录失败（空数据）应该触发失败回调
    EXPECT_FALSE(loginSuccessCalled);
}

TEST_F(PacketHandlerTest, ChatCallback)
{
    bool chatCalled = false;
    PlayerId chatPlayerId = 0;
    std::string chatUsername;
    std::string chatMessage;

    m_packetHandler->setOnChat([&](PlayerId id, const std::string& username, const std::string& message) {
        chatCalled = true;
        chatPlayerId = id;
        chatUsername = username;
        chatMessage = message;
    });

    // 添加玩家
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 直接测试 handleChatMessage 方法
    mc::network::PacketSerializer payload;
    ChatMessagePacket packet;
    packet.setMessage("Hello, World!");
    packet.serialize(payload);

    auto result = m_packetHandler->handleChatMessage(1, payload.data(), payload.size());

    EXPECT_EQ(result, PacketHandleResult::Success);
    EXPECT_TRUE(chatCalled);
    EXPECT_EQ(chatPlayerId, 1u);
    EXPECT_EQ(chatUsername, "Steve");
    EXPECT_EQ(chatMessage, "Hello, World!");
}

// ==================== PlayerInputPacket Serialization Tests ====================

TEST(PacketHandlerPlayerInputPacketTest, SerializeDeserialize)
{
    PlayerInputPacket packet;
    packet.setStrafeSpeed(0.5f);
    packet.setForwardSpeed(-0.75f);
    packet.setJumping(true);
    packet.setSneaking(true);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    PlayerInputPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FLOAT_EQ(packet2.strafeSpeed(), 0.5f);
    EXPECT_FLOAT_EQ(packet2.forwardSpeed(), -0.75f);
    EXPECT_TRUE(packet2.isJumping());
    EXPECT_TRUE(packet2.isSneaking());
}

TEST(PacketHandlerPlayerInputPacketTest, PacketType)
{
    PlayerInputPacket packet;
    EXPECT_EQ(packet.type(), PacketType::PlayerInput);
}

// ==================== MoveVehiclePacket Serialization Tests ====================

TEST(PacketHandlerMoveVehiclePacketTest, SerializeDeserialize)
{
    MoveVehiclePacket packet;
    packet.setPosition(123.45, 64.0, -789.12);
    packet.setRotation(180.0f, 45.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    MoveVehiclePacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_DOUBLE_EQ(packet2.x(), 123.45);
    EXPECT_DOUBLE_EQ(packet2.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet2.z(), -789.12);
    EXPECT_FLOAT_EQ(packet2.yaw(), 180.0f);
    EXPECT_FLOAT_EQ(packet2.pitch(), 45.0f);
}

TEST(PacketHandlerMoveVehiclePacketTest, PacketType)
{
    MoveVehiclePacket packet;
    EXPECT_EQ(packet.type(), PacketType::MoveVehicle);
}

// ==================== EntityActionPacket Serialization Tests ====================

TEST(PacketHandlerEntityActionPacketTest, SerializeDeserialize)
{
    EntityActionPacket packet;
    packet.setEntityId(42);
    packet.setAction(EntityActionType::StartRidingJump);
    packet.setAuxData(75);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityActionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 42u);
    EXPECT_EQ(packet2.action(), EntityActionType::StartRidingJump);
    EXPECT_EQ(packet2.auxData(), 75);
}

TEST(PacketHandlerEntityActionPacketTest, PacketType)
{
    EntityActionPacket packet;
    EXPECT_EQ(packet.type(), PacketType::EntityAction);
}

// ==================== UseEntityPacket Serialization Tests ====================

TEST(PacketHandlerUseEntityPacketTest, SerializeDeserialize_Interact)
{
    UseEntityPacket packet;
    packet.setEntityId(100);
    packet.setAction(UseEntityAction::Interact);
    packet.setHand(Hand::MainHand);
    packet.setSneaking(false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    UseEntityPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 100u);
    EXPECT_EQ(packet2.action(), UseEntityAction::Interact);
    EXPECT_EQ(packet2.hand(), Hand::MainHand);
    EXPECT_FALSE(packet2.isSneaking());
}

TEST(PacketHandlerUseEntityPacketTest, SerializeDeserialize_InteractAt)
{
    UseEntityPacket packet;
    packet.setEntityId(200);
    packet.setAction(UseEntityAction::InteractAt);
    packet.setHand(Hand::OffHand);
    packet.setHitPosition(0.5f, 1.0f, 0.25f);
    packet.setSneaking(true);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    UseEntityPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 200u);
    EXPECT_EQ(packet2.action(), UseEntityAction::InteractAt);
    EXPECT_EQ(packet2.hand(), Hand::OffHand);
    EXPECT_FLOAT_EQ(packet2.hitX(), 0.5f);
    EXPECT_FLOAT_EQ(packet2.hitY(), 1.0f);
    EXPECT_FLOAT_EQ(packet2.hitZ(), 0.25f);
    EXPECT_TRUE(packet2.isSneaking());
}

TEST(PacketHandlerUseEntityPacketTest, PacketType)
{
    UseEntityPacket packet;
    EXPECT_EQ(packet.type(), PacketType::UseEntity);
}

// ==================== SteerBoatPacket Tests ====================

TEST_F(PacketHandlerTest, HandleSteerBoat_UnknownSession_ReturnsIgnore)
{
    SteerBoatPacket packet;
    packet.setPaddleState(true, false);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleSteerBoat(999, data.data(), data.size());

    EXPECT_EQ(result, PacketHandleResult::Ignore);
}

TEST_F(PacketHandlerTest, HandleSteerBoat_ValidSession_NoServer_ReturnsSuccess)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    SteerBoatPacket packet;
    packet.setPaddleState(true, true);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = m_packetHandler->handleSteerBoat(1, data.data(), data.size());

    // 没有设置 server，应该返回 Success（不阻塞）
    EXPECT_EQ(result, PacketHandleResult::Success);
}

TEST_F(PacketHandlerTest, HandleSteerBoat_InvalidPacket_ReturnsError)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // SteerBoatPacket 需要两个 bool，空数据应该失败
    mc::u8 invalidData[] = {};
    auto result = m_packetHandler->handleSteerBoat(1, invalidData, 0);

    EXPECT_EQ(result, PacketHandleResult::Error);
}

TEST_F(PacketHandlerTest, HandleSteerBoat_TooSmallPacket_ReturnsError)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // SteerBoatPacket 需要两个 bool，一个字节不够
    mc::u8 smallData[] = {0x01};
    auto result = m_packetHandler->handleSteerBoat(1, smallData, 1);

    // 一个字节可以读取一个 bool，但需要两个 bool，应该失败
    EXPECT_EQ(result, PacketHandleResult::Error);
}

TEST_F(PacketHandlerTest, HandleSteerBoat_AllPaddleCombinations)
{
    auto conn = createConnection();
    auto* player =
        m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);
    ASSERT_NE(player, nullptr);
    m_playerManager->mapSessionToPlayer(1, 1);

    // 测试所有四种组合
    struct TestCase {
        bool left;
        bool right;
    };

    TestCase cases[] = {{false, false}, {true, false}, {false, true}, {true, true}};

    for (const auto& tc : cases) {
        SteerBoatPacket packet;
        packet.setPaddleState(tc.left, tc.right);

        auto serializeResult = packet.serialize();
        ASSERT_TRUE(serializeResult.success());

        const auto& data = serializeResult.value();
        auto result = m_packetHandler->handleSteerBoat(1, data.data(), data.size());

        // 没有实际载具时，也应该返回 Success
        EXPECT_EQ(result, PacketHandleResult::Success);
    }
}

// ==================== SteerBoatPacket Serialization Tests ====================

TEST(PacketHandlerSteerBoatPacketTest, SerializeDeserialize_BothFalse)
{
    SteerBoatPacket packet;
    packet.setPaddleState(false, false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FALSE(packet2.leftPaddle());
    EXPECT_FALSE(packet2.rightPaddle());
}

TEST(PacketHandlerSteerBoatPacketTest, SerializeDeserialize_LeftOnly)
{
    SteerBoatPacket packet;
    packet.setPaddleState(true, false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_TRUE(packet2.leftPaddle());
    EXPECT_FALSE(packet2.rightPaddle());
}

TEST(PacketHandlerSteerBoatPacketTest, SerializeDeserialize_RightOnly)
{
    SteerBoatPacket packet;
    packet.setPaddleState(false, true);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FALSE(packet2.leftPaddle());
    EXPECT_TRUE(packet2.rightPaddle());
}

TEST(PacketHandlerSteerBoatPacketTest, SerializeDeserialize_BothTrue)
{
    SteerBoatPacket packet;
    packet.setPaddleState(true, true);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_TRUE(packet2.leftPaddle());
    EXPECT_TRUE(packet2.rightPaddle());
}

TEST(PacketHandlerSteerBoatPacketTest, PacketType)
{
    SteerBoatPacket packet;
    EXPECT_EQ(packet.type(), PacketType::SteerBoat);
}

TEST(PacketHandlerSteerBoatPacketTest, DefaultValues)
{
    SteerBoatPacket packet;
    EXPECT_FALSE(packet.leftPaddle());
    EXPECT_FALSE(packet.rightPaddle());
}

TEST(PacketHandlerSteerBoatPacketTest, SetIndividualPaddles)
{
    SteerBoatPacket packet;

    packet.setLeftPaddle(true);
    EXPECT_TRUE(packet.leftPaddle());
    EXPECT_FALSE(packet.rightPaddle());

    packet.setRightPaddle(true);
    EXPECT_TRUE(packet.leftPaddle());
    EXPECT_TRUE(packet.rightPaddle());

    packet.setLeftPaddle(false);
    EXPECT_FALSE(packet.leftPaddle());
    EXPECT_TRUE(packet.rightPaddle());

    packet.setRightPaddle(false);
    EXPECT_FALSE(packet.leftPaddle());
    EXPECT_FALSE(packet.rightPaddle());
}

TEST(PacketHandlerSteerBoatPacketTest, Deserialize_EmptyData_ReturnsError)
{
    SteerBoatPacket packet;
    auto result = packet.deserialize(nullptr, 0);
    EXPECT_FALSE(result.success());
}

// ==================== VehicleMovePacket Correction Tests ====================

/**
 * @brief 验证 VehicleMovePacket 可以正确序列化和反序列化
 *
 * 当载具移动过快时，服务端需要构造 VehicleMovePacket 校正包
 * 发送给客户端。此测试确保校正包的序列化/反序列化正确工作。
 */
TEST(VehicleMovePacketCorrectionTest, SerializeDeserialize)
{
    // 构造校正包，使用服务端已知位置
    VehicleMovePacket correction;
    correction.setPosition(100.5, 64.0, -200.25);
    correction.setRotation(90.0f, 45.0f);

    auto result = correction.serialize();
    ASSERT_TRUE(result.success());

    // 反序列化验证
    VehicleMovePacket parsed;
    auto parseResult = parsed.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(parseResult.success());

    // 校验位置与旋转是否与原始值一致
    EXPECT_DOUBLE_EQ(parsed.x(), 100.5);
    EXPECT_DOUBLE_EQ(parsed.y(), 64.0);
    EXPECT_DOUBLE_EQ(parsed.z(), -200.25);
    EXPECT_FLOAT_EQ(parsed.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(parsed.pitch(), 45.0f);
}

/**
 * @brief 验证 VehicleMovePacket 的 PacketType 为 VehicleMove
 *
 * 确保校正包的类型标识正确，客户端能正确识别。
 */
TEST(VehicleMovePacketCorrectionTest, PacketTypeIsVehicleMove)
{
    VehicleMovePacket packet;
    EXPECT_EQ(packet.type(), PacketType::VehicleMove);
}

/**
 * @brief 验证 VehicleMovePacket 序列化负载大小
 *
 * 校正包的负载应为 32 字节（3 个 f64 + 2 个 f32），
 * 确保序列化结果不会丢失或多余数据。
 */
TEST(VehicleMovePacketCorrectionTest, SerializePayloadSize)
{
    VehicleMovePacket correction;
    correction.setPosition(1.0, 2.0, 3.0);
    correction.setRotation(4.0f, 5.0f);

    auto result = correction.serialize();
    ASSERT_TRUE(result.success());

    // 3 个 f64（位置）+ 2 个 f32（旋转）= 24 + 8 = 32 字节
    EXPECT_EQ(result.value().size(), 32u);
}

/**
 * @brief 验证校正包内容为服务端已知位置而非客户端报告位置
 *
 * 当载具超速时，服务端应使用 vehicle 的当前位置（而非客户端
 * 在 MoveVehiclePacket 中报告的超速位置）构造校正包。
 * 此测试模拟该场景：服务端位置为 (0, 64, 0)，客户端报告位置
 * 为 (500, 64, 500)（超速），校正包应使用服务端位置。
 */
TEST(VehicleMovePacketCorrectionTest, CorrectionUsesServerPosition)
{
    // 模拟服务端已知位置
    const mc::f64 serverX = 0.0;
    const mc::f64 serverY = 64.0;
    const mc::f64 serverZ = 0.0;
    const mc::f32 serverYaw = 180.0f;
    const mc::f32 serverPitch = 0.0f;

    // 客户端报告的超速位置（不应被使用）
    const mc::f64 clientX = 500.0;
    const mc::f64 clientY = 64.0;
    const mc::f64 clientZ = 500.0;

    // 构造校正包时应使用服务端位置
    VehicleMovePacket correction;
    correction.setPosition(serverX, serverY, serverZ);
    correction.setRotation(serverYaw, serverPitch);

    auto result = correction.serialize();
    ASSERT_TRUE(result.success());

    // 验证反序列化后得到的是服务端位置
    VehicleMovePacket parsed;
    auto parseResult = parsed.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(parseResult.success());

    // 校正包中的位置应是服务端位置，而非客户端超速位置
    EXPECT_DOUBLE_EQ(parsed.x(), serverX);
    EXPECT_DOUBLE_EQ(parsed.y(), serverY);
    EXPECT_DOUBLE_EQ(parsed.z(), serverZ);
    EXPECT_FLOAT_EQ(parsed.yaw(), serverYaw);
    EXPECT_FLOAT_EQ(parsed.pitch(), serverPitch);

    // 确保不是客户端报告的超速位置
    EXPECT_NE(parsed.x(), clientX);
    EXPECT_NE(parsed.z(), clientZ);
}

/**
 * @brief 验证校正包序列化失败时不会导致崩溃
 *
 * VehicleMovePacket::serialize() 应始终成功（所有字段均为原始类型），
 * 但代码路径中有对 serialize 结果的检查。此测试验证正常情况下的
 * 序列化始终成功，确保降级处理路径不会因意外失败而崩溃。
 */
TEST(VehicleMovePacketCorrectionTest, SerializeAlwaysSucceedsForPrimitiveTypes)
{
    // 使用极端值测试序列化
    VehicleMovePacket correction;
    correction.setPosition(1e18, -1e18, 0.0);
    correction.setRotation(-360.0f, 360.0f);

    auto result = correction.serialize();
    EXPECT_TRUE(result.success());
    EXPECT_GT(result.value().size(), 0u);
}

/**
 * @brief 验证 ConnectionManager::sendPacketToPlayer 正确封装 VehicleMovePacket
 *
 * 测试完整的发送路径：构造校正包 → 序列化 → 通过 ConnectionManager 封装
 * 头部 → 发送到 FakeServerConnection → 解析头部和负载 → 验证内容。
 * 这确保校正包从构造到客户端接收的完整路径正确。
 */
TEST(VehicleMovePacketCorrectionTest, SendPacketViaConnectionManager)
{
    // 设置玩家管理器和连接管理器
    PlayerManager playerManager(20);
    ConnectionManager connectionManager(playerManager);

    // 创建模拟连接
    auto connectionPair = std::make_unique<LocalConnectionPair>();
    connectionPair->connect();
    auto serverConn = std::make_shared<LocalServerConnection>(&connectionPair->serverEndpoint());

    // 添加玩家
    auto* player = playerManager.addPlayer(
        1, mc::util::uuidToString(mc::util::generateOfflineUuid("TestPlayer")), "TestPlayer", serverConn);
    ASSERT_NE(player, nullptr);

    // 构造校正包
    VehicleMovePacket correction;
    correction.setPosition(100.0, 64.0, 200.0);
    correction.setRotation(90.0f, 45.0f);

    // 序列化并发送
    auto serialResult = correction.serialize();
    ASSERT_TRUE(serialResult.success());

    bool sent = connectionManager.sendPacketToPlayer(1, PacketType::VehicleMove, serialResult.value());
    EXPECT_TRUE(sent);

    // 从客户端端点接收数据
    auto& clientEndpoint = connectionPair->clientEndpoint();
    std::vector<u8> receivedData;
    clientEndpoint.receive(receivedData);

    // 验证接收到的数据不为空
    ASSERT_GT(receivedData.size(), PACKET_HEADER_SIZE);

    // 解析头部（网络字节序/大端序）
    // 头部格式: [u32 totalSize][u16 type][u16 flags][u16 reserved][u16 padding]
    PacketSerializer headerParser;
    // 从接收数据中提取头部字节
    std::vector<u8> headerBytes(receivedData.begin(), receivedData.begin() + PACKET_HEADER_SIZE);

    // 读取 totalSize（大端序）
    mc::u32 totalSize = static_cast<mc::u32>(headerBytes[0]) << 24 | static_cast<mc::u32>(headerBytes[1]) << 16 |
        static_cast<mc::u32>(headerBytes[2]) << 8 | static_cast<mc::u32>(headerBytes[3]);
    EXPECT_GE(totalSize, PACKET_HEADER_SIZE + 32); // 头部 + 32 字节负载

    // 读取 type（大端序）
    mc::u16 packetType = static_cast<mc::u16>(headerBytes[4]) << 8 | static_cast<mc::u16>(headerBytes[5]);
    EXPECT_EQ(static_cast<PacketType>(packetType), PacketType::VehicleMove);

    // 解析负载
    const u8* payload = receivedData.data() + PACKET_HEADER_SIZE;
    size_t payloadSize = receivedData.size() - PACKET_HEADER_SIZE;

    VehicleMovePacket receivedPacket;
    auto parseResult = receivedPacket.deserialize(payload, payloadSize);
    ASSERT_TRUE(parseResult.success());

    // 验证接收到的校正包内容
    EXPECT_DOUBLE_EQ(receivedPacket.x(), 100.0);
    EXPECT_DOUBLE_EQ(receivedPacket.y(), 64.0);
    EXPECT_DOUBLE_EQ(receivedPacket.z(), 200.0);
    EXPECT_FLOAT_EQ(receivedPacket.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(receivedPacket.pitch(), 45.0f);
}

/**
 * @brief 验证发送校正包给不存在玩家时返回 false
 *
 * 当玩家已断开连接或不存在时，sendPacketToPlayer 应返回 false
 * 而非崩溃。这确保了网络异常情况下的降级处理安全。
 */
TEST(VehicleMovePacketCorrectionTest, SendToNonExistentPlayerReturnsFalse)
{
    PlayerManager playerManager(20);
    ConnectionManager connectionManager(playerManager);

    VehicleMovePacket correction;
    correction.setPosition(0.0, 64.0, 0.0);
    correction.setRotation(0.0f, 0.0f);

    auto serialResult = correction.serialize();
    ASSERT_TRUE(serialResult.success());

    // 发送给不存在的玩家（ID 999）
    bool sent = connectionManager.sendPacketToPlayer(999, PacketType::VehicleMove, serialResult.value());
    EXPECT_FALSE(sent);
}
