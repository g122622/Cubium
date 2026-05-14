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

#include "../../Font.hpp"
#include "../../Glyph.hpp"
#include "../paint/PaintContext.hpp"
#include "Widget.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 文本输入框组件
 */
class TextFieldWidget : public Widget {
public:
    using TextChangedCallback = std::function<void(const std::string&)>;
    using TextValidator = std::function<bool(const std::string&)>;

    TextFieldWidget() = default;

    /**
     * @brief 构造函数
     */
    TextFieldWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, width, height));
    }

    void tick(f32 dt) override
    {
        (void)dt;
        ++m_cursorBlinkCounter;
    }

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;

        if (m_drawBackground) {
            const u32 bg = isFocused() ? Colors::fromARGB(255, 30, 30, 30) : Colors::fromARGB(255, 22, 22, 22);
            ctx.drawFilledRect(bounds(), bg);
        }
        ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 120, 120, 120));

        const std::string& displayText = m_text.empty() ? m_placeholder : m_text;
        if (displayText.empty()) {
            return;
        }

        const Rect textClip = getTextClipBounds();
        if (!textClip.isValid()) {
            return;
        }

        const i32 baselineY = getTextBaselineY(ctx);
        const u32 textColor = m_text.empty() ? m_disabledTextColor : (m_enabled ? m_textColor : m_disabledTextColor);

        ctx.pushClip(textClip);
        ctx.drawText(displayText, textStartX() - m_scrollOffset, baselineY, textColor);
        ctx.popClip();
    }

    /**
     * @brief 鼠标点击处理
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override
    {
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false;

        setFocused(true);
        setCursorPosition(positionFromMouseX(mouseX));
        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override
    {
        (void)scanCode;
        if (!canWrite()) return false;
        if (action != 1 && action != 2) return false;

        const bool previousShiftHeld = m_shiftHeld;
        m_shiftHeld = (mods & 0x0001) != 0;

        const auto restoreShift = [&]() { m_shiftHeld = previousShiftHeld; };

        switch (key) {
            case 259:
                if (m_active) {
                    deleteFromCursor(-1);
                }
                restoreShift();
                return true;
            case 261:
                if (m_active) {
                    deleteFromCursor(1);
                }
                restoreShift();
                return true;
            case 262:
                moveCursorBy(1);
                restoreShift();
                return true;
            case 263:
                moveCursorBy(-1);
                restoreShift();
                return true;
            case 268:
                setCursorPosition(0);
                restoreShift();
                return true;
            case 269:
                setCursorPosition(static_cast<i32>(m_text.size()));
                restoreShift();
                return true;
            default:
                restoreShift();
                return false;
        }
    }

    bool onChar(u32 codePoint) override
    {
        if (!canWrite()) return false;
        if (!isAllowedCharacter(codePoint)) return false;

        writeText(codePointToString(codePoint));
        return true;
    }

    /**
     * @brief 设置文本
     */
    void setText(const std::string& text)
    {
        if (m_validator && !m_validator(text)) return;

        std::string newText = text;
        if (static_cast<i32>(newText.size()) > m_maxLength) {
            newText = newText.substr(0, m_maxLength);
        }

        if (m_text != newText) {
            m_text = newText;
            setCursorPositionEnd();
            setSelectionPosition(m_cursorPosition);
            onTextChanged();
        }
    }

    /**
     * @brief 获取文本
     */
    [[nodiscard]] const std::string& text() const { return m_text; }

    /**
     * @brief 在光标处写入文本，或替换当前选区
     */
    void writeText(const std::string& text)
    {
        if (text.empty() && !hasSelection()) return;
        if (!m_active) return;

        const i32 selStart = std::min(m_cursorPosition, m_selectionEnd);
        const i32 selEnd = std::max(m_cursorPosition, m_selectionEnd);

        // 替换选区会释放可写空间，因此这里要把被替换的字符数加回去。
        const i32 availableSpace = m_maxLength - static_cast<i32>(m_text.size()) + (selEnd - selStart);
        std::string toWrite = filterAllowedCharacters(text);
        if (static_cast<i32>(toWrite.size()) > availableSpace) {
            toWrite = toWrite.substr(0, std::max(0, availableSpace));
        }

        std::string newText = m_text.substr(0, selStart) + toWrite + m_text.substr(selEnd);
        if (m_validator && !m_validator(newText)) return;

        m_text = newText;
        setCursorPosition(selStart + static_cast<i32>(toWrite.size()));
        setSelectionPosition(m_cursorPosition);
        onTextChanged();
    }

    /**
     * @brief 获取选中的文本
     */
    [[nodiscard]] std::string getSelectedText() const
    {
        const i32 start = std::min(m_cursorPosition, m_selectionEnd);
        const i32 end = std::max(m_cursorPosition, m_selectionEnd);
        return m_text.substr(start, end - start);
    }

    /**
     * @brief 删除选中的文本
     */
    void deleteSelectedText()
    {
        if (!hasSelection()) return;

        const i32 start = std::min(m_cursorPosition, m_selectionEnd);
        const i32 end = std::max(m_cursorPosition, m_selectionEnd);

        std::string newText = m_text.substr(0, start) + m_text.substr(end);
        if (m_validator && !m_validator(newText)) return;

        m_text = newText;
        setCursorPosition(start);
        setSelectionPosition(m_cursorPosition);
        onTextChanged();
    }

    /**
     * @brief 设置光标位置
     */
    void setCursorPosition(i32 position)
    {
        m_cursorPosition = clampPosition(position);
        if (!m_shiftHeld) {
            m_selectionEnd = m_cursorPosition;
        }
        updateScrollOffset();
    }

    /**
     * @brief 将光标移到开头
     */
    void setCursorPositionStart() { setCursorPosition(0); }

    /**
     * @brief 将光标移到结尾
     */
    void setCursorPositionEnd() { setCursorPosition(static_cast<i32>(m_text.size())); }

    /**
     * @brief 按偏移量移动光标
     */
    void moveCursorBy(i32 delta) { setCursorPosition(m_cursorPosition + delta); }

    /**
     * @brief 获取光标位置
     */
    [[nodiscard]] i32 cursorPosition() const { return m_cursorPosition; }

    /**
     * @brief 设置选区结束位置
     */
    void setSelectionPosition(i32 position)
    {
        m_selectionEnd = clampPosition(position);
        updateScrollOffset();
    }

    /**
     * @brief 全选文本
     */
    void selectAll()
    {
        setCursorPositionEnd();
        m_selectionEnd = 0;
    }

    /**
     * @brief 清除选区
     */
    void clearSelection() { m_selectionEnd = m_cursorPosition; }

    /**
     * @brief 是否存在选区
     */
    [[nodiscard]] bool hasSelection() const { return m_cursorPosition != m_selectionEnd; }

    /**
     * @brief 设置最大长度
     */
    void setMaxLength(i32 maxLength)
    {
        m_maxLength = std::max(0, maxLength);
        if (static_cast<i32>(m_text.size()) > m_maxLength) {
            m_text = m_text.substr(0, m_maxLength);
            m_cursorPosition = clampPosition(m_cursorPosition);
            m_selectionEnd = clampPosition(m_selectionEnd);
            updateScrollOffset();
            onTextChanged();
        }
    }

    /**
     * @brief 获取最大长度
     */
    [[nodiscard]] i32 maxLength() const { return m_maxLength; }

    /**
     * @brief 设置占位符文本
     */
    void setPlaceholder(const std::string& placeholder) { m_placeholder = placeholder; }

    /**
     * @brief 获取占位符文本
     */
    [[nodiscard]] const std::string& placeholder() const { return m_placeholder; }

    /**
     * @brief 设置文本变化回调
     */
    void setTextChangedCallback(TextChangedCallback callback) { m_onTextChanged = std::move(callback); }

    /**
     * @brief 设置文本验证器
     */
    void setValidator(TextValidator validator) { m_validator = std::move(validator); }

    /**
     * @brief 设置是否启用
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 是否启用
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * @brief 设置是否允许失去焦点时保留选区
     */
    void setCanLoseFocus(bool canLoseFocus) { m_canLoseFocus = canLoseFocus; }

    /**
     * @brief 是否允许失去焦点时保留选区
     */
    [[nodiscard]] bool canLoseFocus() const { return m_canLoseFocus; }

    /**
     * @brief 设置是否绘制背景
     */
    void setDrawBackground(bool draw) { m_drawBackground = draw; }

    /**
     * @brief 是否绘制背景
     */
    [[nodiscard]] bool drawBackground() const { return m_drawBackground; }

    /**
     * @brief 设置文本颜色
     */
    void setTextColor(u32 color) { m_textColor = color; }

    /**
     * @brief 获取文本颜色
     */
    [[nodiscard]] u32 textColor() const { return m_textColor; }

    /**
     * @brief 设置禁用状态下的文本颜色
     */
    void setDisabledTextColor(u32 color) { m_disabledTextColor = color; }

    /**
     * @brief 获取禁用状态下的文本颜色
     */
    [[nodiscard]] u32 disabledTextColor() const { return m_disabledTextColor; }

    /**
     * @brief 设置字体
     *
     * @note 该指针由外部管理生命周期，TextFieldWidget 不接管所有权。
     */
    void setFont(void* font)
    {
        m_font = font;
        updateScrollOffset();
    }

    /**
     * @brief 获取字体
     */
    [[nodiscard]] void* font() const { return m_font; }

    /**
     * @brief 是否允许写入文本
     */
    [[nodiscard]] bool canWrite() const { return isVisible() && isFocused() && m_enabled; }

