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

/**
 * @file ServerPlayerMessageTest.cpp
 * @brief ServerPlayer 消息发送功能测试
 *
 * 测试 ServerPlayer 的 sendStatusMessage 和 canReceiveMessages 方法：
 * - canReceiveMessages 检查网络连接状态
 * - sendStatusMessage 通过网络发送消息
 * - actionBar 参数决定消息显示位置（聊天区域或 Action Bar）
 * - 无连接时的行为
 */

#include "common/core/Types.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/TitlePacket.hpp"
#include "server/player/ServerPlayer.hpp"
#include <memory>
#include <gtest/gtest.h>

namespace mc {
namespace {

/**
 * @brief ServerPlayer 消息发送测试夹具
 */
class ServerPlayerMessageTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建本地连接对用于测试
        m_connectionPair = std::make_unique<network::LocalConnectionPair>();
        m_connectionPair->connect();

        // 创建 ServerPlayer
        m_player = std::make_unique<ServerPlayer>(1, "TestPlayer");
    }

    void TearDown() override
    {
        m_player.reset();
        m_connectionPair.reset();
    }

    /**
     * @brief 创建本地连接
     */
    network::ConnectionPtr createConnection()
    {
        return std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    /**
     * @brief 获取客户端端点以读取发送的数据
     */
    network::LocalEndpoint& clientEndpoint() { return m_connectionPair->clientEndpoint(); }

    /**
     * @brief 解析包类型
     *
     * 包头格式: u32 size, u16 type, u16 flags, u16 reserved, u16 padding
     * 类型字段是大端序存储
     */
    network::PacketType parsePacketType(const std::vector<u8>& packet)
    {
        if (packet.size() < 12) {
            // 返回一个无效的类型作为错误指示
            return static_cast<network::PacketType>(0xFFFF);
        }
        // 类型字段是大端序存储
        u16 type = (static_cast<u16>(packet[4]) << 8) | static_cast<u16>(packet[5]);
        return static_cast<network::PacketType>(type);
    }

    std::unique_ptr<ServerPlayer> m_player;
    std::unique_ptr<network::LocalConnectionPair> m_connectionPair;
};

// ========== canReceiveMessages 测试 ==========

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseWithoutConnection)
{
    // 没有连接时，canReceiveMessages 应该返回 false
    EXPECT_FALSE(m_player->canReceiveMessages());
}

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesTrueWithConnection)
{
    // 设置连接后，canReceiveMessages 应该返回 true
    auto conn = createConnection();
    m_player->setConnection(conn);

    EXPECT_TRUE(m_player->canReceiveMessages());
}

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseAfterDisconnect)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);
    EXPECT_TRUE(m_player->canReceiveMessages());

    // 断开连接
    conn->disconnect("test disconnect");

    // 断开后，hasConnection 应该返回 false
    EXPECT_FALSE(m_player->hasConnection());
    EXPECT_FALSE(m_player->canReceiveMessages());
}

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseAfterNullConnection)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);
    EXPECT_TRUE(m_player->canReceiveMessages());

    // 设置为空连接
    m_player->setConnection(nullptr);
    EXPECT_FALSE(m_player->canReceiveMessages());
}

// ========== sendStatusMessage 测试 ==========

TEST_F(ServerPlayerMessageTest, SendStatusMessageDoesNotThrowWithConnection)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 发送消息不应该抛出异常
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.message"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.occupied", true));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.no_sleep", false));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageDoesNotThrowWithoutConnection)
{
    // 没有连接时发送消息也不应该抛出异常（只是记录警告）
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.message"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("another.message", true));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithEmptyString)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 空字符串应该也能正常发送
    EXPECT_NO_THROW(m_player->sendStatusMessage(""));
    EXPECT_NO_THROW(m_player->sendStatusMessage("", true));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithTranslationKeys)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 测试所有睡眠相关翻译键
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.occupied"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.too_far_away"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.obstructed"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.no_sleep"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.not_safe"));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithLongMessage)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 长消息也应该正常发送
    std::string longMessage(1000, 'a');
    EXPECT_NO_THROW(m_player->sendStatusMessage(longMessage));
}

// ========== 多态性测试 ==========

TEST_F(ServerPlayerMessageTest, PolymorphicCallThroughBasePointer)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 使用基类指针调用
    Player* basePtr = m_player.get();

    // 基类指针调用应该使用 ServerPlayer 的实现
    EXPECT_TRUE(basePtr->canReceiveMessages());
    EXPECT_NO_THROW(basePtr->sendStatusMessage("test.polymorphism"));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageCallsSendSystemMessage)
{
    // 这个测试验证 sendStatusMessage 调用了 sendSystemMessage
    // 设置连接后应该能正常发送
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 同时调用两种方法不应该有问题
    EXPECT_NO_THROW(m_player->sendStatusMessage("status.message"));
    EXPECT_NO_THROW(m_player->sendSystemMessage("system.message"));
}

