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

#include "TextWidget.hpp"
#include "Widget.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/Geometry.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextParser.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/util/text/Utf8.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 富文本组件
 *
 * 支持渲染 ITextComponent，包括：
 * - 颜色和样式（粗体、斜体、下划线、删除线）
 * - 点击事件（打开URL、执行命令、建议命令、复制到剪贴板）
 * - 悬停事件（显示文本提示）
 * - 自动换行
 */
class RichTextWidget : public Widget {
public:
    using ClickCallback = std::function<void(const text::ClickEvent&)>;
    using HoverCallback = std::function<void(const text::HoverEvent&, i32, i32)>;

    // ==================== 构造函数 ====================

    RichTextWidget() = default;

    /**
     * @brief 构造富文本组件
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    RichTextWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 构造带文本的富文本组件
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param text 文本组件
     */
    RichTextWidget(std::string id, i32 x, i32 y, i32 width, i32 height, std::unique_ptr<text::ITextComponent> text)
        : Widget(std::move(id))
        , m_text(std::move(text))
    {
        setBounds(Rect(x, y, width, height));
    }

    ~RichTextWidget() override = default;

    // 禁止拷贝
    RichTextWidget(const RichTextWidget&) = delete;
    RichTextWidget& operator=(const RichTextWidget&) = delete;

    // 允许移动
    RichTextWidget(RichTextWidget&&) noexcept = default;
    RichTextWidget& operator=(RichTextWidget&&) noexcept = default;

    // ==================== 生命周期 ====================

    void init() override
    {
        Widget::init();
        _relayout();
    }

    void tick(f32 dt) override
    {
        Widget::tick(dt);

        // 混淆效果动画：累积经过时间，用于驱动混淆字符刷新
        m_obfuscatedTimer += dt;

        // 混淆字符每隔一段时间刷新（约每 0.05 秒刷新一次，约 20 FPS 视觉效果）
        // MC Java 的混淆动画频率与帧率一致，因为每次 drawInBatch 都重新生成随机字符
        // 这里使用定时刷新以避免不必要的重绘开销
        constexpr f32 OBFUSCATED_REFRESH_INTERVAL = 0.05f;
        if (m_obfuscatedTimer >= OBFUSCATED_REFRESH_INTERVAL) {
            m_obfuscatedTimer -= OBFUSCATED_REFRESH_INTERVAL;
        }
    }

    // ==================== 绘制 ====================

    void paint(PaintContext& ctx) override
    {
        if (!isVisible() || m_lines.empty()) {
            return;
        }

        // 重新布局（如果需要）
        if (m_linesDirty) {
            _relayout();
        }

        // 绘制每一行
        for (const auto& line : m_lines) {
            f32 x = static_cast<f32>(bounds().x);

            // 水平对齐
            if (m_alignment == TextAlignment::Center) {
                x += (bounds().width - line.width) / 2.0f;
            } else if (m_alignment == TextAlignment::Right) {
                x += bounds().width - line.width;
            }

            // 绘制该行的所有运行
            for (const auto& run : line.runs) {
                // 构建渲染样式
                u32 color =
                    run.style.getColor().has_value() ? text::getFormattingColor(*run.style.getColor()) : m_baseColor;

                // 计算文本宽度（用于装饰线）
                f32 runWidth = run.advanceWidth;
                i32 textX = static_cast<i32>(x);
                i32 textY = static_cast<i32>(line.y);

                // 混淆文字：替换为随机等宽字符
                std::string displayText;
                if (run.style.isObfuscated()) {
                    displayText = _obfuscateText(run.text);
                }
                const std::string& textToDraw = run.style.isObfuscated() ? displayText : run.text;

                // 斜体：通过倾斜变换绘制
                if (run.style.isItalic()) {
                    // 保存当前状态
                    ctx.save();
                    // 斜体效果：X方向倾斜约12度，以文本基线为中心
                    ctx.translate(static_cast<f32>(textX), static_cast<f32>(textY) + s_fontHeight * 0.5f);
                    ctx.concat(paint::Matrix::makeSkew(-12.0f, 0.0f));
                    ctx.translate(static_cast<f32>(-textX), static_cast<f32>(-textY) - s_fontHeight * 0.5f);
                }

                // 绘制阴影
                if (m_shadow) {
                    ctx.drawText(textToDraw, textX + 1, textY + 1, m_shadowColor);
                    // 粗体阴影需要额外偏移绘制
                    if (run.style.isBold()) {
                        ctx.drawText(textToDraw, textX + 2, textY + 1, m_shadowColor);
                    }
                }

                // 绘制主文本
                ctx.drawText(textToDraw, textX, textY, color);

                // 粗体：额外绘制一次偏移文本
                if (run.style.isBold()) {
                    ctx.drawText(textToDraw, textX + 1, textY, color);
                }

                // 删除线：在文本中间绘制水平线
                if (run.style.isStrikethrough()) {
                    f32 strikethroughY = textY + s_fontHeight * 0.5f;
                    ctx.drawFilledRect(
                        Rect(textX, static_cast<i32>(strikethroughY), static_cast<i32>(runWidth), 1), color);
                }

                // 下划线：在文本底部绘制水平线
                if (run.style.isUnderlined()) {
                    f32 underlineY = textY + s_fontHeight - 1.0f;
                    ctx.drawFilledRect(Rect(textX, static_cast<i32>(underlineY), static_cast<i32>(runWidth), 1), color);
                }

                // 斜体：恢复状态
                if (run.style.isItalic()) {
                    ctx.restore();
                }

                x += run.advanceWidth;
            }
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)button;
        (void)mods;
        if (!isActive() || !isVisible() || !m_eventsEnabled) {
            return false;
        }

        // 转换为本地坐标
        i32 localX = mouseX - bounds().x;
        i32 localY = mouseY - bounds().y;

        // 查找点击的运行
        const TextRun* run = _findRunAt(localX, localY);
        if (run == nullptr) {
            return false;
        }

        m_pressedRun = run;

        // 处理点击事件
        if (run->style.getClickEvent() != nullptr) {
            _handleClickEvent(*run->style.getClickEvent());
            return true;
        }

        return false;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        (void)mods;
        m_pressedRun = nullptr;
        return false;
    }

