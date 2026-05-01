#pragma once

#include "../../kagero/widget/ContainerWidget.hpp"
#include "../../kagero/paint/PaintContext.hpp"
#include "../../../chat/ChatHistory.hpp"
#include "client/command/ClientCommandManager.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/text/ITextComponent.hpp"

#include <functional>
#include <chrono>

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
 *
 * 参考 MC 的 ChatScreen 实现
 */
class ChatWidget : public kagero::widget::ContainerWidget {
public:
    /**
     * @brief 命令发送回调类型
     */
    using CommandCallback = std::function<void(const String&)>;

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
    void setCommandCallback(CommandCallback callback) {
        MC_ASSERT_RELEASE(!m_commandCallback);
        m_commandCallback = std::move(callback);
    }

    /**
     * @brief 设置命令补全管理器
     */
    void setCommandManager(mc::client::command::ClientCommandManager* commandManager) {
        MC_ASSERT_RELEASE(!m_commandManager);
        m_commandManager = commandManager;
        updateCommandSuggestions();
    }

    // ========== 状态 ==========

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] bool isOpen() const { return m_open; }

    /**
     * @brief 打开聊天框
     * @param command 是否以命令模式打开（自动填入 /）
     */
    void open(bool command = false);

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
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override;

    // ========== 消息 ==========

    /**
     * @brief 添加聊天消息
     */
    void addMessage(const String& message, u32 color = 0xFFFFFFFF);

    /**
     * @brief 添加富文本消息
     */
    void addMessage(std::unique_ptr<text::ITextComponent> message,
                    chat::ChatMessageType type = chat::ChatMessageType::Chat);

    /**
     * @brief 添加系统消息
     */
    void addSystemMessage(const String& message);

    /**
     * @brief 获取聊天历史
     */
    [[nodiscard]] ChatHistory& history() { return m_history; }
    [[nodiscard]] const ChatHistory& history() const { return m_history; }

    // ========== 输入 ==========

    /**
     * @brief 获取当前输入
     */
    [[nodiscard]] const String& input() const { return m_input; }

    /**
     * @brief 设置输入文本
     */
    void setInput(const String& text);

    /**
     * @brief 清除输入
     */
    void clearInput();

private:
    // ========== 内部方法 ==========

    /**
     * @brief 插入文本到光标位置
     */
    void insertText(const String& text);

    /**
     * @brief 删除选中的文本
     */
    void deleteSelection();

    /**
     * @brief 删除光标前的字符
     */
    void deleteBeforeCursor();

    /**
     * @brief 删除光标后的字符
     */
    void deleteAfterCursor();

    /**
     * @brief 移动光标
     */
    void moveCursor(i32 offset, bool selecting = false);

    /**
     * @brief 移动光标到行首/行尾
     */
    void moveCursorToEdge(bool start, bool selecting = false);

    /**
     * @brief 获取光标位置对应的像素偏移
     */
    [[nodiscard]] f32 getCursorPixelPosition() const;

    /**
     * @brief 发送当前输入
     */
    void sendInput();

    /**
     * @brief 更新光标闪烁
     */
    void updateCursorBlink(f32 dt);

    /**
     * @brief 更新命令补全建议
     */
    void updateCommandSuggestions();

    /**
     * @brief 清空命令补全建议
     */
    void clearCommandSuggestions();

    /**
     * @brief 接受当前命令补全建议
     */
    void acceptCommandSuggestion();

    /**
     * @brief 渲染命令补全列表
     */
    void renderCommandSuggestions(kagero::widget::PaintContext& ctx);

    /**
     * @brief 判断当前输入是否为命令
     */
    [[nodiscard]] bool isCommandInput() const;

    /**
     * @brief 渲染消息列表
     */
    void renderMessages(kagero::widget::PaintContext& ctx);

    /**
     * @brief 渲染输入框
     */
    void renderInputBox(kagero::widget::PaintContext& ctx);

    // ========== 成员变量 ==========

    ChatHistory m_history;
    String m_input;
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

    // 输入框尺寸
    static constexpr f32 INPUT_BOX_HEIGHT = 20.0f;
    static constexpr f32 INPUT_BOX_PADDING = 4.0f;
    static constexpr f32 CHAT_WIDTH_RATIO = 0.4f;
    static constexpr f32 MESSAGE_FADE_START = 3.0f;

    // 命令回调
    CommandCallback m_commandCallback;
};

} // namespace mc::client::ui::minecraft::widgets
