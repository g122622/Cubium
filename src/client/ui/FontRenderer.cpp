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

#include "FontRenderer.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include <algorithm>

namespace mc::client {

using text::ITextComponent;
using text::StringTextComponent;

FontRenderer::FontRenderer() = default;

FontRenderer::~FontRenderer()
{
    destroy();
}

Result<void> FontRenderer::initialize(Font* font)
{
    if (font == nullptr) {
        return Error(ErrorCode::NullPointer, "Font pointer is null");
    }

    m_font = font;
    m_vertices.reserve(1024); // 预分配空间
    m_indices.reserve(1536);

    return {};
}

void FontRenderer::destroy()
{
    m_vertices.clear();
    m_indices.clear();
    m_font = nullptr;
}

void FontRenderer::beginBatch()
{
    m_vertices.clear();
    m_indices.clear();
    m_currentX = 0.0f;
    m_currentY = 0.0f;
    m_inBatch = true;
}

f32 FontRenderer::addText(const std::string& text, f32 x, f32 y, const TextStyle& style)
{
    if (!m_inBatch || m_font == nullptr) {
        return 0.0f;
    }

    f32 startX = x;
    f32 shadowOffset = Glyph::getShadowOffset() * m_scale;

    // 如果需要阴影，先绘制阴影
    if (style.shadow) {
        u32 shadowColor = 0xFF3F3F3F; // MC阴影颜色
        f32 shadowX = x + shadowOffset;
        f32 shadowY = y + shadowOffset;

        size_t pos = 0;
        while (pos < text.size()) {
            u32 codepoint = decodeCodepoint(text, pos);

            if (codepoint == '\n') {
                shadowX = startX + shadowOffset;
                shadowY += m_font->getFontHeight() * m_scale;
                continue;
            }

            const Glyph* glyph = m_font->getGlyph(codepoint);
            if (glyph != nullptr) {
                addGlyphVertices(*glyph, shadowX, shadowY, shadowColor, false);
                shadowX += glyph->advance * m_scale;
                if (style.bold) {
                    shadowX += Glyph::getBoldOffset() * m_scale;
                }
            } else {
                shadowX += 4.0f * m_scale; // 默认宽度
            }
        }
    }

    // 绘制主文本
    size_t pos = 0;
    while (pos < text.size()) {
        u32 codepoint = decodeCodepoint(text, pos);

        if (codepoint == '\n') {
            x = startX;
            y += m_font->getFontHeight() * m_scale;
            continue;
        }

        const Glyph* glyph = m_font->getGlyph(codepoint);
        if (glyph != nullptr) {
            addGlyphVertices(*glyph, x, y, style.color, style.italic);

            // 粗体：额外绘制一次偏移后的字形
            if (style.bold) {
                f32 boldOffset = Glyph::getBoldOffset() * m_scale;
                addGlyphVertices(*glyph, x + boldOffset, y, style.color, style.italic);
            }

            // 添加装饰效果
            if (style.strikethrough || style.underline) {
                addDecoration(x, y, glyph->advance * m_scale, style.color, style.strikethrough, style.underline);
            }

            x += glyph->advance * m_scale;
            if (style.bold) {
                x += Glyph::getBoldOffset() * m_scale;
            }
        } else {
            x += 4.0f * m_scale; // 默认宽度
        }
    }

    m_currentX = x;
    m_currentY = y;

    return x - startX;
}

f32 FontRenderer::addText(const ITextComponent& component, f32 x, f32 y, const TextStyle& baseStyle)
{
    if (!m_inBatch || m_font == nullptr) {
        return 0.0f;
    }

    return addTextComponent(component, x, y, baseStyle);
}

f32 FontRenderer::addTextComponent(const ITextComponent& component, f32 x, f32 y, const TextStyle& baseStyle)
{
    // 合并样式
    TextStyle renderStyle = mergeStyles(component.getStyle(), baseStyle);

    f32 startX = x;
    const StringTextComponent* stringComp = dynamic_cast<const StringTextComponent*>(&component);

    // 渲染当前组件的文本
    if (stringComp != nullptr) {
        const std::string& text = stringComp->getText();
        x += addText(text, x, y, renderStyle);
    } else {
        // 对于其他组件类型（如 TranslationTextComponent），渲染未格式化文本
        std::string unformattedText = component.getUnformattedText();
        x += addText(unformattedText, x, y, renderStyle);
    }

    // 递归渲染子组件
    const auto& siblings = component.getSiblings();
    for (const auto& sibling : siblings) {
        // 子组件继承父组件的样式作为基础
        x += addTextComponent(*sibling, x, y, renderStyle) - startX;
        startX = x; // 更新起始位置
    }

    return x - startX + (startX - x); // 返回总宽度
}

TextStyle FontRenderer::mergeStyles(const text::Style& style, const TextStyle& baseStyle) const
{
    TextStyle result = baseStyle;

    // 颜色
    if (style.getColor().has_value()) {
        result.color = text::getFormattingColor(*style.getColor());
    }

    // 样式标志
    if (style.isBold()) {
        result.bold = true;
    }
    if (style.isItalic()) {
        result.italic = true;
    }
    if (style.isUnderlined()) {
        result.underline = true;
    }
    if (style.isStrikethrough()) {
        result.strikethrough = true;
    }

    // 混淆效果暂不支持（需要动画）

    return result;
}

f32 FontRenderer::getTextWidth(const ITextComponent& component)
{
    if (m_font == nullptr) {
        return 0.0f;
    }

    // 使用未格式化文本计算宽度
    return getTextWidth(component.getUnformattedText());
}

f32 FontRenderer::addTextWithShadow(const std::string& text, f32 x, f32 y, u32 color)
{
    TextStyle style;
    style.color = color;
    style.shadow = true;
    return addText(text, x, y, style);
}

void FontRenderer::endBatch()
{
    m_inBatch = false;
}

f32 FontRenderer::getTextWidth(const std::string& text)
{
    if (m_font == nullptr) {
        return 0.0f;
    }

    f32 width = 0.0f;
    f32 maxWidth = 0.0f;
    size_t pos = 0;

    while (pos < text.size()) {
        u32 codepoint = decodeCodepoint(text, pos);

        if (codepoint == '\n') {
            maxWidth = std::max(maxWidth, width);
            width = 0.0f;
            continue;
        }

        const Glyph* glyph = m_font->getGlyph(codepoint);
        if (glyph != nullptr) {
            width += glyph->advance * m_scale;
        } else {
            width += 4.0f * m_scale;
        }
    }

    return std::max(maxWidth, width);
}

u32 FontRenderer::getFontHeight() const
{
    if (m_font == nullptr) {
        return static_cast<u32>(9 * m_scale); // 默认高度
    }
    return static_cast<u32>(m_font->getFontHeight() * m_scale);
}

size_t FontRenderer::estimateVertexCount(const std::string& text) const
{
    // 每个字符最多6个顶点（两个三角形）* 2（阴影）* 2（粗体）
    size_t charCount = 0;
    size_t pos = 0;
    while (pos < text.size()) {
        u8 byte = static_cast<u8>(text[pos]);
        if ((byte & 0x80) == 0) {
            pos += 1;
        } else if ((byte & 0xE0) == 0xC0) {
            pos += 2;
        } else if ((byte & 0xF0) == 0xE0) {
            pos += 3;
        } else if ((byte & 0xF8) == 0xF0) {
            pos += 4;
        } else {
            pos += 1;
        }
        charCount++;
    }
    return charCount * 6 * 4; // 阴影和粗体各翻倍
}

void FontRenderer::addGlyphVertices(const Glyph& glyph, f32 x, f32 y, u32 color, bool italic)
{
    // 计算字形边界
    // 注意：bearingY是从基线到字形顶部的距离
    // 屏幕坐标系中Y向下，所以需要调整

    // 应用缩放因子
    f32 scaledBearingY = glyph.bearingY * m_scale;
    f32 scaledHeight = glyph.height * m_scale;
    f32 scaledBearingX = glyph.bearingX * m_scale;
    f32 scaledWidth = glyph.width * m_scale;

    f32 glyphTop = y - scaledBearingY + static_cast<f32>(m_font->getFontHeight()) * m_scale;
    f32 glyphBottom = glyphTop + scaledHeight;
    f32 glyphLeft = x + scaledBearingX;
    f32 glyphRight = glyphLeft + scaledWidth;

    // 斜体偏移（顶部向右倾斜）
    f32 italicOffset = italic ? (scaledHeight * 0.25f) : 0.0f;

    // 添加4个顶点
    // 注意：现在不翻转纹理坐标V轴，屏幕Y轴向下与纹理V轴向下一致
    // 所以屏幕上方（小Y）对应纹理顶部（小V），屏幕下方（大Y）对应纹理底部（大V）
    // 字体使用槽位0（FONT_ATLAS_SLOT）
    constexpr u8 FONT_SLOT = 0;
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 左上（屏幕Y小，对应纹理V小，即v0）
    m_vertices.emplace_back(glyphLeft + italicOffset,
        glyphTop,
        glyph.u0,
        glyph.v0, // 使用v0（顶部）
        color,
        FONT_SLOT);

    // 右上
    m_vertices.emplace_back(glyphRight + italicOffset,
        glyphTop,
        glyph.u1,
        glyph.v0, // 使用v0（顶部）
        color,
        FONT_SLOT);

    // 右下（屏幕Y大，对应纹理V大，即v1）
    m_vertices.emplace_back(glyphRight,
        glyphBottom,
        glyph.u1,
        glyph.v1, // 使用v1（底部）
        color,
        FONT_SLOT);

    // 左下
    m_vertices.emplace_back(glyphLeft,
        glyphBottom,
        glyph.u0,
        glyph.v1, // 使用v1（底部）
        color,
        FONT_SLOT);

    // 添加两个三角形（6个索引）
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void FontRenderer::addDecoration(f32 x, f32 y, f32 width, u32 color, bool strikethrough, bool underline)
{
    u32 baseIndex = static_cast<u32>(m_vertices.size());
    f32 fontHeight = static_cast<f32>(m_font->getFontHeight()) * m_scale;

    // 装饰线使用槽位0（FONT_ATLAS_SLOT），但UV为负表示纯色
    constexpr u8 FONT_SLOT = 0;
    constexpr f32 SOLID_RECT_UV = -1.0f;

    // 删除线
    if (strikethrough) {
        f32 strikeY = y + fontHeight * 0.5f;
        f32 strikeHeight = 1.0f * m_scale;

        // 左上
        m_vertices.emplace_back(x, strikeY, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);
        // 右上
        m_vertices.emplace_back(x + width, strikeY, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);
        // 右下
        m_vertices.emplace_back(x + width, strikeY + strikeHeight, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);
        // 左下
        m_vertices.emplace_back(x, strikeY + strikeHeight, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);

        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 1);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 3);

        baseIndex += 4;
    }

