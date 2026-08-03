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

#include "ChatHistory.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::chat {

void ChatHistory::addMessage(const std::string& message, ChatMessageType type, bool permanent)
{
    m_messages.emplace_front(std::make_unique<text::StringTextComponent>(message), type, permanent);

    // 限制消息数量
    while (m_messages.size() > MAX_MESSAGES) {
        m_messages.pop_back();
    }
}

void ChatHistory::addMessage(std::unique_ptr<text::ITextComponent> message, ChatMessageType type, bool permanent)
{
    m_messages.emplace_front(std::move(message), type, permanent);

    // 限制消息数量
    while (m_messages.size() > MAX_MESSAGES) {
        m_messages.pop_back();
    }
}

void ChatHistory::addSystemMessage(const std::string& message)
{
    // 系统消息使用灰色样式
    auto content = std::make_unique<text::StringTextComponent>(message);
    text::Style style;
    style.setColor(text::TextFormatting::Gray);
    content->setStyle(style);
    addMessage(std::move(content), ChatMessageType::System, false);
}

void ChatHistory::clear()
{
    m_messages.clear();
}

std::vector<ChatMessage> ChatHistory::getVisibleMessages(bool includeFading) const
{
    std::vector<ChatMessage> result;
    auto now = std::chrono::steady_clock::now();

    size_t count = 0;
    for (const auto& msg : m_messages) {
        if (count >= MAX_VISIBLE) break;

        if (msg.permanent) {
            ChatMessage msgCopy;
            msgCopy.content = msg.content ? msg.content->deepCopy() : std::make_unique<text::StringTextComponent>("");
            msgCopy.type = msg.type;
            msgCopy.timestamp = msg.timestamp;
            msgCopy.permanent = msg.permanent;
            result.push_back(std::move(msgCopy));
            count++;
        } else if (includeFading) {
            // 计算消息年龄
            auto age = std::chrono::duration<f32>(now - msg.timestamp).count();
            if (age < MESSAGE_FADE_TIME + 1.0f) { // 额外1秒淡出时间
                ChatMessage msgCopy;
                msgCopy.content =
                    msg.content ? msg.content->deepCopy() : std::make_unique<text::StringTextComponent>("");
                msgCopy.type = msg.type;
                msgCopy.timestamp = msg.timestamp;
                msgCopy.permanent = msg.permanent;
                result.push_back(std::move(msgCopy));
                count++;
            }
        }
    }

    return result;
}

void ChatHistory::addToInputHistory(const std::string& input)
{
    if (input.empty()) return;

    // 避免重复
    if (!m_inputHistory.empty() && m_inputHistory.back() == input) return;

    m_inputHistory.push_back(input);

    // 限制历史大小
    while (m_inputHistory.size() > MAX_INPUT_HISTORY) {
        m_inputHistory.erase(m_inputHistory.begin());
    }

    // 重置导航索引
    m_historyIndex = m_inputHistory.size();
}

std::string ChatHistory::getPreviousInput()
{
    if (m_inputHistory.empty()) return "";

    if (m_historyIndex == m_inputHistory.size()) {
        // 保存当前输入
        m_savedInput = "";
    }

    if (m_historyIndex > 0) {
        m_historyIndex--;
        return m_inputHistory[m_historyIndex];
    }

    return m_inputHistory[0];
}

std::string ChatHistory::getNextInput()
{
    if (m_inputHistory.empty()) return "";

    if (m_historyIndex < m_inputHistory.size() - 1) {
        m_historyIndex++;
        return m_inputHistory[m_historyIndex];
    }

    // 到达底部，返回保存的输入
    m_historyIndex = m_inputHistory.size();
    return m_savedInput;
}

void ChatHistory::resetInputNavigation()
{
    m_historyIndex = m_inputHistory.size();
    m_savedInput.clear();
}

void ChatHistory::clearInputHistory()
{
    m_inputHistory.clear();
    m_historyIndex = 0;
    m_savedInput.clear();
}

} // namespace mc::client::chat