    void onMouseEnter() override { Widget::onMouseEnter(); }

    void onMouseLeave() override
    {
        Widget::onMouseLeave();
        m_hoveredRun = nullptr;
    }

    bool onMouseMove(i32 mouseX, i32 mouseY) override
    {
        if (!isActive() || !isVisible() || !m_eventsEnabled) {
            return false;
        }

        // 转换为本地坐标
        i32 localX = mouseX - bounds().x;
        i32 localY = mouseY - bounds().y;

        // 查找悬停的运行
        const TextRun* run = _findRunAt(localX, localY);

        // 检查悬停状态变化
        if (run != m_hoveredRun) {
            m_hoveredRun = run;

            // 处理悬停事件
            if (run != nullptr && run->style.getHoverEvent() != nullptr) {
                _handleHoverEvent(*run->style.getHoverEvent(), mouseX, mouseY);
            }
        }

        return run != nullptr;
    }

    // ==================== 文本设置 ====================

    /**
     * @brief 设置文本内容
     * @param text 文本组件（转移所有权）
     */
    void setText(std::unique_ptr<text::ITextComponent> text)
    {
        m_text = std::move(text);
        m_linesDirty = true;
    }

    /**
     * @brief 设置纯文本内容
     * @param text 纯文本字符串
     */
    void setText(const std::string& text)
    {
        m_text = text::TextParser::parse(text);
        m_linesDirty = true;
    }

    /**
     * @brief 获取文本组件
     */
    [[nodiscard]] const text::ITextComponent* getText() const { return m_text.get(); }

    /**
     * @brief 获取纯文本内容
     */
    [[nodiscard]] std::string getUnformattedText() const
    {
        if (m_text == nullptr) {
            return "";
        }
        return m_text->getUnformattedText();
    }

    // ==================== 样式设置 ====================

    void setBaseColor(u32 color) { m_baseColor = color; }
    void setShadow(bool shadow) { m_shadow = shadow; }
    void setShadowColor(u32 color) { m_shadowColor = color; }
    void setWordWrap(bool wrap)
    {
        m_wordWrap = wrap;
        m_linesDirty = true;
    }
    void setAlignment(TextAlignment alignment) { m_alignment = alignment; }
    void setLineSpacing(f32 spacing)
    {
        m_lineSpacing = spacing;
        m_linesDirty = true;
    }
    void setMaxLines(i32 maxLines)
    {
        m_maxLines = maxLines;
        m_linesDirty = true;
    }
    void setFont(::mc::client::Font* font)
    {
        m_font = font;
        m_linesDirty = true;
    }
    void setEventsEnabled(bool enabled) { m_eventsEnabled = enabled; }
    void setOnClick(ClickCallback callback) { m_onClick = std::move(callback); }
    void setOnHover(HoverCallback callback) { m_onHover = std::move(callback); }

    // ==================== 布局计算 ====================

    [[nodiscard]] f32 getTextWidth() const
    {
        if (m_linesDirty) {
            _relayout();
        }
        f32 maxWidth = 0.0f;
        for (const auto& line : m_lines) {
            maxWidth = std::max(maxWidth, line.width);
        }
        return maxWidth;
    }

    [[nodiscard]] f32 getTextHeight() const
    {
        if (m_linesDirty) {
            _relayout();
        }
        if (m_lines.empty()) {
            return 0.0f;
        }
        return m_lines.back().y + s_fontHeight - bounds().y;
    }

