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
#include "client/ui/Font.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/Utf8.hpp"
#include <algorithm>
#include <cstddef>
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
 * 支持自动换行、最大行数限制和文本缩放。
 */
class TextWidget : public Widget {
public:
    TextWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    TextWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 构造函数（带文本）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param text 初始文本内容
     */
    TextWidget(std::string id, i32 x, i32 y, i32 width, i32 height, std::string text)
        : Widget(std::move(id))
        , m_text(std::move(text))
    {
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 绘制文本内容
     * @param ctx 绘图上下文
     */
    void paint(PaintContext& ctx) override
    {
        if (!isVisible() || m_text.empty()) return;

        switch (m_alignment) {
            case TextAlignment::Left:
                ctx.drawText(m_text, bounds().x, bounds().y, m_color);
                break;
            case TextAlignment::Center:
                ctx.drawTextCentered(m_text, bounds(), m_color);
                break;
            case TextAlignment::Right: {
                f32 textWidth = ctx.canvas().getTextWidth(m_text);
                i32 x = bounds().x + bounds().width - static_cast<i32>(textWidth);
                ctx.drawText(m_text, x, bounds().y, m_color);
                break;
            }
        }
    }

    /**
     * @brief 设置文本内容，仅在文本变化时标记缓存脏位
     */
    void setText(const std::string& text)
    {
        if (m_text != text) {
            m_text = text;
            m_linesDirty = true;
        }
    }

    /**
     * @brief 获取文本内容
     */
    [[nodiscard]] const std::string& text() const { return m_text; }

    /**
     * @brief 设置文本颜色（ARGB格式）
     */
    void setColor(u32 color) { m_color = color; }

    /**
     * @brief 获取文本颜色
     */
    [[nodiscard]] u32 color() const { return m_color; }

    /**
     * @brief 设置是否绘制文本阴影
     */
    void setShadow(bool shadow) { m_shadow = shadow; }

    /**
     * @brief 是否绘制文本阴影
     */
    [[nodiscard]] bool hasShadow() const { return m_shadow; }

    /**
     * @brief 设置阴影颜色（ARGB格式）
     */
    void setShadowColor(u32 color) { m_shadowColor = color; }

    /**
     * @brief 获取阴影颜色
     */
    [[nodiscard]] u32 shadowColor() const { return m_shadowColor; }

    /**
     * @brief 设置文本对齐方式
     */
    void setAlignment(TextAlignment alignment) { m_alignment = alignment; }

    /**
     * @brief 获取文本对齐方式
     */
    [[nodiscard]] TextAlignment alignment() const { return m_alignment; }

    /**
     * @brief 设置最大行数
     * @param maxLines 最大行数，0表示不限
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
     * @brief 设置行高（像素）
     */
    void setLineHeight(i32 lineHeight) { m_lineHeight = lineHeight; }

    /**
     * @brief 获取行高
     */
    [[nodiscard]] i32 lineHeight() const { return m_lineHeight; }

    /**
     * @brief 设置文本缩放比例
     */
    void setScale(f32 scale)
    {
        m_scale = scale;
        m_linesDirty = true;
    }

    /**
     * @brief 获取文本缩放比例
     */
    [[nodiscard]] f32 scale() const { return m_scale; }

    /**
     * @brief 获取文本渲染宽度（考虑缩放）
     */
    [[nodiscard]] f32 getTextWidth() const
    {
        const auto& lines = _ensureLines();
        f32 maxWidth = 0.0f;
        for (const auto& line : lines) {
            maxWidth = std::max(maxWidth, _measureLineWidth(line));
        }
        return maxWidth * m_scale;
    }

    /**
     * @brief 获取文本渲染高度（考虑缩放和最大行数限制）
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
     * @brief 获取当前文本行数
     */
    [[nodiscard]] i32 getLineCount() const { return static_cast<i32>(_ensureLines().size()); }

    /**
     * @brief 获取指定行的文本内容
     * @param lineIndex 行索引（从0开始）
     * @return 行文本内容，索引越界时返回空字符串
     */
    [[nodiscard]] std::string getLine(i32 lineIndex) const
    {
        const auto& lines = _ensureLines();
        if (lineIndex < 0 || static_cast<size_t>(lineIndex) >= lines.size()) {
            return {};
        }
        return lines[static_cast<size_t>(lineIndex)];
    }

    /**
     * @brief 设置字体
     * @param font 字体对象指针（外部管理生命周期）
     */
    void setFont(::mc::client::Font* font)
    {
        if (m_font != font) {
            m_font = font;
            m_linesDirty = true;
        }
    }

    /**
     * @brief 获取字体指针
     */
    [[nodiscard]] ::mc::client::Font* font() const { return m_font; }

protected:
    /**
     * @brief 尺寸变化后使行缓存失效
     */
    void onSizeChanged() override { m_linesDirty = true; }

    // 成员变量
    std::string m_text;                              ///< 文本内容
    u32 m_color = Colors::WHITE;                     ///< 文本颜色（ARGB）
    bool m_shadow = true;                            ///< 是否绘制阴影
    u32 m_shadowColor = Colors::MC_DARK_GRAY;        ///< 阴影颜色（ARGB）
    TextAlignment m_alignment = TextAlignment::Left; ///< 对齐方式
    i32 m_maxLines = 0;                              ///< 最大行数，0表示不限
    bool m_wordWrap = false;                         ///< 是否启用自动换行
    i32 m_lineHeight = DEFAULT_LINE_HEIGHT;          ///< 行高（像素）
    f32 m_scale = 1.0f;                              ///< 文本缩放比例
    ::mc::client::Font* m_font = nullptr;            ///< 字体指针（外部管理生命周期）
    mutable std::vector<std::string> m_lines;        ///< 行缓存（mutable支持延迟计算）
    mutable bool m_linesDirty = true;                ///< 行缓存是否需要重建

private:
    /// 缺失字形时的回退宽度（像素）
    static constexpr f32 MISSING_GLYPH_ADVANCE = 4.0f;
    /// 无字体时的每字符回退宽度（像素）
    static constexpr f32 FALLBACK_CHAR_ADVANCE = 8.0f;
    /// 默认行高（像素）
    static constexpr i32 DEFAULT_LINE_HEIGHT = 9;

    /**
     * @brief 计算单个码点的水平步进宽度
     * @param codePoint Unicode码点
     * @return 步进宽度（像素）
     */
    [[nodiscard]] f32 _measureGlyphAdvance(u32 codePoint) const
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
     * @brief 计算单行文本的总宽度（按码点迭代 UTF-8）
     * @param text 文本内容
     * @return 文本宽度（像素）
     */
    [[nodiscard]] f32 _measureLineWidth(const std::string& text) const
    {
        f32 width = 0.0f;
        util::text::utf8ForEachCodepoint(text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) {
            width += _measureGlyphAdvance(codePoint);
        });
        return width;
    }

