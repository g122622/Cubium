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

#include "ChatWidget.hpp"
#include "client/command/ClientCommandManager.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/Utf8.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft::widgets {

namespace {

using mc::f32;
using mc::client::Font;
using mc::client::renderer::trident::gui::GuiRenderer;

[[nodiscard]] f32 measureTextWidth(Font* font, GuiRenderer* gui, const std::string& text)
{
    if (font != nullptr) {
        return font->getStringWidthUTF8(text);
    }

    if (gui != nullptr) {
        return static_cast<f32>(gui->getTextWidth(text));
    }

    // TODO: 当 font 和 gui 均不可用时，应提供更好的回退策略，而非简单估算
    return static_cast<f32>(util::text::utf8CodepointCount(text) * 6);
}

[[nodiscard]] size_t findCursorIndexFromClick(const std::string& text, Font* font, GuiRenderer* gui, f32 clickX)
{
    if (text.empty() || clickX <= 0.0f) {
        return 0;
    }

    const f32 totalWidth = measureTextWidth(font, gui, text);
    if (clickX >= totalWidth) {
        return text.size();
    }

    size_t index = 0;
    f32 previousWidth = 0.0f;
    while (index < text.size()) {
        const size_t nextIndex = util::text::utf8NextCodepointIndex(text, index);
        const f32 nextWidth = measureTextWidth(font, gui, text.substr(0, nextIndex));

        if (clickX <= nextWidth) {
            if ((clickX - previousWidth) <= (nextWidth - clickX)) {
                return index;
            }
            return nextIndex;
        }

        previousWidth = nextWidth;
        index = nextIndex;
    }

    return text.size();
}

[[nodiscard]] std::string sanitizeClipboardText(std::string text)
{
    // 按码点过滤：移除控制字符（保留换行符），替换为空格
    std::string result;
    result.reserve(text.size());
    util::text::utf8ForEachCodepoint(text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) {
        if (codePoint < 32u) {
            if (codePoint == U'\n') {
                result += '\n';
            } else {
                result += ' ';
            }
        } else if (codePoint == 127u) {
            result += ' ';
        } else {
            util::text::utf8Append(result, codePoint);
        }
    });

    return result;
}

} // namespace

ChatWidget::ChatWidget()
    : ContainerWidget()
    , m_open(false)
    , m_commandMode(false)
{
    setId("chat");
    setVisible(true);
    setActive(false); // 默认不激活，打开时才激活
}

void ChatWidget::open(bool command)
{
    m_open = true;
    m_commandMode = command;
    if (command && m_input.empty()) {
        m_input = "/";
        m_cursorPos = 1;
    }
    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
    _updateCommandSuggestions();
    setActive(true); // 打开时激活，接收事件
}

void ChatWidget::close()
{
    m_open = false;
    m_commandMode = false;
    clearInput();
    _clearCommandSuggestions();
    m_history.resetInputNavigation(); // 重置历史导航
    setActive(false);                 // 关闭时取消激活
}

void ChatWidget::toggle()
{
    if (m_open) {
        close();
    } else {
        open(false);
    }
}

void ChatWidget::paint(kagero::widget::PaintContext& ctx)
{
    // 渲染消息列表
    _renderMessages(ctx);

    // 如果打开，渲染输入框
    if (m_open) {
        _renderInputBox(ctx);
    }
}

void ChatWidget::tick(f32 dt)
{
    if (m_open) {
        _updateCursorBlink(dt);
    }
}