    [[nodiscard]] i32 getLineCount() const
    {
        if (m_linesDirty) {
            _relayout();
        }
        return static_cast<i32>(m_lines.size());
    }

protected:
    void onSizeChanged() override { m_linesDirty = true; }

private:
    /**
     * @brief 文本样式片段
     */
    struct TextRun {
        std::string text;  // 文本内容
        text::Style style; // 样式
        Rect bounds;       // 渲染区域
        f32 advanceWidth;  // 文本宽度
    };

    /**
     * @brief 文本行
     */
    struct TextLine {
        std::vector<TextRun> runs;
        f32 width = 0.0f;
        f32 height = 0.0f;
        f32 y = 0.0f;
    };

    /**
     * @brief 重新计算布局（延迟布局，可在const方法中调用）
     */
    void _relayout() const
    {
        m_lines.clear();
        m_linesDirty = false;

        if (m_text == nullptr) {
            return;
        }

        // 分解组件为 TextRun 列表
        std::vector<TextRun> runs;
        _flattenComponent(*m_text, text::Style(), runs);

        // 布局运行
        _layoutRuns(runs);
    }

    /**
     * @brief 将 ITextComponent 分解为 TextRun 列表
     */
    void _flattenComponent(
        const text::ITextComponent& component, const text::Style& parentStyle, std::vector<TextRun>& runs) const
    {
        // 合并样式
        text::Style style = component.getStyle().mergeWithParent(parentStyle);

        // 获取文本
        const text::StringTextComponent* stringComp = dynamic_cast<const text::StringTextComponent*>(&component);

        if (stringComp != nullptr) {
            const std::string& text = stringComp->getText();
            if (!text.empty()) {
                TextRun run;
                run.text = text;
                run.style = style;
                run.advanceWidth = 0.0f;
                runs.push_back(std::move(run));
            }
        } else {
            // 对于其他组件类型，使用未格式化文本
            std::string unformatted = component.getUnformattedText();
            if (!unformatted.empty()) {
                TextRun run;
                run.text = unformatted;
                run.style = style;
                run.advanceWidth = 0.0f;
                runs.push_back(std::move(run));
            }
        }

        // 递归处理子组件
        const auto& siblings = component.getSiblings();
        for (const auto& sibling : siblings) {
            _flattenComponent(*sibling, style, runs);
        }
    }

    /**
     * @brief 将 TextRun 列表布局为行
     */
    void _layoutRuns(std::vector<TextRun>& runs) const
    {
        if (runs.empty()) {
            return;
        }

        f32 x = static_cast<f32>(bounds().x);
        f32 y = static_cast<f32>(bounds().y);
        f32 maxWidth = static_cast<f32>(bounds().width);
        i32 lineCount = 0;

        TextLine currentLine;
        currentLine.y = y;
        currentLine.height = s_fontHeight * m_lineSpacing;

        for (auto& run : runs) {
            // 计算运行宽度
            run.advanceWidth = _measureTextWidth(run.text);

            // 检查是否需要换行
            if (m_wordWrap && x + run.advanceWidth > bounds().x + maxWidth && !currentLine.runs.empty()) {
                // 保存当前行
                m_lines.push_back(std::move(currentLine));
                currentLine = TextLine();
                lineCount++;

                // 开始新行
                x = static_cast<f32>(bounds().x);
                y += s_fontHeight * m_lineSpacing;
                currentLine.y = y;
                currentLine.height = s_fontHeight * m_lineSpacing;

                // 检查最大行数
                if (m_maxLines > 0 && lineCount >= m_maxLines) {
                    break;
                }
            }

            // 设置运行边界
            run.bounds = Rect(static_cast<i32>(x),
                static_cast<i32>(y),
                static_cast<i32>(run.advanceWidth),
                static_cast<i32>(s_fontHeight));

            // 添加到当前行
            currentLine.runs.push_back(run);
            currentLine.width += run.advanceWidth;
            x += run.advanceWidth;
        }

        // 保存最后一行
        if (!currentLine.runs.empty()) {
            m_lines.push_back(std::move(currentLine));
        }
    }

    /**
     * @brief 测量文本宽度
     */
    [[nodiscard]] f32 _measureTextWidth(const std::string& text) const
    {
        if (m_font) {
            f32 width = 0.0f;
            util::text::utf8ForEachCodepoint(text, [&](u32 codePoint, size_t /*byteOffset*/, size_t /*byteLength*/) {
                if (const auto* glyph = m_font->getGlyph(codePoint); glyph != nullptr) {
                    width += glyph->advance;
                } else {
                    // 缺失字形的回退宽度
                    constexpr f32 MISSING_GLYPH_WIDTH = 4.0f;
                    width += MISSING_GLYPH_WIDTH;
                }
            });
            return width;
        }
        // 无字体时的回退宽度：每码点8像素
        constexpr f32 FALLBACK_CHAR_WIDTH = 8.0f;
        return static_cast<f32>(util::text::utf8CodepointCount(text)) * FALLBACK_CHAR_WIDTH;
    }