    // 下划线
    if (underline) {
        f32 underlineY = y + fontHeight;
        f32 underlineHeight = 1.0f * m_scale;

        // 左上
        m_vertices.emplace_back(x, underlineY, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);
        // 右上
        m_vertices.emplace_back(x + width, underlineY, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);
        // 右下
        m_vertices.emplace_back(
            x + width, underlineY + underlineHeight, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);
        // 左下
        m_vertices.emplace_back(x, underlineY + underlineHeight, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_SLOT);

        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 1);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 3);
    }
}

u32 FontRenderer::decodeCodepoint(const std::string& text, size_t& pos) const
{
    if (pos >= text.size()) {
        return 0;
    }

    u8 byte = static_cast<u8>(text[pos]);

    // UTF-8解码
    if ((byte & 0x80) == 0) {
        // 单字节字符 (0xxxxxxx)
        u32 codepoint = byte;
        pos += 1;
        return codepoint;
    } else if ((byte & 0xE0) == 0xC0) {
        // 双字节字符 (110xxxxx 10xxxxxx)
        if (pos + 1 >= text.size()) {
            pos += 1;
            return '?';
        }
        u32 codepoint = ((byte & 0x1F) << 6) | (static_cast<u8>(text[pos + 1]) & 0x3F);
        pos += 2;
        return codepoint;
    } else if ((byte & 0xF0) == 0xE0) {
        // 三字节字符 (1110xxxx 10xxxxxx 10xxxxxx)
        if (pos + 2 >= text.size()) {
            pos += 1;
            return '?';
        }
        u32 codepoint = ((byte & 0x0F) << 12) | ((static_cast<u8>(text[pos + 1]) & 0x3F) << 6) |
            (static_cast<u8>(text[pos + 2]) & 0x3F);
        pos += 3;
        return codepoint;
    } else if ((byte & 0xF8) == 0xF0) {
        // 四字节字符 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if (pos + 3 >= text.size()) {
            pos += 1;
            return '?';
        }
        u32 codepoint = ((byte & 0x07) << 18) | ((static_cast<u8>(text[pos + 1]) & 0x3F) << 12) |
            ((static_cast<u8>(text[pos + 2]) & 0x3F) << 6) | (static_cast<u8>(text[pos + 3]) & 0x3F);
        pos += 4;
        return codepoint;
    } else {
        // 无效的UTF-8序列
        pos += 1;
        return '?';
    }
}

} // namespace mc::client