bool ChatWidget::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    if (!m_open || action != GLFW_PRESS) {
        return false;
    }

    switch (key) {
        case GLFW_KEY_ESCAPE:
            close();
            return true;

        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            _sendInput();
            return true;

        case GLFW_KEY_TAB:
            _acceptCommandSuggestion();
            return true;

        case GLFW_KEY_BACKSPACE:
            if (m_hasSelection) {
                _deleteSelection();
            } else {
                _deleteBeforeCursor();
            }
            return true;

        case GLFW_KEY_DELETE:
            if (m_hasSelection) {
                _deleteSelection();
            } else {
                _deleteAfterCursor();
            }
            return true;

        case GLFW_KEY_LEFT:
            if (mods & GLFW_MOD_CONTROL) {
                _moveCursorToEdge(true, mods & GLFW_MOD_SHIFT);
            } else {
                _moveCursor(-1, mods & GLFW_MOD_SHIFT);
            }
            return true;

        case GLFW_KEY_RIGHT:
            if (mods & GLFW_MOD_CONTROL) {
                _moveCursorToEdge(false, mods & GLFW_MOD_SHIFT);
            } else {
                _moveCursor(1, mods & GLFW_MOD_SHIFT);
            }
            return true;

        case GLFW_KEY_HOME:
            _moveCursorToEdge(true, mods & GLFW_MOD_SHIFT);
            return true;

        case GLFW_KEY_END:
            _moveCursorToEdge(false, mods & GLFW_MOD_SHIFT);
            return true;

        case GLFW_KEY_UP:
            // 浏览命令历史（使用 ChatHistory 的方法）
            setInput(m_history.getPreviousInput());
            return true;

        case GLFW_KEY_DOWN:
            // 浏览命令历史（使用 ChatHistory 的方法）
            setInput(m_history.getNextInput());
            return true;

        case GLFW_KEY_A:
            if (mods & GLFW_MOD_CONTROL) {
                // 全选
                m_selectionStart = 0;
                m_selectionEnd = m_input.size();
                m_hasSelection = true;
                return true;
            }
            break;

        case GLFW_KEY_C:
            if (mods & GLFW_MOD_CONTROL && m_hasSelection) {
                GLFWwindow* window = glfwGetCurrentContext();
                if (window != nullptr) {
                    const size_t start = std::min(m_selectionStart, m_selectionEnd);
                    const size_t end = std::max(m_selectionStart, m_selectionEnd);
                    const std::string selectedText = m_input.substr(start, end - start);
                    glfwSetClipboardString(window, selectedText.c_str());
                }
                return true;
            }
            break;

        case GLFW_KEY_V:
            if (mods & GLFW_MOD_CONTROL) {
                GLFWwindow* window = glfwGetCurrentContext();
                if (window != nullptr) {
                    const char* clipboardText = glfwGetClipboardString(window);
                    if (clipboardText != nullptr && clipboardText[0] != '\0') {
                        std::string pastedText = sanitizeClipboardText(std::string(clipboardText));
                        if (!pastedText.empty()) {
                            if (m_hasSelection) {
                                _deleteSelection();
                            }
                            _insertText(pastedText);
                        }
                    }
                }
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

bool ChatWidget::onChar(u32 codePoint)
{
    if (!m_open) {
        return false;
    }

    // 忽略控制字符
    if (codePoint < 32) {
        return false;
    }

    // 删除选中内容
    if (m_hasSelection) {
        _deleteSelection();
    }

    // 插入字符（使用 UTF-8 编码）
    _insertText(util::text::utf8Encode(codePoint));
    return true;
}

bool ChatWidget::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    (void)mods;
    if (!m_open) {
        return false;
    }

    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return true;
    }

    const f32 screenHeight = static_cast<f32>(height());
    const f32 inputY = screenHeight - INPUT_BOX_HEIGHT - INPUT_BOX_BOTTOM_MARGIN;
    const f32 mouseYf = static_cast<f32>(mouseY);
    if (mouseYf < inputY || mouseYf > inputY + INPUT_BOX_HEIGHT) {
        return true;
    }

    const f32 textX = INPUT_BOX_PADDING + TEXT_HORIZONTAL_PADDING;
    const f32 clickX = static_cast<f32>(mouseX) - textX;

    m_cursorPos = findCursorIndexFromClick(m_input, m_font, m_gui, clickX);
    m_selectionStart = m_cursorPos;
    m_selectionEnd = m_cursorPos;
    m_hasSelection = false;
    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
    _updateCommandSuggestions();
    return true;
}

void ChatWidget::addMessage(const std::string& message, ChatMessageType type)
{
    // Actionbar/GameInfo 消息不进入聊天历史，而是路由到动作栏回调
    if (type == ChatMessageType::Actionbar || type == ChatMessageType::GameInfo) {
        if (m_actionbarCallback) {
            m_actionbarCallback(message);
        }
        return;
    }

    m_history.addMessage(message, type);
}

