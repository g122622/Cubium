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
 * @brief ServerPlayer 消息发送功能测试（新网络层 IR 版本）
 *
 * 新网络层 ServerPlayer::setConnection 接受 mc::server::net::ServerClientConnection*
 * （裸指针），sendStatusMessage 在有连接时发 ir::play::SetActionBarText/SetTitleText
 * 等 IR 包，无连接时直接 return（不抛）。
 *
 * 本测试聚焦无连接路径的核心契约（canReceiveMessages=false、sendStatusMessage 不抛）。
 * 旧"有连接 + 字节解析标题/聊天广播"用例依赖旧 12 字节头 + 旧 packet 枚举，新层为
 * IR 包无字节头，已移除。有连接发包路径由 ServerClientConnection
 * + ClientPlayVisitor 集成测试覆盖（sendSystemMessage/sendChatMessage 现发 SystemChat IR）。
 */

#include "common/core/Types.hpp"
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
        // 创建 ServerPlayer（无连接）
        m_player = std::make_unique<ServerPlayer>(1, "TestPlayer");
    }

    void TearDown() override { m_player.reset(); }

    std::unique_ptr<ServerPlayer> m_player;
};

// ========== canReceiveMessages 测试（无连接路径）==========

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseWithoutConnection)
{
    // 没有连接时，canReceiveMessages 应该返回 false
    EXPECT_FALSE(m_player->canReceiveMessages());
    EXPECT_FALSE(m_player->hasConnection());
}

TEST_F(ServerPlayerMessageTest, CanReceiveMessagesFalseAfterNullConnection)
{
    // 显式设置为空连接
    m_player->setConnection(nullptr);
    EXPECT_FALSE(m_player->canReceiveMessages());
    EXPECT_FALSE(m_player->hasConnection());
}

// ========== sendStatusMessage 测试（无连接路径，不抛异常）==========

TEST_F(ServerPlayerMessageTest, SendStatusMessageDoesNotThrowWithoutConnection)
{
    // 没有连接时发送消息也不应该抛出异常（实现直接 return）
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.message"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("another.message", true));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithEmptyString)
{
    // 空字符串应该也能正常调用（不抛）
    EXPECT_NO_THROW(m_player->sendStatusMessage(""));
    EXPECT_NO_THROW(m_player->sendStatusMessage("", true));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithTranslationKeys)
{
    // 测试所有睡眠相关翻译键（无连接，仅验证不抛）
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.occupied"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.too_far_away"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.obstructed"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.no_sleep"));
    EXPECT_NO_THROW(m_player->sendStatusMessage("block.minecraft.bed.not_safe"));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageWithLongMessage)
{
    // 长消息也应该正常调用（不抛）
    std::string longMessage(1000, 'a');
    EXPECT_NO_THROW(m_player->sendStatusMessage(longMessage));
}

TEST_F(ServerPlayerMessageTest, SendStatusMessageActionBarNoConnection)
{
    // 没有连接时发送 actionBar 消息不应该抛出异常
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.actionbar", true));
    EXPECT_NO_THROW(m_player->sendStatusMessage("test.noplayer", true));
}

TEST_F(ServerPlayerMessageTest, SendSystemMessageDoesNotThrowWithoutConnection)
{
    // sendSystemMessage 无连接时不抛（有连接时发 SystemChat IR 包）
    EXPECT_NO_THROW(m_player->sendSystemMessage("system.message"));
}

// ========== 多态性测试（无连接路径）==========

TEST_F(ServerPlayerMessageTest, PolymorphicCallThroughBasePointer)
{
    // 使用基类指针调用（无连接）
    Player* basePtr = m_player.get();

    // 基类指针调用应该使用 ServerPlayer 的实现
    EXPECT_FALSE(basePtr->canReceiveMessages());
    EXPECT_NO_THROW(basePtr->sendStatusMessage("test.polymorphism"));
}

} // namespace
} // namespace mc
