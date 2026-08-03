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

#include "client/chat/ChatHistory.hpp"
#include "client/command/ClientCommandManager.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/text/ITextComponent.hpp"
#include <memory>

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client {
class Font;
}

namespace mc::client::ui::minecraft::widgets {

// 引入类型
using chat::ChatHistory;
using chat::ChatMessageType;
namespace text = mc::text;

/**
 * @brief 聊天Widget
 *
 * 处理聊天框的显示和交互，包括：
 * - 消息显示
 * - 文本输入
 * - 命令历史导航
 * - 光标和选区
 */
class ChatWidget : public kagero::widget::ContainerWidget {
public:
    /**
     * @brief 命令发送回调类型
     */
    using CommandCallback = std::function<void(const std::string&)>;

    /**
     * @brief 动作栏消息回调类型
     *
     * 当收到 Actionbar/GameInfo 类型的消息时触发，用于将消息路由到 TitleWidget 显示。
     * @param text 消息文本
     */
    using ActionbarCallback = std::function<void(const std::string& text)>;

    ChatWidget();
    ~ChatWidget() override = default;

    // ========== 初始化 ==========

    /**
     * @brief 设置字体
     */
    void setFont(Font* font) { m_font = font; }

    /**
     * @brief 设置GUI渲染器
     */
    void setGuiRenderer(renderer::trident::gui::GuiRenderer* gui) { m_gui = gui; }

    /**
     * @brief 设置命令回调
     */
    void setCommandCallback(CommandCallback callback)
    {
        MC_ASSERT_RELEASE(!m_commandCallback);
        m_commandCallback = std::move(callback);
    }

    /**
     * @brief 设置命令补全管理器
     */
    void setCommandManager(mc::client::command::ClientCommandManager* commandManager)
    {
        MC_ASSERT_RELEASE(!m_commandManager);
        m_commandManager = commandManager;
        _updateCommandSuggestions();
    }

    /**
     * @brief 设置动作栏回调
     *
     * 当收到 Actionbar/GameInfo 类型的消息时，调用此回调将消息
     * 路由到 TitleWidget 进行动作栏显示（屏幕中央偏下位置），
     * 而非在聊天窗口中显示。与 MC Java 版一致：
     * Chat/System 消息显示在聊天窗口，Actionbar/GameInfo 消息显示在动作栏。
     */
    void setActionbarCallback(ActionbarCallback callback) { m_actionbarCallback = std::move(callback); }

    // ========== 状态 ==========

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] bool isOpen() const { return m_open; }

    /**
     * @brief 打开聊天框
     * @param command 是否以命令模式打开（自动填入 /）
     */
    void open(bool command);

    /**
     * @brief 关闭聊天框
     */
    void close();

    /**
     * @brief 切换聊天框状态
     */
    void toggle();

    // ========== Widget接口 ==========

    /**
     * @brief 绘制聊天框
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 每帧更新
     */
    void tick(f32 dt) override;

    /**
     * @brief 处理键盘按键
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

    /**
     * @brief 处理字符输入
     */
    bool onChar(u32 codePoint) override;

    /**
     * @brief 处理鼠标点击
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;

    // ========== 消息 ==========

    /**
     * @brief 添加聊天消息
     * @param message 消息文本
     * @param type 消息类型（默认 Chat）
     */
    void addMessage(const std::string& message, ChatMessageType type = ChatMessageType::Chat);

    /**
     * @brief 添加富文本消息
     */
    void addMessage(std::unique_ptr<text::ITextComponent> message, chat::ChatMessageType type);

    /**
     * @brief 添加系统消息
     */
    void addSystemMessage(const std::string& message);

    /**
     * @brief 获取聊天历史
     */
    [[nodiscard]] ChatHistory& history() { return m_history; }
    [[nodiscard]] const ChatHistory& history() const { return m_history; }

    // ========== 输入 ==========

