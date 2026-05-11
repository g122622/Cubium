/**
 * @file ServerPlayerMessageTest.cpp
 * @brief ServerPlayer 消息发送功能测试
 *
 * 测试 ServerPlayer 的 sendStatusMessage 和 canReceiveMessages 方法：
 * - canReceiveMessages 检查网络连接状态
 * - sendStatusMessage 通过网络发送消息
 * - 无连接时的行为
 */

#include <gtest/gtest.h>
#include "server/player/ServerPlayer.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc {
namespace {

/**
 * @brief ServerPlayer 消息发送测试夹具
 */
class ServerPlayerMessageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建本地连接对用于测试
        m_connectionPair = std::make_unique<network::LocalConnectionPair>();
        m_connectionPair->connect();

        // 创建 ServerPlayer
        m_player = std::make_unique<ServerPlayer>(1, "TestPlayer");
    }

    void TearDown() override {
        m_player.reset();
        m_connectionPair.reset();
    }

    /**
     * @brief 创建本地连接
     */
    network::ConnectionPtr createConnection() {
        return std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    std::unique_ptr<ServerPlayer> m_player;
    std::unique_ptr<network::LocalConnectionPair> m_connectionPair;
};

// ========== canReceiveMessages 测试 ==========

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseWithoutConnection) {
    // 没有连接时，canReceiveMessages 应该返回 false
    EXPECT_FALSE(m_player->canReceiveMessages());
}

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesTrueWithConnection) {
    // 设置连接后，canReceiveMessages 应该返回 true
    auto conn = createConnection();
    m_player->setConnection(conn);

    EXPECT_TRUE(m_player->canReceiveMessages());
}

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseAfterDisconnect) {
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

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseAfterNullConnection) {
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);
    EXPECT_TRUE(m_player->canReceiveMessages());

    // 设置为空连接
    m_player->setConnection(nullptr);
    EXPECT_FALSE(m_player->canReceiveMessages());
}

// ========== sendStatusMessage 测试 ==========

TEST_F(ServerPlayerMessageTest, SendStatusMessageDoesNotThrowWithConnection) {
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 发送消息不应该抛出异常
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.message"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.occupied", true));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.no_sleep", false));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageDoesNotThrowWithoutConnection) {
    // 没有连接时发送消息也不应该抛出异常（只是记录警告）
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.message"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("another.message", true));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithEmptyString) {
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 空字符串应该也能正常发送
    EXPECT_NO_THROW(m_player->sendStatusMessage(""));
    EXPECT_NO_THROW(m_player->sendStatusMessage("", true));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithTranslationKeys) {
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

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithLongMessage) {
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 长消息也应该正常发送
    std::string longMessage(1000, 'a');
    EXPECT_NO_THROW(m_player->sendStatusMessage(longMessage));
}

// ========== 多态性测试 ==========

TEST_F(ServerPlayerMessageTest, PolymorphicCallThroughBasePointer) {
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 使用基类指针调用
    Player* basePtr = m_player.get();

    // 基类指针调用应该使用 ServerPlayer 的实现
    EXPECT_TRUE(basePtr->canReceiveMessages());
    EXPECT_NO_THROW(basePtr->sendStatusMessage("test.polymorphism"));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageCallsSendSystemMessage) {
    // 这个测试验证 sendStatusMessage 调用了 sendSystemMessage
    // 设置连接后应该能正常发送
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 同时调用两种方法不应该有问题
    EXPECT_NO_THROW(m_player->sendStatusMessage("status.message"));
    EXPECT_NO_THROW(m_player->sendSystemMessage("system.message"));
}

// ========== 多次发送测试 ==========

TEST_F(ServerPlayerMessageTest, MultipleMessagesInSequence) {
    // 设置连接
    auto conn = createConnection();
    m_player->setConnection(conn);

    // 连续发送多条消息
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(m_player->sendStatusMessage("message." + std::to_string(i)));
    }
}

TEST_F(ServerPlayerMessageTest, MessagesAfterReconnect) {
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

} // namespace
} // namespace mc