void ChatWidget::addMessage(std::unique_ptr<text::ITextComponent> message, ChatMessageType type)
{
    // Actionbar/GameInfo 消息不进入聊天历史，而是路由到动作栏回调
    if (type == ChatMessageType::Actionbar || type == ChatMessageType::GameInfo) {
        if (m_actionbarCallback) {
            m_actionbarCallback(message ? message->getUnformattedText() : "");
        }
        return;
    }

    m_history.addMessage(std::move(message), type);
}

void ChatWidget::addSystemMessage(const std::string& message)
{
    m_history.addSystemMessage(message);
}

void ChatWidget::setInput(const std::string& text)
{
    m_input = text;
    m_cursorPos = m_input.size();
    m_hasSelection = false;
    _updateCommandSuggestions();
}

void ChatWidget::clearInput()
{
    m_input.clear();
    m_cursorPos = 0;
    m_selectionStart = 0;
    m_selectionEnd = 0;
    m_hasSelection = false;
    _updateCommandSuggestions();
}

void ChatWidget::_insertText(const std::string& text)
{
    m_input.insert(m_cursorPos, text);
    m_cursorPos += text.size();
    m_hasSelection = false;
    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
    _updateCommandSuggestions();
}

void ChatWidget::_deleteSelection()
{
    if (!m_hasSelection) {
        return;
    }

    size_t start = std::min(m_selectionStart, m_selectionEnd);
    size_t end = std::max(m_selectionStart, m_selectionEnd);

    m_input.erase(start, end - start);
    m_cursorPos = start;
    m_hasSelection = false;
    _updateCommandSuggestions();
}

void ChatWidget::_deleteBeforeCursor()
{
    if (m_cursorPos > 0) {
        const size_t prevPos = util::text::utf8PrevCodepointIndex(m_input, m_cursorPos);
        m_input.erase(prevPos, m_cursorPos - prevPos);
        m_cursorPos = prevPos;
        _updateCommandSuggestions();
    }
}

void ChatWidget::_deleteAfterCursor()
{
    if (m_cursorPos < m_input.size()) {
        const size_t nextPos = util::text::utf8NextCodepointIndex(m_input, m_cursorPos);
        m_input.erase(m_cursorPos, nextPos - m_cursorPos);
        _updateCommandSuggestions();
    }
}

void ChatWidget::_moveCursor(i32 offset, bool selecting)
{
    if (selecting && !m_hasSelection) {
        m_selectionStart = m_cursorPos;
        m_hasSelection = true;
    }

    // 按码点移动光标，offset = +1 向右移动一个码点，-1 向左移动一个码点
    if (offset > 0) {
        for (i32 i = 0; i < offset && m_cursorPos < m_input.size(); ++i) {
            m_cursorPos = util::text::utf8NextCodepointIndex(m_input, m_cursorPos);
        }
    } else if (offset < 0) {
        for (i32 i = 0; i < -offset && m_cursorPos > 0; ++i) {
            m_cursorPos = util::text::utf8PrevCodepointIndex(m_input, m_cursorPos);
        }
    }

    if (selecting) {
        m_selectionEnd = m_cursorPos;
    } else {
        m_hasSelection = false;
    }

    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
    _updateCommandSuggestions();
}

void ChatWidget::_moveCursorToEdge(bool start, bool selecting)
{
    if (selecting && !m_hasSelection) {
        m_selectionStart = m_cursorPos;
        m_hasSelection = true;
    }

    m_cursorPos = start ? 0 : m_input.size();

    if (selecting) {
        m_selectionEnd = m_cursorPos;
    } else {
        m_hasSelection = false;
    }

    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
    _updateCommandSuggestions();
}

f32 ChatWidget::_getCursorPixelPosition() const
{
    return measureTextWidth(m_font, m_gui, m_input.substr(0, m_cursorPos));
}

void ChatWidget::_sendInput()
{
    if (m_input.empty()) {
        close();
        return;
    }

    // 添加到输入历史（使用 ChatHistory 的方法，已包含大小限制和去重）
    m_history.addToInputHistory(m_input);

    // 调用回调
    if (m_commandCallback) {
        m_commandCallback(m_input);
    }

    close();
}

void ChatWidget::_updateCommandSuggestions()
{
    if (!m_open || !_isCommandInput() || m_commandManager == nullptr) {
        _clearCommandSuggestions();
        return;
    }

    const i32 cursor = static_cast<i32>(std::min(m_cursorPos, m_input.size()));
    auto result = m_commandManager->getSuggestions(m_input, cursor);
    m_commandSuggestions = std::move(result);
    m_selectedCommandSuggestion = 0;
}