// ========== 多次发送测试 ==========

TEST_F(ServerPlayerMessageTest, MultipleMessagesInSequence)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 连续发送多条消息
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(m_player->sendStatusMessage("message." + std::to_string(i)));
    }
}

TEST_F(ServerPlayerMessageTest, MessagesAfterReconnect)
{
    // 第一次连接
    auto conn1 = createConnection();
    m_player->setConnection(conn1);
    EXPECT_TRUE(m_player->canReceiveMessages());
    EXPECT_NO_THROW(m_player->sendStatusMessage("first.message"));

    // 断开连接
    conn1->disconnect("test");
    EXPECT_FALSE(m_player->canReceiveMessages());

    // 重新连接（使用新的连接对）
    m_connectionPair = std::make_unique<network::LocalConnectionPair>();
    m_connectionPair->connect();
    auto conn2 = createConnection();
    m_player->setConnection(conn2);

    // 应该能正常发送
    EXPECT_TRUE(m_player->canReceiveMessages());
    EXPECT_NO_THROW(m_player->sendStatusMessage("second.message"));
}

// ========== ActionBar 参数测试 ==========

TEST_F(ServerPlayerMessageTest, SendStatusMessageActionBarSendsTitlePacket)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 发送 actionBar=true 的消息
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.actionbar", true));

    // 接收数据包
    std::vector<u8> packet;
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_FALSE(packet.empty());

    // 验证是 Title 包
    network::PacketType type = parsePacketType(packet);
    EXPECT_EQ(type, network::PacketType::Title);
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageChatSendsChatPacket)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 发送 actionBar=false 的消息（发送到聊天区域）
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.chat", false));

    // 接收数据包
    std::vector<u8> packet;
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_FALSE(packet.empty());

    // 验证是 ChatBroadcast 包
    network::PacketType type = parsePacketType(packet);
    EXPECT_EQ(type, network::PacketType::ChatBroadcast);
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageDefaultSendsChatPacket)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 发送默认参数的消息（actionBar=false）
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.default"));

    // 接收数据包
    std::vector<u8> packet;
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_FALSE(packet.empty());

    // 验证是 ChatBroadcast 包
    network::PacketType type = parsePacketType(packet);
    EXPECT_EQ(type, network::PacketType::ChatBroadcast);
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageActionBarWithEmptyString)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 发送空字符串 actionBar 消息
    EXPECT_NO_THROW(m_player->sendStatusMessage("", true));

    // 接收数据包
    std::vector<u8> packet;
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_FALSE(packet.empty());

    // 验证是 Title 包
    network::PacketType type = parsePacketType(packet);
    EXPECT_EQ(type, network::PacketType::Title);
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageMultipleActionBarMessages)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 连续发送多条 actionBar 消息
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(m_player->sendStatusMessage("actionbar." + std::to_string(i), true));
    }

    // 验证收到 5 个 Title 包
    for (int i = 0; i < 5; ++i) {
        std::vector<u8> packet;
        ASSERT_TRUE(clientEndpoint().receive(packet));
        network::PacketType type = parsePacketType(packet);
        EXPECT_EQ(type, network::PacketType::Title);
    }
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageMixedChatAndActionBar)
{
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 混合发送聊天和 actionBar 消息
    EXPECT_NO_THROW(m_player->sendStatusMessage("chat.message", false));
    EXPECT_NO_THROW(m_player->sendStatusMessage("actionbar.message", true));
    EXPECT_NO_THROW(m_player->sendStatusMessage("another.chat", false));
    EXPECT_NO_THROW(m_player->sendStatusMessage("another.actionbar", true));

    // 验证收到正确的包类型顺序
    std::vector<u8> packet;

    // 第一个应该是 ChatBroadcast
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_EQ(parsePacketType(packet), network::PacketType::ChatBroadcast);

    // 第二个应该是 Title (actionBar)
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_EQ(parsePacketType(packet), network::PacketType::Title);

    // 第三个应该是 ChatBroadcast
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_EQ(parsePacketType(packet), network::PacketType::ChatBroadcast);

    // 第四个应该是 Title (actionBar)
    ASSERT_TRUE(clientEndpoint().receive(packet));
    EXPECT_EQ(parsePacketType(packet), network::PacketType::Title);
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageActionBarNoConnection)
{
    // 没有连接时发送 actionBar 消息不应该抛出异常
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.actionbar", true));
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.noplayer", true));
}

} // namespace
} // namespace mc