    /**
     * @brief 追加一行到缓存
     */
    void _appendLine(std::string line) const { m_lines.emplace_back(std::move(line)); }

    /**
     * @brief 将当前文本按可用宽度构建成行缓存
     *
     * 采用延迟计算策略：仅在行缓存标记为脏时才重新构建。
     * 支持显式换行符('\n')拆分和基于宽度的自动换行。
     */
    [[nodiscard]] const std::vector<std::string>& _ensureLines() const
    {
        if (!m_linesDirty) {
            return m_lines;
        }

        m_lines.clear();
        m_linesDirty = false;

        if (m_text.empty()) {
            return m_lines;
        }

        // 不启用自动换行时，仅按显式换行符拆分
        if (!m_wordWrap) {
            _appendExplicitLines();
            return m_lines;
        }

        // 可换行宽度不足时，退化为显式换行
        const i32 wrapWidth = _getWrapWidth();
        if (wrapWidth <= 0) {
            _appendExplicitLines();
            return m_lines;
        }

        // 基于宽度的自动换行
        std::string currentLine;
        currentLine.reserve(m_text.size());
        f32 currentWidth = 0.0f;

        util::text::utf8ForEachCodepoint(m_text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) {
            if (codePoint == U'\n') {
                _appendLine(std::move(currentLine));
                currentLine.clear();
                currentWidth = 0.0f;
                return;
            }

            const f32 advance = _measureGlyphAdvance(codePoint);
            // 跳过行首空格
            if (codePoint == U' ' && currentLine.empty()) {
                return;
            }

            // 当前行已有内容且加入当前字符会超宽时，换行
            if (currentWidth > 0.0f && currentWidth + advance > static_cast<f32>(wrapWidth)) {
                _appendLine(std::move(currentLine));
                currentLine.clear();
                currentWidth = 0.0f;
                // 换行后跳过空格
                if (codePoint == U' ') {
                    return;
                }
            }

            util::text::utf8Append(currentLine, codePoint);
            currentWidth += advance;
        });

        _appendLine(std::move(currentLine));
        return m_lines;
    }

    /**
     * @brief 将文本按显式换行符拆分为多行
     */
    void _appendExplicitLines() const
    {
        size_t start = 0;
        while (start <= m_text.size()) {
            const size_t end = m_text.find('\n', start);
            _appendLine(m_text.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) {
                break;
            }

            start = end + 1;
            // 文本末尾是换行符时，追加一个空行
            if (start == m_text.size()) {
                _appendLine({});
                break;
            }
        }
    }

    /**
     * @brief 获取当前可换行宽度（组件宽度减去水平内边距）
     */
    [[nodiscard]] i32 _getWrapWidth() const { return std::max(0, bounds().width - padding().horizontal()); }
};

} // namespace mc::client::ui::kagero::widget