void ChatWidget::_clearCommandSuggestions()
{
    m_commandSuggestions = mc::command::Suggestions();
    m_selectedCommandSuggestion = 0;
}

void ChatWidget::_acceptCommandSuggestion()
{
    if (m_commandSuggestions.isEmpty()) {
        _updateCommandSuggestions();
        if (m_commandSuggestions.isEmpty()) {
            return;
        }
    }

    const auto& suggestions = m_commandSuggestions.getList();
    if (suggestions.empty()) {
        return;
    }

    const size_t index = std::min(m_selectedCommandSuggestion, suggestions.size() - 1);
    const auto& suggestion = suggestions[index];
    const size_t insertStart =
        static_cast<size_t>(std::clamp(suggestion.getStart(), 0, static_cast<i32>(m_input.size())));
    const size_t cursor = std::min(m_cursorPos, m_input.size());

    std::string completed = m_input.substr(0, insertStart);
    completed += suggestion.getText();
    if (cursor < m_input.size()) {
        completed += m_input.substr(cursor);
    }

    m_input = std::move(completed);
    m_cursorPos = insertStart + suggestion.getText().size();
    m_hasSelection = false;
    m_selectionStart = m_cursorPos;
    m_selectionEnd = m_cursorPos;
    _updateCommandSuggestions();
}

void ChatWidget::_renderCommandSuggestions(kagero::widget::PaintContext& ctx)
{
    if (!m_open || m_commandSuggestions.isEmpty() || m_gui == nullptr) {
        return;
    }

    const auto& suggestions = m_commandSuggestions.getList();
    if (suggestions.empty()) {
        return;
    }

    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());
    const f32 chatWidth = screenWidth * CHAT_WIDTH_RATIO;
    constexpr f32 padding = INPUT_BOX_PADDING;

    size_t visibleCount = std::min<size_t>(suggestions.size(), MAX_VISIBLE_SUGGESTIONS);
    f32 maxTextWidth = 0.0f;
    for (size_t index = 0; index < visibleCount; ++index) {
        maxTextWidth = std::max(maxTextWidth, static_cast<f32>(m_gui->getTextWidth(suggestions[index].getText())));
    }

    const f32 boxWidth = std::min(chatWidth, maxTextWidth + padding * 2.0f + TEXT_HORIZONTAL_PADDING + 2.0f);
    const f32 boxHeight = static_cast<f32>(visibleCount) * (LINE_HEIGHT + 2.0f) + padding * 2.0f;
    const f32 inputY = screenHeight - INPUT_BOX_HEIGHT - INPUT_BOX_BOTTOM_MARGIN;
    const f32 boxX = INPUT_BOX_PADDING;
    const f32 boxY = std::max(INPUT_BOX_PADDING, inputY - boxHeight - INPUT_BOX_PADDING);

    ctx.drawFilledRect(
        kagero::Rect(
            static_cast<i32>(boxX), static_cast<i32>(boxY), static_cast<i32>(boxWidth), static_cast<i32>(boxHeight)),
        0xB0202020);

    for (size_t index = 0; index < visibleCount; ++index) {
        const f32 rowY = boxY + padding + static_cast<f32>(index) * (LINE_HEIGHT + 2.0f);
        const bool selected = index == m_selectedCommandSuggestion;
        ctx.drawFilledRect(kagero::Rect(static_cast<i32>(boxX + 1.0f),
                               static_cast<i32>(rowY - 1.0f),
                               static_cast<i32>(boxWidth - 2.0f),
                               static_cast<i32>(LINE_HEIGHT + 2.0f)),
            selected ? 0x60FFFFFF : 0x20000000);

        const u32 textColor = selected ? 0xFF111111 : 0xFFFFFFFF;
        m_gui->drawText(suggestions[index].getText(), boxX + padding + 2.0f, rowY, textColor, false);
    }
}

bool ChatWidget::_isCommandInput() const
{
    return m_commandMode || (!m_input.empty() && m_input.front() == '/');
}

void ChatWidget::_updateCursorBlink(f32 dt)
{
    m_cursorBlinkTimer += dt;
    if (m_cursorBlinkTimer >= CURSOR_BLINK_RATE) {
        m_cursorBlinkTimer = 0.0f;
        m_cursorVisible = !m_cursorVisible;
    }
}