protected:
    /**
     * @brief 失去焦点时清除选区
     */
    void onFocusLost() override
    {
        if (m_canLoseFocus) {
            m_selectionEnd = m_cursorPosition;
        }
    }

    /**
     * @brief 文本变化后回调
     */
    virtual void onTextChanged()
    {
        if (m_onTextChanged) {
            m_onTextChanged(m_text);
        }
    }

    /**
     * @brief 从光标处删除字符
     */
    void deleteFromCursor(i32 delta)
    {
        if (hasSelection()) {
            deleteSelectedText();
            return;
        }

        if (m_text.empty()) return;

        i32 start;
        i32 end;

        if (delta < 0) {
            start = std::max(0, m_cursorPosition + delta);
            end = m_cursorPosition;
        } else {
            start = m_cursorPosition;
            end = std::min(static_cast<i32>(m_text.size()), m_cursorPosition + delta);
        }

        if (start != end) {
            std::string newText = m_text.substr(0, start) + m_text.substr(end);
            if (m_validator && !m_validator(newText)) return;

            m_text = newText;
            setCursorPosition(start);
            onTextChanged();
        }
    }

    /**
     * @brief 获取外部字体对象
     */
    [[nodiscard]] ::mc::client::Font* resolvedFont() const { return static_cast<::mc::client::Font*>(m_font); }

    /**
     * @brief 计算单个码点的宽度
     */
    [[nodiscard]] f32 measureGlyphAdvance(char32_t codePoint) const
    {
        if (auto* font = resolvedFont()) {
            if (const auto* glyph = font->getGlyph(static_cast<u32>(codePoint)); glyph != nullptr) {
                return glyph->advance;
            }
            return 4.0f;
        }
        return 8.0f;
    }

    /**
     * @brief 计算文本宽度
     */
    [[nodiscard]] f32 measureTextWidth(const std::string& text) const
    {
        f32 width = 0.0f;
        for (char32_t codePoint : text) {
            width += measureGlyphAdvance(codePoint);
        }
        return width;
    }

    /**
     * @brief 计算指定位置之前的文本宽度
     */
    [[nodiscard]] f32 measurePrefixWidth(i32 position) const
    {
        const i32 clamped = clampPosition(position);
        f32 width = 0.0f;
        for (i32 index = 0; index < clamped; ++index) {
            width += measureGlyphAdvance(m_text[static_cast<size_t>(index)]);
        }
        return width;
    }

    /**
     * @brief 根据像素偏移量计算光标位置
     */
    [[nodiscard]] i32 positionFromTextOffset(f32 offset) const
    {
        if (offset <= 0.0f) {
            return 0;
        }

        f32 width = 0.0f;
        const i32 textLength = static_cast<i32>(m_text.size());
        for (i32 index = 0; index < textLength; ++index) {
            const f32 advance = measureGlyphAdvance(m_text[static_cast<size_t>(index)]);
            if (width + advance > offset) {
                return index;
            }
            width += advance;
        }

        return textLength;
    }

    /**
     * @brief 根据鼠标位置计算光标位置
     */
    [[nodiscard]] i32 positionFromMouseX(i32 mouseX) const
    {
        const i32 innerWidth = innerTextWidth();
        const i32 localX = std::clamp(mouseX - textStartX(), 0, innerWidth);
        return positionFromTextOffset(static_cast<f32>(m_scrollOffset + localX));
    }

    /**
     * @brief 获取文本起始 X 坐标
     */
    [[nodiscard]] i32 textStartX() const { return bounds().x + 1 + padding().left; }

    /**
     * @brief 获取文本基线 Y 坐标
     */
    [[nodiscard]] i32 getTextBaselineY(PaintContext& ctx) const
    {
        const i32 fontHeight = static_cast<i32>(ctx.getFontHeight());
        return bounds().y + (bounds().height - fontHeight) / 2;
    }

    /**
     * @brief 获取内容区域剪裁范围
     */
    [[nodiscard]] Rect getTextClipBounds() const
    {
        const i32 left = bounds().x + 1 + padding().left;
        const i32 top = bounds().y + 1 + padding().top;
        const i32 width = std::max(0, bounds().width - 2 - padding().horizontal());
        const i32 height = std::max(0, bounds().height - 2 - padding().vertical());
        return Rect(left, top, width, height);
    }

    /**
     * @brief 获取可显示的文本宽度
     */
    [[nodiscard]] i32 innerTextWidth() const { return std::max(0, bounds().width - 2 - padding().horizontal()); }

    /**
     * @brief 更新滚动偏移，确保光标始终可见
     */
    void updateScrollOffset()
    {
        const i32 visibleWidth = innerTextWidth();
        if (visibleWidth <= 0) {
            m_scrollOffset = 0;
            return;
        }

        const i32 cursorX = static_cast<i32>(std::floor(measurePrefixWidth(m_cursorPosition)));
        const i32 maxScroll = std::max(0, static_cast<i32>(std::ceil(measureTextWidth(m_text))) - visibleWidth);

        if (cursorX < m_scrollOffset) {
            m_scrollOffset = cursorX;
        } else if (cursorX > m_scrollOffset + visibleWidth) {
            m_scrollOffset = cursorX - visibleWidth;
        }

        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxScroll);
    }

    /**
     * @brief 限制光标位置在有效范围内
     */
    [[nodiscard]] i32 clampPosition(i32 pos) const
    {
        return std::max(0, std::min(pos, static_cast<i32>(m_text.size())));
    }

    /**
     * @brief 检查字符是否允许输入
     */
    static bool isAllowedCharacter(u32 codePoint)
    {
        if (codePoint < 32) return false;
        if (codePoint == 127) return false;
        return true;
    }

    /**
     * @brief 过滤允许输入的字符
     */
    static std::string filterAllowedCharacters(const std::string& text)
    {
        std::string result;
        result.reserve(text.size());
        for (char32_t codePoint : text) {
            if (isAllowedCharacter(static_cast<u32>(codePoint))) {
                result.push_back(codePoint);
            }
        }
        return result;
    }

    /**
     * @brief 将码点转为字符串
     */
    static std::string codePointToString(u32 codePoint)
    {
        std::string result;
        result.push_back(static_cast<char32_t>(codePoint));
        return result;
    }

    std::string m_text;
    std::string m_placeholder;
    i32 m_maxLength = 32;
    i32 m_cursorPosition = 0;
    i32 m_selectionEnd = 0;
    i32 m_scrollOffset = 0;
    i32 m_cursorBlinkCounter = 0;
    bool m_enabled = true;
    bool m_canLoseFocus = true;
    bool m_drawBackground = true;
    bool m_shiftHeld = false;
    u32 m_textColor = 0xE0E0E0;
    u32 m_disabledTextColor = 0x707070;
    u32 m_selectionColor = 0xFF0000FF;
    void* m_font = nullptr;
    TextChangedCallback m_onTextChanged;
    TextValidator m_validator;
};

} // namespace mc::client::ui::kagero::widget
