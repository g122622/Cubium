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
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 文本对齐方式
 */
enum class TextAlignment : u8 { Left, Center, Right };

/**
 * @brief 文本组件
 *
 * 负责显示静态文本，并提供宽度、行数和取行能力。
 */
class TextWidget : public Widget {
public:
    TextWidget() = default;

    /**
     * @brief 构造函数
     */
    TextWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 构造函数（带文本）
     */
    TextWidget(std::string id, i32 x, i32 y, i32 width, i32 height, std::string text)
        : Widget(std::move(id))
        , m_text(std::move(text))
    {
        setBounds(Rect(x, y, width, height));
    }

    void paint(PaintContext& ctx) override
    {
        if (!isVisible() || m_text.empty()) return;
        ctx.drawTextCentered(m_text, bounds(), m_color);
    }

    /**
     * @brief 设置文本
     */
    void setText(const std::string& text)
    {
        if (m_text != text) {
            m_text = text;
            m_linesDirty = true;
        }
    }

    /**
     * @brief 获取文本
     */
    [[nodiscard]] const std::string& text() const { return m_text; }

    /**
     * @brief 设置文本颜色
     */
    void setColor(u32 color) { m_color = color; }

    /**
     * @brief 获取文本颜色
     */
    [[nodiscard]] u32 color() const { return m_color; }

    /**
     * @brief 设置阴影
     */
    void setShadow(bool shadow) { m_shadow = shadow; }

    /**
     * @brief 是否有阴影
     */
    [[nodiscard]] bool hasShadow() const { return m_shadow; }

    /**
     * @brief 设置阴影颜色
     */
    void setShadowColor(u32 color) { m_shadowColor = color; }

    /**
     * @brief 获取阴影颜色
     */
    [[nodiscard]] u32 shadowColor() const { return m_shadowColor; }

    /**
     * @brief 设置对齐方式
     */
    void setAlignment(TextAlignment alignment) { m_alignment = alignment; }

    /**
     * @brief 获取对齐方式
     */
    [[nodiscard]] TextAlignment alignment() const { return m_alignment; }

    /**
     * @brief 设置最大行数，0 表示不限
     */
    void setMaxLines(i32 maxLines)
    {
        m_maxLines = maxLines;
        m_linesDirty = true;
    }

    /**
     * @brief 获取最大行数
     */
    [[nodiscard]] i32 maxLines() const { return m_maxLines; }

    /**
     * @brief 设置是否启用自动换行
     */
    void setWordWrap(bool wrap)
    {
        m_wordWrap = wrap;
        m_linesDirty = true;
    }

    /**
     * @brief 是否启用自动换行
     */
    [[nodiscard]] bool wordWrap() const { return m_wordWrap; }

    /**
     * @brief 设置行高
     */
    void setLineHeight(i32 lineHeight) { m_lineHeight = lineHeight; }

    /**
     * @brief 获取行高
     */
    [[nodiscard]] i32 lineHeight() const { return m_lineHeight; }

    /**
     * @brief 设置缩放
     */
    void setScale(f32 scale)
    {
        m_scale = scale;
        m_linesDirty = true;
    }

    /**
     * @brief 获取缩放
     */
    [[nodiscard]] f32 scale() const { return m_scale; }

    /**
     * @brief 获取文本宽度
     */
    [[nodiscard]] f32 getTextWidth() const
    {
        const auto& lines = ensureLines();
        f32 maxWidth = 0.0f;
        for (const auto& line : lines) {
            maxWidth = std::max(maxWidth, measureLineWidth(line));
        }
        return maxWidth * m_scale;
    }

    /**
     * @brief 获取文本高度
     */
    [[nodiscard]] f32 getTextHeight() const
    {
        const i32 lineCount = getLineCount();
        if (lineCount <= 0) {
            return 0.0f;
        }

        const i32 visibleLines = m_maxLines > 0 ? std::min(lineCount, m_maxLines) : lineCount;
        return static_cast<f32>(visibleLines * m_lineHeight) * m_scale;
    }

