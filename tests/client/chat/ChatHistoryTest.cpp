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

#include <gtest/gtest.h>

#include "client/chat/ChatHistory.hpp"

using namespace mc::client::chat;
using namespace mc::text;

// ============================================================================
// ChatMessageType 枚举测试
// ============================================================================

TEST(ChatMessageType, EnumValues)
{
    // 验证消息类型枚举值存在且可比较
    EXPECT_NE(ChatMessageType::Chat, ChatMessageType::System);
    EXPECT_NE(ChatMessageType::Chat, ChatMessageType::Actionbar);
    EXPECT_NE(ChatMessageType::Chat, ChatMessageType::GameInfo);
    EXPECT_NE(ChatMessageType::System, ChatMessageType::Actionbar);
    EXPECT_NE(ChatMessageType::System, ChatMessageType::GameInfo);
    EXPECT_NE(ChatMessageType::Actionbar, ChatMessageType::GameInfo);
}

// ============================================================================
// ChatHistory 基础消息测试
// ============================================================================

TEST(ChatHistory, AddChatMessage)
{
    ChatHistory history;
    history.addMessage("Hello world", ChatMessageType::Chat);

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].getPlainText(), "Hello world");
    EXPECT_EQ(messages[0].type, ChatMessageType::Chat);
    EXPECT_FALSE(messages[0].permanent);
}

TEST(ChatHistory, AddSystemMessage)
{
    ChatHistory history;
    history.addSystemMessage("Server started");

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].getPlainText(), "Server started");
    EXPECT_EQ(messages[0].type, ChatMessageType::System);
    EXPECT_FALSE(messages[0].permanent);
}

TEST(ChatHistory, AddActionbarMessage)
{
    ChatHistory history;
    history.addMessage("Action bar text", ChatMessageType::Actionbar);

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].getPlainText(), "Action bar text");
    EXPECT_EQ(messages[0].type, ChatMessageType::Actionbar);
}

TEST(ChatHistory, AddGameInfoMessage)
{
    ChatHistory history;
    history.addMessage("Game info", ChatMessageType::GameInfo);

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].getPlainText(), "Game info");
    EXPECT_EQ(messages[0].type, ChatMessageType::GameInfo);
}

TEST(ChatHistory, AddRichTextMessage)
{
    ChatHistory history;
    auto content = std::make_unique<StringTextComponent>("Rich text");
    history.addMessage(std::move(content), ChatMessageType::System);

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].getPlainText(), "Rich text");
    EXPECT_EQ(messages[0].type, ChatMessageType::System);
}

// ============================================================================
// ChatHistory 消息类型区分测试
// ============================================================================

TEST(ChatHistory, MixedMessageTypes)
{
    ChatHistory history;
    history.addMessage("Chat message", ChatMessageType::Chat);
    history.addMessage("System message", ChatMessageType::System);
    history.addMessage("Actionbar message", ChatMessageType::Actionbar);
    history.addMessage("Game info message", ChatMessageType::GameInfo);

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 4u);

    // 消息按添加顺序的逆序存储（最新在前）
    EXPECT_EQ(messages[0].type, ChatMessageType::GameInfo);
    EXPECT_EQ(messages[1].type, ChatMessageType::Actionbar);
    EXPECT_EQ(messages[2].type, ChatMessageType::System);
    EXPECT_EQ(messages[3].type, ChatMessageType::Chat);
}

TEST(ChatHistory, SystemMessageHasGrayStyle)
{
    ChatHistory history;
    history.addSystemMessage("System notification");

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 1u);

    // addSystemMessage 应设置灰色样式
    const auto& style = messages[0].content->getStyle();
    EXPECT_TRUE(style.getColor().has_value());
    EXPECT_EQ(style.getColor().value(), TextFormatting::Gray);
}

TEST(ChatHistory, ChatMessageNoExplicitColor)
{
    ChatHistory history;
    history.addMessage("Player chat", ChatMessageType::Chat);

    const auto& messages = history.allMessages();
    ASSERT_EQ(messages.size(), 1u);

    // 聊天消息不应有显式颜色样式
    const auto& style = messages[0].content->getStyle();
    EXPECT_FALSE(style.getColor().has_value());
}

// ============================================================================
// ChatHistory 消息限制测试
// ============================================================================

TEST(ChatHistory, MaxMessagesLimit)
{
    ChatHistory history;

    // 添加超过最大消息数的消息
    for (size_t i = 0; i < ChatHistory::MAX_MESSAGES + 10; ++i) {
        history.addMessage("Message " + std::to_string(i), ChatMessageType::Chat);
    }

    const auto& messages = history.allMessages();
    EXPECT_EQ(messages.size(), ChatHistory::MAX_MESSAGES);
}

