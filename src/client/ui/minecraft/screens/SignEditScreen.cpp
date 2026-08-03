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

#include "SignEditScreen.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace mc::client::ui::minecraft {

SignEditScreen::SignEditScreen(const BlockPos& pos,
    const std::array<std::string, LINE_COUNT>& initialLines,
    bool isFrontSide,
    SubmitCallback submitCallback,
    CloseCallback closeCallback)
    : Screen("sign_edit_screen")
    , m_pos(pos)
    , m_initialLines(initialLines)
    , m_isFrontSide(isFrontSide)
    , m_submitCallback(std::move(submitCallback))
    , m_closeCallback(std::move(closeCallback))
{
    setPauseScreen(false);
}

void SignEditScreen::onOpen()
{
    _initTextFields();
}

void SignEditScreen::onClose()
{
    // 屏幕关闭时无需额外清理
}

void SignEditScreen::tick(f32 dt)
{
    // 更新所有子组件的 tick（光标闪烁等）
    for (auto& child : m_children) {
        child->tick(dt);
    }
}

void SignEditScreen::paint(kagero::widget::PaintContext& ctx)
{
    // 绘制半透明背景遮罩
    const auto& b = bounds();
    ctx.drawFilledRect(b, Colors::fromARGB(200, 0, 0, 0));

    // 绘制编辑面板背景
    const i32 panelX = SCREEN_PADDING;
    const i32 panelY = SCREEN_PADDING;
    const i32 panelWidth = FIELD_WIDTH + 2 * SCREEN_PADDING;
    const i32 totalFieldsHeight = LINE_COUNT * FIELD_HEIGHT + (LINE_COUNT - 1) * FIELD_SPACING;
    const i32 panelHeight = TITLE_HEIGHT + totalFieldsHeight + SCREEN_PADDING;
    ctx.drawFilledRect(kagero::Rect(panelX, panelY, panelWidth, panelHeight), Colors::fromARGB(255, 40, 40, 40));
    ctx.drawBorder(kagero::Rect(panelX, panelY, panelWidth, panelHeight), 1.0f, Colors::fromARGB(255, 120, 120, 120));

    // 绘制标题
    ctx.drawText("Edit Sign Message", panelX + 4, panelY + 4, Colors::fromARGB(255, 255, 255, 255));

    // 绘制所有子组件
    ContainerWidget::paint(ctx);
}

void SignEditScreen::_initTextFields()
{
    // 计算输入框起始位置（居中布局）
    const i32 totalHeight = LINE_COUNT * FIELD_HEIGHT + (LINE_COUNT - 1) * FIELD_SPACING;
    const i32 startY = SCREEN_PADDING + TITLE_HEIGHT;
    const i32 startX = SCREEN_PADDING;

    for (i32 i = 0; i < LINE_COUNT; ++i) {
        auto field = std::make_unique<kagero::widget::TextFieldWidget>("sign_line_" + std::to_string(i),
            startX,
            startY + i * (FIELD_HEIGHT + FIELD_SPACING),
            FIELD_WIDTH,
            FIELD_HEIGHT);

        field->setMaxLength(MAX_LINE_LENGTH);
        field->setText(m_initialLines[static_cast<std::size_t>(i)]);
        field->setDrawBackground(true);
        field->setVisible(true);
        field->setActive(true);

        m_textFields[static_cast<std::size_t>(i)] = field.get();
        addChild(std::move(field));
    }

    // 默认聚焦第一行
    if (m_textFields[0] != nullptr) {
        m_textFields[0]->setFocused(true);
        m_currentLine = 0;
    }
}

bool SignEditScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    // 只处理按下和重复事件
    if (action != 1 && action != 2) { // Press=1, Repeat=2
        return false;
    }

    // ESC 取消编辑
    if (key == 256) { // GLFW_KEY_ESCAPE
        _cancel();
        return true;
    }

    // Enter 提交编辑
    if (key == 257) { // GLFW_KEY_ENTER
        _submit();
        return true;
    }

    // Tab 切换到下一行
    if (key == 258) { // GLFW_KEY_TAB
        // 取消当前行焦点
        if (m_textFields[static_cast<std::size_t>(m_currentLine)] != nullptr) {
            m_textFields[static_cast<std::size_t>(m_currentLine)]->setFocused(false);
        }

        // Shift+Tab 向上切换，普通 Tab 向下切换
        if (mods & 1) { // GLFW_MOD_SHIFT
            m_currentLine = (m_currentLine - 1 + LINE_COUNT) % LINE_COUNT;
        } else {
            m_currentLine = (m_currentLine + 1) % LINE_COUNT;
        }

        if (m_textFields[static_cast<std::size_t>(m_currentLine)] != nullptr) {
            m_textFields[static_cast<std::size_t>(m_currentLine)]->setFocused(true);
            m_textFields[static_cast<std::size_t>(m_currentLine)]->setCursorPositionEnd();
        }
        return true;
    }

    // 其他按键交给当前聚焦的 TextFieldWidget 处理
    if (m_textFields[static_cast<std::size_t>(m_currentLine)] != nullptr) {
        if (m_textFields[static_cast<std::size_t>(m_currentLine)]->onKey(key, scanCode, action, mods)) {
            return true;
        }
    }

    return Screen::onKey(key, scanCode, action, mods);
}

void SignEditScreen::_submit()
{
    // 收集4行文本
    std::array<std::string, LINE_COUNT> lines;
    for (i32 i = 0; i < LINE_COUNT; ++i) {
        if (m_textFields[static_cast<std::size_t>(i)] != nullptr) {
            lines[static_cast<std::size_t>(i)] = m_textFields[static_cast<std::size_t>(i)]->text();
        }
    }

    // 调用提交回调（发送 UpdateSignPacket 给服务端）
    if (m_submitCallback) {
        m_submitCallback(m_pos, lines, m_isFrontSide);
    }

    // 关闭屏幕
    if (m_closeCallback) {
        m_closeCallback();
    }
}

void SignEditScreen::_cancel()
{
    // 取消编辑，不提交，直接关闭
    if (m_closeCallback) {
        m_closeCallback();
    }
}

} // namespace mc::client::ui::minecraft
