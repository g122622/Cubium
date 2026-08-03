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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <chrono>
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::chat {

/**
 * @brief 聊天消息类型
 */
enum class ChatMessageType : u8 {
    Chat,      ///< 玩家聊天
    System,    ///< 系统消息
    Actionbar, ///< 动作栏消息
    GameInfo   ///< 游戏信息
};

/**
 * @brief 聊天消息
 *
 * 支持富文本内容，包含时间戳和消息类型。
 */
struct ChatMessage {
    std::unique_ptr<text::ITextComponent> content;   ///< 消息内容（富文本）
    ChatMessageType type = ChatMessageType::Chat;    ///< 消息类型
    std::chrono::steady_clock::time_point timestamp; ///< 时间戳
    bool permanent = false;                          ///< 是否永久显示（不淡出）

    ChatMessage() = default;

    /**
     * @brief 从纯文本构造消息
     * @param text 纯文本内容
     * @param msgType 消息类型
     * @param perm 是否永久显示
     */
    explicit ChatMessage(const std::string& text, ChatMessageType msgType, bool perm)
        : content(std::make_unique<text::StringTextComponent>(text))
        , type(msgType)
        , timestamp(std::chrono::steady_clock::now())
        , permanent(perm)
    {}

    /**
     * @brief 从文本组件构造消息
     * @param textComponent 文本组件（所有权转移）
     * @param msgType 消息类型
     * @param perm 是否永久显示
     */
    explicit ChatMessage(std::unique_ptr<text::ITextComponent> textComponent, ChatMessageType msgType, bool perm)
        : content(std::move(textComponent))
        , type(msgType)
        , timestamp(std::chrono::steady_clock::now())
        , permanent(perm)
    {}

    /**
     * @brief 获取纯文本内容
     * @return 纯文本字符串
     */
    [[nodiscard]] std::string getPlainText() const { return content ? content->getUnformattedText() : ""; }

    /**
     * @brief 获取格式化文本（§ 代码格式）
     * @return 格式化文本字符串
     */
    [[nodiscard]] std::string getFormattedText() const { return content ? content->getFormattedText() : ""; }
};

/**
 * @brief 聊天历史管理器
 *
 * 管理聊天消息历史，支持：
 * - 消息添加和过期
 * - 消息历史导航
 * - 命令历史
 */
class ChatHistory {
public:
    static constexpr size_t MAX_MESSAGES = 100;     ///< 最大消息数
    static constexpr size_t MAX_VISIBLE = 10;       ///< 最大可见消息数
    static constexpr size_t MAX_INPUT_HISTORY = 50; ///< 最大输入历史
    static constexpr f32 MESSAGE_FADE_TIME = 5.0f;  ///< 消息淡出时间（秒）

    ChatHistory() = default;

    // ========== 消息管理 ==========

    /**
     * @brief 添加聊天消息（纯文本）
     * @param message 消息文本
     * @param type 消息类型
     * @param permanent 是否永久显示
     */
    void addMessage(const std::string& message, ChatMessageType type = ChatMessageType::Chat, bool permanent = false);

    /**
     * @brief 添加聊天消息（富文本）
     * @param message 消息组件（所有权转移）
     * @param type 消息类型
     * @param permanent 是否永久显示
     */
    void addMessage(std::unique_ptr<text::ITextComponent> message,
        ChatMessageType type = ChatMessageType::Chat,
        bool permanent = false);

    /**
     * @brief 添加系统消息
     * @param message 消息文本
     */
    void addSystemMessage(const std::string& message);

    /**
     * @brief 清除所有消息
     */
    void clear();

    /**
     * @brief 获取可见消息
     * @param includeFading 是否包含正在淡出的消息
     */
    [[nodiscard]] std::vector<ChatMessage> getVisibleMessages(bool includeFading) const;

    /**
     * @brief 获取所有消息
     */
    [[nodiscard]] const std::deque<ChatMessage>& allMessages() const { return m_messages; }

    // ========== 输入历史 ==========

    /**
     * @brief 添加到输入历史
     * @param input 输入文本
     */
    void addToInputHistory(const std::string& input);

    /**
     * @brief 获取上一个输入
     */
    [[nodiscard]] std::string getPreviousInput();

    /**
     * @brief 获取下一个输入
     */
    [[nodiscard]] std::string getNextInput();

    /**
     * @brief 重置输入历史导航
     */
    void resetInputNavigation();

    /**
     * @brief 清除输入历史
     */
    void clearInputHistory();

private:
    std::deque<ChatMessage> m_messages;
    std::vector<std::string> m_inputHistory;
    size_t m_historyIndex = 0;
    std::string m_savedInput; ///< 导航时保存的当前输入
};

} // namespace mc::client::chat
