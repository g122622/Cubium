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

#include "Font.hpp"
#include "Glyph.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mc::client {

/**
 * @brief 文本渲染器
 *
 * 负责将文本渲染到屏幕上，支持：
 * - 阴影
 * - 粗体
 * - 斜体
 * - 颜色
 * - 混淆（§k，随机替换等宽字符）
 * - UTF-8文本
 * - ITextComponent富文本
 */
class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    // 禁止拷贝
    FontRenderer(const FontRenderer&) = delete;
    FontRenderer& operator=(const FontRenderer&) = delete;

    /**
     * @brief 初始化字体渲染器
     * @param font 字体对象
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(Font* font);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 开始文本批次
     *
     * 清空顶点缓冲区，准备接收新的文本
     */
    void beginBatch();

    /**
     * @brief 添加文本到批次
     * @param text 文本内容（UTF-8）
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param style 文本样式
     * @return 实际渲染宽度
     */
    f32 addText(const std::string& text, f32 x, f32 y, const TextStyle& style = {});

    /**
     * @brief 添加富文本组件到批次
     * @param component 文本组件
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param baseStyle 基础样式（会与组件样式合并）
     * @return 实际渲染宽度
     */
    f32 addText(const text::ITextComponent& component, f32 x, f32 y, const TextStyle& baseStyle = {});

    /**
     * @brief 添加带阴影的文本到批次
     * @param text 文本内容（UTF-8）
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param color 文本颜色
     * @return 实际渲染宽度
     */
    f32 addTextWithShadow(const std::string& text, f32 x, f32 y, u32 color = Colors::WHITE);

    /**
     * @brief 结束文本批次
     *
     * 返回所有文本的顶点数据，供GPU渲染使用
     */
    void endBatch();

    /**
     * @brief 获取顶点数据
     */
    [[nodiscard]] const std::vector<GuiVertex>& vertices() const { return m_vertices; }
    [[nodiscard]] std::vector<GuiVertex>& vertices() { return m_vertices; }

    /**
     * @brief 获取索引数据
     */
    [[nodiscard]] const std::vector<u32>& indices() const { return m_indices; }
    [[nodiscard]] std::vector<u32>& indices() { return m_indices; }

    /**
     * @brief 获取关联的字体
     */
    [[nodiscard]] Font* font() const { return m_font; }

    /**
     * @brief 获取文本宽度
     * @param text 文本内容
     * @return 宽度（像素）
     */
    [[nodiscard]] f32 getTextWidth(const std::string& text);

    /**
     * @brief 获取富文本组件宽度
     * @param component 文本组件
     * @return 宽度（像素）
     */
    [[nodiscard]] f32 getTextWidth(const text::ITextComponent& component);

    /**
     * @brief 获取字体高度
     */
    [[nodiscard]] u32 getFontHeight() const;

    /**
     * @brief 设置缩放因子
     * @param scale 缩放因子（默认1.0）
     */
    void setScale(f32 scale) { m_scale = scale; }

    /**
     * @brief 获取缩放因子
     */
    [[nodiscard]] f32 scale() const { return m_scale; }

    /**
     * @brief 计算绘制文本所需的顶点数
     */
    [[nodiscard]] size_t estimateVertexCount(const std::string& text) const;

private:
    /**
     * @brief 添加单个字形顶点
     */
    void _addGlyphVertices(const Glyph& glyph, f32 x, f32 y, u32 color, bool italic);

    /**
     * @brief 添加装饰效果（删除线、下划线）
     */
    void _addDecoration(f32 x, f32 y, f32 width, u32 color, bool strikethrough, bool underline);

    /**
     * @brief 从UTF-8字符串解码码点
     * @param text UTF-8文本
     * @param pos 当前位置（会被更新）
     * @return 解码的码点
     */
    [[nodiscard]] u32 _decodeCodepoint(const std::string& text, size_t& pos) const;

    /**
     * @brief 将 TextFormatting::Style 转换为 TextStyle
     * @param style 文本样式
     * @param baseStyle 基础渲染样式
     * @return 合并后的渲染样式
     */
    [[nodiscard]] TextStyle _mergeStyles(const text::Style& style, const TextStyle& baseStyle) const;

    /**
     * @brief 递归渲染 ITextComponent
     */
    f32 _addTextComponent(const text::ITextComponent& component, f32 x, f32 y, const TextStyle& baseStyle);

    Font* m_font = nullptr;
    std::vector<GuiVertex> m_vertices; // 顶点缓冲
    std::vector<u32> m_indices;        // 索引缓冲
    f32 m_currentX = 0.0f;             // 当前X位置
    f32 m_currentY = 0.0f;             // 当前Y位置
    f32 m_scale = 1.0f;                // 缩放因子
    bool m_inBatch = false;            // 是否在批次中
    math::Random m_random;             // 混淆文字使用的随机数生成器
};

} // namespace mc::client