    /**
     * @brief 获取当前输入
     */
    [[nodiscard]] const std::string& input() const { return m_input; }

    /**
     * @brief 设置输入文本
     */
    void setInput(const std::string& text);

    /**
     * @brief 清除输入
     */
    void clearInput();

private:
    // ========== 内部方法 ==========

    /**
     * @brief 插入文本到光标位置
     */
    void _insertText(const std::string& text);

    /**
     * @brief 删除选中的文本
     */
    void _deleteSelection();

    /**
     * @brief 删除光标前的字符
     */
    void _deleteBeforeCursor();

    /**
     * @brief 删除光标后的字符
     */
    void _deleteAfterCursor();

    /**
     * @brief 移动光标
     */
    void _moveCursor(i32 offset, bool selecting);

    /**
     * @brief 移动光标到行首/行尾
     */
    void _moveCursorToEdge(bool start, bool selecting);

    /**
     * @brief 获取光标位置对应的像素偏移
     */
    [[nodiscard]] f32 _getCursorPixelPosition() const;

    /**
     * @brief 发送当前输入
     */
    void _sendInput();

    /**
     * @brief 更新光标闪烁
     */
    void _updateCursorBlink(f32 dt);

    /**
     * @brief 更新命令补全建议
     */
    void _updateCommandSuggestions();

    /**
     * @brief 清空命令补全建议
     */
    void _clearCommandSuggestions();

    /**
     * @brief 接受当前命令补全建议
     */
    void _acceptCommandSuggestion();

    /**
     * @brief 渲染命令补全列表
     */
    void _renderCommandSuggestions(kagero::widget::PaintContext& ctx);

    /**
     * @brief 判断当前输入是否为命令
     */
    [[nodiscard]] bool _isCommandInput() const;

    /**
     * @brief 渲染消息列表
     */
    void _renderMessages(kagero::widget::PaintContext& ctx);

    /**
     * @brief 渲染输入框
     */
    void _renderInputBox(kagero::widget::PaintContext& ctx);

    // ========== 成员变量 ==========

    ChatHistory m_history;
    std::string m_input;
    size_t m_cursorPos = 0;
    size_t m_selectionStart = 0;
    size_t m_selectionEnd = 0;
    bool m_hasSelection = false;
    bool m_open = false;
    bool m_commandMode = false;

    Font* m_font = nullptr;
    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
    mc::client::command::ClientCommandManager* m_commandManager = nullptr;

    mc::command::Suggestions m_commandSuggestions;
    size_t m_selectedCommandSuggestion = 0;

    // 光标闪烁
    f32 m_cursorBlinkTimer = 0.0f;
    bool m_cursorVisible = true;
    static constexpr f32 CURSOR_BLINK_RATE = 0.5f;

    // 布局常量
    static constexpr f32 INPUT_BOX_HEIGHT = 20.0f;
    static constexpr f32 INPUT_BOX_PADDING = 4.0f;
    static constexpr f32 CHAT_WIDTH_RATIO = 0.4f;
    static constexpr f32 MESSAGE_FADE_START = 3.0f;
    static constexpr f32 LINE_HEIGHT = 9.0f;              // 字体行高
    static constexpr f32 INPUT_BOX_BOTTOM_MARGIN = 10.0f; // 输入框距底部的间距
    static constexpr f32 MESSAGE_BOTTOM_MARGIN = 20.0f;   // 消息区域距底部的间距（含输入框空间）
    static constexpr f32 TEXT_HORIZONTAL_PADDING = 4.0f;  // 文本水平内边距
    static constexpr f32 MESSAGE_FADE_DURATION = 3.0f;    // 消息淡出持续时间（秒）
    static constexpr u32 MAX_VISIBLE_SUGGESTIONS = 6;     // 最大可见命令补全建议数

    // 命令回调
    CommandCallback m_commandCallback;

    // 动作栏回调
    ActionbarCallback m_actionbarCallback;
};

} // namespace mc::client::ui::minecraft::widgets