void ChatWidget::_renderMessages(kagero::widget::PaintContext& ctx)
{
    if (m_gui == nullptr) {
        return;
    }

    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());
    const f32 chatWidth = screenWidth * CHAT_WIDTH_RATIO;
    constexpr f32 padding = INPUT_BOX_PADDING;

    // 从底部向上渲染消息
    const auto& messages = m_history.allMessages();
    f32 y = screenHeight - MESSAGE_BOTTOM_MARGIN;

    auto now = std::chrono::steady_clock::now();

    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        if (y < 0.0f) {
            break; // 超出屏幕顶部
        }

        // Actionbar/GameInfo 消息不应出现在聊天窗口中，跳过
        // （正常情况下不会出现在历史中，因为 addMessage 已路由到动作栏回调）
        if (it->type == ChatMessageType::Actionbar || it->type == ChatMessageType::GameInfo) {
            continue;
        }

        // 计算消息透明度（旧消息淡出）
        f32 alpha = 1.0f;
        if (!m_open && !it->permanent) {
            // 关闭状态时，旧消息淡出
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->timestamp).count();
            f32 age = static_cast<f32>(elapsed);
            if (age > MESSAGE_FADE_START) {
                alpha = std::max(0.0f, 1.0f - (age - MESSAGE_FADE_START) / MESSAGE_FADE_DURATION);
            }
        }

        if (alpha > 0.01f) {
            // 获取消息文本
            std::string plainText = it->getPlainText();

            // 渲染消息背景
            f32 textWidth = static_cast<f32>(m_gui->getTextWidth(plainText));
            ctx.drawFilledRect(kagero::Rect(static_cast<i32>(padding),
                                   static_cast<i32>(y - LINE_HEIGHT),
                                   static_cast<i32>(textWidth + padding * 2),
                                   static_cast<i32>(LINE_HEIGHT + 2)),
                static_cast<u32>(0x80000000 * alpha));

            // 根据消息类型选择文本颜色
            // 与 MC Java 一致：玩家消息为白色，系统消息为灰色
            u32 baseColor = 0xFFFFFFFF; // 白色默认（玩家聊天消息）
            if (it->type == ChatMessageType::System) {
                baseColor = 0xFFAAAAAA; // 灰色（与 ChatHistory::addSystemMessage 的 Gray 样式一致）
            }

            u8 a = static_cast<u8>(255 * alpha);
            u32 color = (baseColor & 0x00FFFFFF) | (static_cast<u32>(a) << 24);
            m_gui->drawText(plainText, padding + 2.0f, y - LINE_HEIGHT, color, true);
        }

        y -= LINE_HEIGHT + 2.0f;
    }
}

void ChatWidget::_renderInputBox(kagero::widget::PaintContext& ctx)
{
    if (m_gui == nullptr) {
        return;
    }

    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());
    const f32 chatWidth = screenWidth * CHAT_WIDTH_RATIO;
    const f32 inputY = screenHeight - INPUT_BOX_HEIGHT - INPUT_BOX_BOTTOM_MARGIN;

    // 渲染输入框背景
    ctx.drawFilledRect(kagero::Rect(static_cast<i32>(INPUT_BOX_PADDING),
                           static_cast<i32>(inputY),
                           static_cast<i32>(chatWidth),
                           static_cast<i32>(INPUT_BOX_HEIGHT)),
        0x80000000);

    // 渲染输入文本
    f32 textX = INPUT_BOX_PADDING + TEXT_HORIZONTAL_PADDING;
    f32 textY = inputY + (INPUT_BOX_HEIGHT - LINE_HEIGHT) / 2.0f;
    m_gui->drawText(m_input, textX, textY, 0xFFFFFFFF, false);

    // 渲染命令补全列表
    _renderCommandSuggestions(ctx);

    // 渲染光标
    if (m_cursorVisible) {
        f32 cursorX = textX + _getCursorPixelPosition();
        ctx.drawFilledRect(
            kagero::Rect(
                static_cast<i32>(cursorX), static_cast<i32>(textY - 1.0f), 1, static_cast<i32>(LINE_HEIGHT + 2.0f)),
            0xFFFFFFFF);
    }
}

} // namespace mc::client::ui::minecraft::widgets