    /**
     * @brief 将文本中的非空格字符替换为等宽随机字符（混淆效果 §k）
     *
     * 核心逻辑：
     * 1. 获取原始字符的前进宽度
     * 2. 从字体中找到相同宽度的随机字符替换
     * 3. 空格不被替换
     *
     * @param text 原始文本
     * @return 混淆后的文本（UTF-8编码）
     */
    [[nodiscard]] std::string _obfuscateText(const std::string& text) const
    {
        if (!m_font) {
            return text;
        }

        std::string result;
        result.reserve(text.size());

        util::text::utf8ForEachCodepoint(text, [&](u32 codepoint, size_t byteOffset, size_t byteLength) {
            // 空格不替换
            if (codepoint == ' ') {
                result.append(text, byteOffset, byteLength);
                return;
            }

            // 获取原始字符的前进宽度
            const Glyph* originalGlyph = m_font->getGlyph(codepoint);
            if (originalGlyph == nullptr) {
                // 未知字符保留原样
                result.append(text, byteOffset, byteLength);
                return;
            }

            // 从字体中查找等宽随机字符
            i32 width = static_cast<i32>(std::ceil(originalGlyph->advance));
            const Glyph* randomGlyph = m_font->getRandomGlyph(m_obfuscatedRandom, width);
            if (randomGlyph == nullptr) {
                // 没有匹配宽度的字符，保留原样
                result.append(text, byteOffset, byteLength);
                return;
            }

            // 将随机码点编码为UTF-8
            util::text::utf8Append(result, randomGlyph->codepoint);
        });

        return result;
    }

    /**
     * @brief 查找指定位置的文本运行
     */
    [[nodiscard]] const TextRun* _findRunAt(i32 x, i32 y) const
    {
        for (const auto& line : m_lines) {
            for (const auto& run : line.runs) {
                if (run.bounds.contains(x, y)) {
                    return &run;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief 处理点击事件
     *
     * 默认实现：如果设置了 m_onClick 回调则调用回调。
     * 应用层应通过 setOnClick() 设置回调来处理具体事件：
     * - OpenUrl: 调用平台 API 打开浏览器
     * - RunCommand: 调用 CommandDispatcher 执行命令
     * - SuggestCommand: 在聊天框填入命令
     * - CopyToClipboard: 调用平台 API 复制文本
     */
    bool _handleClickEvent(const text::ClickEvent& event)
    {
        // 调用回调
        if (m_onClick) {
            m_onClick(event);
            return true;
        }

        // 无回调时不执行默认处理
        return false;
    }

    /**
     * @brief 处理悬停事件
     *
     * 默认实现：如果设置了 m_onHover 回调则调用回调。
     * 应用层应通过 setOnHover() 设置回调来处理具体事件：
     * - ShowText: 显示工具提示
     * - ShowItem: 显示物品提示
     * - ShowEntity: 显示实体提示
     */
    void _handleHoverEvent(const text::HoverEvent& event, i32 x, i32 y)
    {
        // 调用回调
        if (m_onHover) {
            m_onHover(event, x, y);
        }
    }

    // 文本内容
    std::unique_ptr<text::ITextComponent> m_text;

    // 布局数据（mutable 允许在const方法中延迟更新布局缓存）
    mutable std::vector<TextLine> m_lines;
    mutable bool m_linesDirty = true;

    // 样式属性
    u32 m_baseColor = Colors::WHITE;
    bool m_shadow = true;
    u32 m_shadowColor = Colors::MC_DARK_GRAY;
    bool m_wordWrap = true;
    TextAlignment m_alignment = TextAlignment::Left;
    f32 m_lineSpacing = 1.0f;
    i32 m_maxLines = 0;                   // 0 = 无限制
    ::mc::client::Font* m_font = nullptr; ///< 字体指针（外部管理生命周期）
    constexpr static f32 s_fontHeight = 9.0f;

    // 事件处理
    bool m_eventsEnabled = true;
    const TextRun* m_hoveredRun = nullptr;
    const TextRun* m_pressedRun = nullptr;

    // 回调
    ClickCallback m_onClick;
    HoverCallback m_onHover;

    // 混淆效果动画状态
    mutable math::Random m_obfuscatedRandom; // 混淆字符随机数生成器
    f32 m_obfuscatedTimer = 0.0f;            // 混淆刷新计时器
};

} // namespace mc::client::ui::kagero::widget
