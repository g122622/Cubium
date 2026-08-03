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

#include "Widget.hpp"
#include "common/input/KeyBinding.hpp"
#include "common/util/text/Utf8.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>

#include "client/ui/Font.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 文本输入框组件
 *
 * 支持文本输入、光标定位、文本选区、滚动、验证等功能。
 * 内部存储使用 UTF-8 编码的 std::string，所有位置/索引操作基于码点
 * （而非字节偏移），确保对 CJK、emoji 等多字节字符的正确处理。
 */
class TextFieldWidget : public Widget {
public:
    using TextChangedCallback = std::function<void(const std::string&)>;
    using TextValidator = std::function<bool(const std::string&)>;

    TextFieldWidget() = default;

    /**
     * @brief 构造函数
     *
     * @param id 组件标识符
     * @param x X 坐标
     * @param y Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    TextFieldWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 每帧更新，用于光标闪烁计时
     */
    void tick(f32 dt) override
    {
        if (!isFocused()) {
            m_cursorBlinkTimer = 0.0f;
            m_cursorVisible = true;
            return;
        }

        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= CURSOR_BLINK_RATE) {
            m_cursorBlinkTimer -= CURSOR_BLINK_RATE;
            m_cursorVisible = !m_cursorVisible;
        }
    }

    /**
     * @brief 绘制文本输入框
     */
    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;

        // 绘制背景
        if (m_drawBackground) {
            const u32 bg = isFocused() ? Colors::fromARGB(255, 30, 30, 30) : Colors::fromARGB(255, 22, 22, 22);
            ctx.drawFilledRect(bounds(), bg);
        }
        ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 120, 120, 120));

        const std::string& displayText = m_text.empty() ? m_placeholder : m_text;
        if (displayText.empty() && !hasSelection()) {
            // 无文本且无选区时仍需绘制光标
            if (isFocused() && m_cursorVisible) {
                _drawCursor(ctx, textStartX() - m_scrollOffset);
            }
            return;
        }

        const Rect textClip = getTextClipBounds();
        if (!textClip.isValid()) {
            return;
        }

        const i32 baselineY = getTextBaselineY(ctx);
        const u32 textColor = m_text.empty() ? m_disabledTextColor : (m_enabled ? m_textColor : m_disabledTextColor);

        ctx.pushClip(textClip);

        // 绘制选区高亮（在文本下方）
        if (hasSelection() && !m_text.empty()) {
            const i32 selStart = std::min(m_cursorPosition, m_selectionEnd);
            const i32 selEnd = std::max(m_cursorPosition, m_selectionEnd);
            const f32 selStartX =
                static_cast<f32>(textStartX()) + _measurePrefixWidth(selStart) - static_cast<f32>(m_scrollOffset);
            const f32 selEndX =
                static_cast<f32>(textStartX()) + _measurePrefixWidth(selEnd) - static_cast<f32>(m_scrollOffset);
            const i32 fontHeight = static_cast<i32>(ctx.getFontHeight());
            const Rect selectionRect(
                static_cast<i32>(selStartX), baselineY - 1, static_cast<i32>(selEndX - selStartX), fontHeight + 2);
            ctx.drawFilledRect(selectionRect, m_selectionColor);
        }

        // 绘制文本
        ctx.drawText(displayText, textStartX() - m_scrollOffset, baselineY, textColor);

        // 绘制光标
        if (isFocused() && m_cursorVisible && !hasSelection()) {
            const f32 cursorX = static_cast<f32>(textStartX()) + _measurePrefixWidth(m_cursorPosition) -
                static_cast<f32>(m_scrollOffset);
            _drawCursor(ctx, cursorX);
        }

        ctx.popClip();
    }

    /**
     * @brief 鼠标点击处理，点击后聚焦并定位光标
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseY;
        (void)mods;

        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false;

        setFocused(true);
        _resetCursorBlink();
        setCursorPosition(positionFromMouseX(mouseX));
        return true;
    }

    /**
     * @brief 键盘事件处理，支持退格、删除、方向键、Home/End 等操作
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override
    {
        (void)scanCode;
        if (!canWrite()) return false;
        // 只处理按下和重复事件
        if (action != static_cast<i32>(KeyAction::Press) && action != static_cast<i32>(KeyAction::Repeat)) return false;

        const bool previousShiftHeld = m_shiftHeld;
        m_shiftHeld = hasMod(static_cast<KeyMods>(mods), KeyMods::Shift);

        const auto restoreShift = [&]() { m_shiftHeld = previousShiftHeld; };

        switch (key) {
            case Keys::Backspace:
                if (m_active) {
                    deleteFromCursor(-1);
                }
                restoreShift();
                return true;
            case Keys::Delete:
                if (m_active) {
                    deleteFromCursor(1);
                }
                restoreShift();
                return true;
            case Keys::Right:
                moveCursorBy(1);
                restoreShift();
                return true;
            case Keys::Left:
                moveCursorBy(-1);
                restoreShift();
                return true;
            case Keys::Home:
                setCursorPosition(0);
                restoreShift();
                return true;
            case Keys::End:
                setCursorPositionEnd();
                restoreShift();
                return true;
            default:
                restoreShift();
                return false;
        }
    }

    /**
     * @brief 字符输入事件处理
     */
    bool onChar(u32 codePoint) override
    {
        if (!canWrite()) return false;
        if (!isAllowedCharacter(codePoint)) return false;

        writeText(util::text::utf8Encode(codePoint));
        return true;
    }

    /**
     * @brief 设置文本
     */
    void setText(const std::string& text)
    {
        if (m_validator && !m_validator(text)) return;

        std::string newText = text;
        // 按码点数截断，确保不截断多字节字符
        const size_t maxCodepoints = static_cast<size_t>(m_maxLength);
        const size_t currentCodepoints = util::text::utf8CodepointCount(newText);
        if (currentCodepoints > maxCodepoints) {
            const size_t byteOffset = util::text::utf8CodepointToByteOffset(newText, maxCodepoints);
            newText = newText.substr(0, byteOffset);
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

        // 计算被替换选区的码点数
        const i32 replacedCodepoints = selEnd - selStart;

        // 过滤输入文本中的非法字符
        std::string toWrite = filterAllowedCharacters(text);

        // 计算可用空间（以码点计）
        const i32 currentCodepoints = static_cast<i32>(util::text::utf8CodepointCount(m_text));
        const i32 availableSpace = m_maxLength - currentCodepoints + replacedCodepoints;
        if (availableSpace <= 0) return;

        // 按码点数截断 toWrite
        const size_t toWriteCodepoints = util::text::utf8CodepointCount(toWrite);
        if (static_cast<i32>(toWriteCodepoints) > availableSpace) {
            const size_t byteOffset =
                util::text::utf8CodepointToByteOffset(toWrite, static_cast<size_t>(availableSpace));
            toWrite = toWrite.substr(0, byteOffset);
        }

        // 计算字节偏移
        const size_t selStartByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(selStart));
        const size_t selEndByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(selEnd));

        std::string newText = m_text.substr(0, selStartByte) + toWrite + m_text.substr(selEndByte);
        if (m_validator && !m_validator(newText)) return;

        m_text = newText;
        const size_t writeCodepoints = util::text::utf8CodepointCount(toWrite);
        setCursorPosition(selStart + static_cast<i32>(writeCodepoints));
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
        const size_t startByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(start));
        const size_t endByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(end));
        return m_text.substr(startByte, endByte - startByte);
    }

    /**
     * @brief 删除选中的文本
     */
    void deleteSelectedText()
    {
        if (!hasSelection()) return;

        const i32 start = std::min(m_cursorPosition, m_selectionEnd);
        const i32 end = std::max(m_cursorPosition, m_selectionEnd);

        const size_t startByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(start));
        const size_t endByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(end));

        std::string newText = m_text.substr(0, startByte) + m_text.substr(endByte);
        if (m_validator && !m_validator(newText)) return;

        m_text = newText;
        setCursorPosition(start);
        setSelectionPosition(m_cursorPosition);
        onTextChanged();
    }

    /**
     * @brief 设置光标位置（码点索引）
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
    void setCursorPositionEnd() { setCursorPosition(static_cast<i32>(util::text::utf8CodepointCount(m_text))); }

    /**
     * @brief 按码点偏移量移动光标
     */
    void moveCursorBy(i32 delta) { setCursorPosition(m_cursorPosition + delta); }

    /**
     * @brief 获取光标位置（码点索引）
     */
    [[nodiscard]] i32 cursorPosition() const { return m_cursorPosition; }

    /**
     * @brief 设置选区结束位置（码点索引）
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
     * @brief 设置最大长度（码点数）
     */
    void setMaxLength(i32 maxLength)
    {
        m_maxLength = std::max(0, maxLength);
        const size_t currentCodepoints = util::text::utf8CodepointCount(m_text);
        if (static_cast<i32>(currentCodepoints) > m_maxLength) {
            const size_t byteOffset = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(m_maxLength));
            m_text = m_text.substr(0, byteOffset);
            m_cursorPosition = clampPosition(m_cursorPosition);
            m_selectionEnd = clampPosition(m_selectionEnd);
            updateScrollOffset();
            onTextChanged();
        }
    }

    /**
     * @brief 获取最大长度（码点数）
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
     * @brief 设置选区高亮颜色（ARGB 格式）
     */
    void setSelectionColor(u32 color) { m_selectionColor = color; }

    /**
     * @brief 设置字体
     *
     * @note 该指针由外部管理生命周期，TextFieldWidget 不接管所有权。
     */
    void setFont(::mc::client::Font* font)
    {
        m_font = font;
        updateScrollOffset();
    }

    /**
     * @brief 获取字体
     */
    [[nodiscard]] ::mc::client::Font* font() const { return m_font; }

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
     * @brief 文本变化后回调，通知外部监听者
     */
    virtual void onTextChanged()
    {
        if (m_onTextChanged) {
            m_onTextChanged(m_text);
        }
    }

    /**
     * @brief 从光标处删除字符（按码点移动）
     *
     * @param delta 删除方向和数量（码点），负数向左删除，正数向右删除
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
            end = m_cursorPosition;
            start = std::max(0, m_cursorPosition + delta);
        } else {
            start = m_cursorPosition;
            const i32 totalCodepoints = static_cast<i32>(util::text::utf8CodepointCount(m_text));
            end = std::min(totalCodepoints, m_cursorPosition + delta);
        }

        if (start != end) {
            const size_t startByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(start));
            const size_t endByte = util::text::utf8CodepointToByteOffset(m_text, static_cast<size_t>(end));

            std::string newText = m_text.substr(0, startByte) + m_text.substr(endByte);
            if (m_validator && !m_validator(newText)) return;

            m_text = newText;
            setCursorPosition(start);
            onTextChanged();
        }
    }

    /**
     * @brief 计算单个码点的水平步进宽度
     */
    [[nodiscard]] f32 measureGlyphAdvance(u32 codePoint) const
    {
        if (m_font) {
            if (const auto* glyph = m_font->getGlyph(codePoint); glyph != nullptr) {
                return glyph->advance;
            }
            return MISSING_GLYPH_ADVANCE;
        }
        return FALLBACK_CHAR_ADVANCE;
    }

    /**
     * @brief 计算文本宽度（按码点迭代 UTF-8）
     */
    [[nodiscard]] f32 measureTextWidth(const std::string& text) const
    {
        f32 width = 0.0f;
        util::text::utf8ForEachCodepoint(text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) {
            width += measureGlyphAdvance(codePoint);
        });
        return width;
    }

    /**
     * @brief 计算从文本开头到指定码点位置之间的宽度
     *
     * @param codepointPosition 码点索引
     */
    [[nodiscard]] f32 _measurePrefixWidth(i32 codepointPosition) const
    {
        f32 width = 0.0f;
        i32 count = 0;
        util::text::utf8ForEachCodepoint(
            m_text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) -> bool {
                if (count >= codepointPosition) {
                    return false;
                }
                width += measureGlyphAdvance(codePoint);
                ++count;
                return true;
            });
        return width;
    }

    /**
     * @brief 根据像素偏移量计算对应的码点索引
     */
    [[nodiscard]] i32 positionFromTextOffset(f32 offset) const
    {
        if (offset <= 0.0f) {
            return 0;
        }

        f32 width = 0.0f;
        i32 codepointIndex = 0;
        util::text::utf8ForEachCodepoint(
            m_text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) -> bool {
                const f32 advance = measureGlyphAdvance(codePoint);
                if (width + advance > offset) {
                    return false;
                }
                width += advance;
                ++codepointIndex;
                return true;
            });

        return codepointIndex;
    }

    /**
     * @brief 根据鼠标 X 坐标计算码点索引
     */
    [[nodiscard]] i32 positionFromMouseX(i32 mouseX) const
    {
        const i32 innerWidth = innerTextWidth();
        const i32 localX = std::clamp(mouseX - textStartX(), 0, innerWidth);
        return positionFromTextOffset(static_cast<f32>(m_scrollOffset + localX));
    }

    /**
     * @brief 获取文本起始 X 坐标（含边框和内边距偏移）
     */
    [[nodiscard]] i32 textStartX() const { return bounds().x + 1 + padding().left; }

    /**
     * @brief 获取文本基线 Y 坐标（垂直居中）
     */
    [[nodiscard]] i32 getTextBaselineY(PaintContext& ctx) const
    {
        const i32 fontHeight = static_cast<i32>(ctx.getFontHeight());
        return bounds().y + (bounds().height - fontHeight) / 2;
    }

    /**
     * @brief 获取内容区域剪裁范围（扣除边框和内边距）
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
     * @brief 获取可显示的文本宽度（扣除边框和内边距）
     */
    [[nodiscard]] i32 innerTextWidth() const { return std::max(0, bounds().width - 2 - padding().horizontal()); }

    /**
     * @brief 更新滚动偏移，确保光标始终在可见区域内
     */
    void updateScrollOffset()
    {
        const i32 visibleWidth = innerTextWidth();
        if (visibleWidth <= 0) {
            m_scrollOffset = 0;
            return;
        }

        const i32 cursorX = static_cast<i32>(std::floor(_measurePrefixWidth(m_cursorPosition)));
        const i32 maxScroll = std::max(0, static_cast<i32>(std::ceil(measureTextWidth(m_text))) - visibleWidth);

        if (cursorX < m_scrollOffset) {
            m_scrollOffset = cursorX;
        } else if (cursorX > m_scrollOffset + visibleWidth) {
            m_scrollOffset = cursorX - visibleWidth;
        }

        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxScroll);
    }

    /**
     * @brief 将码点位置限制在 [0, codepointCount] 范围内
     */
    [[nodiscard]] i32 clampPosition(i32 pos) const
    {
        const i32 totalCodepoints = static_cast<i32>(util::text::utf8CodepointCount(m_text));
        return std::max(0, std::min(pos, totalCodepoints));
    }

    /**
     * @brief 检查字符码点是否允许输入（过滤控制字符和 § 符号）
     *
     * 除控制字符外还禁止 § (0xA7) 符号（Minecraft 格式化前缀）。
     */
    static bool isAllowedCharacter(u32 codePoint)
    {
        if (codePoint < 32) return false;
        if (codePoint == 127) return false;
        if (codePoint == 167) return false; // § 符号
        return true;
    }

    /**
     * @brief 过滤字符串中不允许输入的字符（按码点过滤）
     */
    static std::string filterAllowedCharacters(const std::string& text)
    {
        std::string result;
        result.reserve(text.size());
        util::text::utf8ForEachCodepoint(text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) {
            if (isAllowedCharacter(codePoint)) {
                util::text::utf8Append(result, codePoint);
            }
        });
        return result;
    }

    // ---- 成员变量 ----

    /// 缺失字形的回退步进宽度（像素）
    static constexpr f32 MISSING_GLYPH_ADVANCE = 4.0f;
    /// 无字体时的每字符回退宽度（像素）
    static constexpr f32 FALLBACK_CHAR_ADVANCE = 8.0f;
    /// 光标闪烁周期（秒）
    static constexpr f32 CURSOR_BLINK_RATE = 0.5f;

    std::string m_text;                   ///< 当前文本内容（UTF-8 编码）
    std::string m_placeholder;            ///< 占位符文本（文本为空时显示）
    i32 m_maxLength = 32;                 ///< 最大文本长度（码点数）
    i32 m_cursorPosition = 0;             ///< 光标位置（码点索引）
    i32 m_selectionEnd = 0;               ///< 选区结束位置（码点索引）
    i32 m_scrollOffset = 0;               ///< 水平滚动偏移（像素）
    f32 m_cursorBlinkTimer = 0.0f;        ///< 光标闪烁计时器（秒）
    bool m_cursorVisible = true;          ///< 光标是否可见（闪烁状态）
    bool m_enabled = true;                ///< 是否启用输入
    bool m_canLoseFocus = true;           ///< 失去焦点时是否清除选区
    bool m_drawBackground = true;         ///< 是否绘制背景
    bool m_shiftHeld = false;             ///< Shift 键是否按下（用于选区扩展）
    u32 m_textColor = 0xE0E0E0;           ///< 正常状态文本颜色
    u32 m_disabledTextColor = 0x707070;   ///< 禁用状态文本颜色
    u32 m_selectionColor = 0x8000AAFF;    ///< 选区高亮颜色（半透明蓝色）
    ::mc::client::Font* m_font = nullptr; ///< 字体指针（外部管理生命周期）
    TextChangedCallback m_onTextChanged;  ///< 文本变化回调
    TextValidator m_validator;            ///< 文本验证器

private:
    /**
     * @brief 重置光标闪烁（输入事件后光标重新可见）
     */
    void _resetCursorBlink()
    {
        m_cursorBlinkTimer = 0.0f;
        m_cursorVisible = true;
    }

    /**
     * @brief 绘制光标线
     */
    void _drawCursor(PaintContext& ctx, f32 x) const
    {
        const i32 baselineY = getTextBaselineY(ctx);
        const i32 fontHeight = static_cast<i32>(ctx.getFontHeight());
        ctx.drawFilledRect(Rect(static_cast<i32>(x), baselineY - 1, 1, fontHeight + 2), 0xFFFFFFFF);
    }
};

} // namespace mc::client::ui::kagero::widget