    /**
     * @brief 获取行数
     */
    [[nodiscard]] i32 getLineCount() const { return static_cast<i32>(ensureLines().size()); }

    /**
     * @brief 获取指定行的文本
     */
    [[nodiscard]] std::string getLine(i32 lineIndex) const
    {
        const auto& lines = ensureLines();
        if (lineIndex < 0 || static_cast<size_t>(lineIndex) >= lines.size()) {
            return {};
        }
        return lines[static_cast<size_t>(lineIndex)];
    }

    /**
     * @brief 设置字体
     */
    void setFont(void* font)
    {
        if (m_font != font) {
            m_font = font;
            m_linesDirty = true;
        }
    }

    /**
     * @brief 获取字体
     */
    [[nodiscard]] void* font() const { return m_font; }

protected:
    /**
     * @brief 尺寸变化后使行缓存失效
     */
    void onSizeChanged() override { m_linesDirty = true; }

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
     * @brief 计算单行文本宽度
     */
    [[nodiscard]] f32 measureLineWidth(const std::string& text) const
    {
        f32 width = 0.0f;
        for (char32_t codePoint : text) {
            width += measureGlyphAdvance(codePoint);
        }
        return width;
    }

    /**
     * @brief 追加一行到缓存
     */
    void appendLine(std::string line) const { m_lines.emplace_back(std::move(line)); }

    /**
     * @brief 将当前文本按可用宽度构建成行缓存
     */
    [[nodiscard]] const std::vector<std::string>& ensureLines() const
    {
        if (!m_linesDirty) {
            return m_lines;
        }

        m_lines.clear();
        m_linesDirty = false;

        if (m_text.empty()) {
            return m_lines;
        }

        if (!m_wordWrap) {
            appendExplicitLines();
            return m_lines;
        }

        const i32 wrapWidth = getWrapWidth();
        if (wrapWidth <= 0) {
            appendExplicitLines();
            return m_lines;
        }

        std::string currentLine;
        currentLine.reserve(m_text.size());
        f32 currentWidth = 0.0f;

        for (char32_t codePoint : m_text) {
            if (codePoint == U'\n') {
                appendLine(std::move(currentLine));
                currentLine.clear();
                currentWidth = 0.0f;
                continue;
            }

            const f32 advance = measureGlyphAdvance(codePoint);
            if (codePoint == U' ' && currentLine.empty()) {
                continue;
            }

            if (currentWidth > 0.0f && currentWidth + advance > static_cast<f32>(wrapWidth)) {
                appendLine(std::move(currentLine));
                currentLine.clear();
                currentWidth = 0.0f;
                if (codePoint == U' ') {
                    continue;
                }
            }

            currentLine.push_back(codePoint);
            currentWidth += advance;
        }

        appendLine(std::move(currentLine));
        return m_lines;
    }

    /**
     * @brief 将文本按显式换行符拆分
     */
    void appendExplicitLines() const
    {
        size_t start = 0;
        while (start <= m_text.size()) {
            const size_t end = m_text.find(U'\n', start);
            appendLine(m_text.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) {
                break;
            }

            start = end + 1;
            if (start == m_text.size()) {
                appendLine({});
                break;
            }
        }
    }

    /**
     * @brief 获取当前可换行宽度
     */
    [[nodiscard]] i32 getWrapWidth() const { return std::max(0, bounds().width - padding().horizontal()); }

    std::string m_text;
    u32 m_color = Colors::WHITE;
    bool m_shadow = true;
    u32 m_shadowColor = Colors::MC_DARK_GRAY;
    TextAlignment m_alignment = TextAlignment::Left;
    i32 m_maxLines = 0;
    bool m_wordWrap = false;
    i32 m_lineHeight = 9;
    f32 m_scale = 1.0f;
    void* m_font = nullptr;
    mutable std::vector<std::string> m_lines;
    mutable bool m_linesDirty = true;
};

} // namespace mc::client::ui::kagero::widget