TEST(ChatHistory, ClearMessages)
{
    ChatHistory history;
    history.addMessage("Message 1", ChatMessageType::Chat);
    history.addMessage("Message 2", ChatMessageType::System);
    ASSERT_EQ(history.allMessages().size(), 2u);

    history.clear();
    EXPECT_EQ(history.allMessages().size(), 0u);
}

// ============================================================================
// ChatHistory 输入历史测试
// ============================================================================

TEST(ChatHistory, InputHistoryNavigation)
{
    ChatHistory history;
    history.addToInputHistory("/say hello");
    history.addToInputHistory("/time set day");

    EXPECT_EQ(history.getPreviousInput(), "/time set day");
    EXPECT_EQ(history.getPreviousInput(), "/say hello");
    EXPECT_EQ(history.getNextInput(), "/time set day");
}

TEST(ChatHistory, InputHistoryDeduplication)
{
    ChatHistory history;
    history.addToInputHistory("/say hello");
    history.addToInputHistory("/say hello"); // 重复输入应被过滤

    EXPECT_EQ(history.getPreviousInput(), "/say hello");
    EXPECT_EQ(history.getPreviousInput(), "/say hello"); // 只有一条记录
}

TEST(ChatHistory, InputHistoryResetNavigation)
{
    ChatHistory history;
    history.addToInputHistory("/command1");
    history.addToInputHistory("/command2");
    history.getPreviousInput(); // 开始导航

    history.resetInputNavigation();

    // 重置后应从头开始导航
    EXPECT_EQ(history.getPreviousInput(), "/command2");
}

TEST(ChatHistory, EmptyInputHistoryNavigation)
{
    ChatHistory history;
    EXPECT_EQ(history.getPreviousInput(), "");
    EXPECT_EQ(history.getNextInput(), "");
}

// ============================================================================
// ChatHistory 可见消息测试
// ============================================================================

TEST(ChatHistory, VisibleMessagesIncludePermanent)
{
    ChatHistory history;
    history.addMessage("Permanent", ChatMessageType::Chat, true);
    history.addMessage("Normal", ChatMessageType::Chat, false);

    auto visible = history.getVisibleMessages(false);
    // 永久消息应始终可见
    ASSERT_EQ(visible.size(), 1u);
    EXPECT_EQ(visible[0].getPlainText(), "Permanent");
    EXPECT_TRUE(visible[0].permanent);
}

TEST(ChatHistory, VisibleMessagesIncludeFading)
{
    ChatHistory history;
    history.addMessage("Normal", ChatMessageType::Chat, false);

    // 刚添加的消息应包含在 includeFading=true 的结果中
    auto visible = history.getVisibleMessages(true);
    ASSERT_EQ(visible.size(), 1u);
    EXPECT_EQ(visible[0].getPlainText(), "Normal");
}

TEST(ChatHistory, MaxVisibleMessagesLimit)
{
    ChatHistory history;

    // 添加超过最大可见数的永久消息
    for (size_t i = 0; i < ChatHistory::MAX_VISIBLE + 5; ++i) {
        history.addMessage("Message " + std::to_string(i), ChatMessageType::Chat, true);
    }

    auto visible = history.getVisibleMessages(false);
    EXPECT_EQ(visible.size(), ChatHistory::MAX_VISIBLE);
}

// ============================================================================
// ChatMessage 构造测试
// ============================================================================

TEST(ChatMessage, PlainTextConstruction)
{
    ChatMessage msg("Test message", ChatMessageType::System, false);
    EXPECT_EQ(msg.getPlainText(), "Test message");
    EXPECT_EQ(msg.type, ChatMessageType::System);
    EXPECT_FALSE(msg.permanent);
}

TEST(ChatMessage, RichTextConstruction)
{
    auto content = std::make_unique<StringTextComponent>("Rich content");
    ChatMessage msg(std::move(content), ChatMessageType::Actionbar, true);
    EXPECT_EQ(msg.getPlainText(), "Rich content");
    EXPECT_EQ(msg.type, ChatMessageType::Actionbar);
    EXPECT_TRUE(msg.permanent);
}

TEST(ChatMessage, DefaultValues)
{
    ChatMessage msg;
    EXPECT_EQ(msg.getPlainText(), "");
    EXPECT_EQ(msg.type, ChatMessageType::Chat);
    EXPECT_FALSE(msg.permanent);
}
