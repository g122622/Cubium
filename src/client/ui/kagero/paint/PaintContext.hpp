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

#include "./Color.hpp"
#include "./contracts/ICanvas.hpp"
#include "./contracts/IImage.hpp"
#include "./contracts/IPaint.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/Geometry.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 绘图上下文
 *
 * 提供便捷的绘图方法，封装 ICanvas 操作。
 * 内部缓存 paint 对象以避免每帧创建临时对象。
 */
class PaintContext {
public:
    explicit PaintContext(paint::ICanvas& canvas);

    [[nodiscard]] paint::ICanvas& canvas();
    [[nodiscard]] const paint::ICanvas& canvas() const;

    // ==================== 绘图方法 ====================

    /**
     * @brief 绘制居中文本
     * @param text 文本内容
     * @param bounds 边界矩形
     * @param color 颜色（ARGB）
     */
    void drawTextCentered(const std::string& text, const Rect& bounds, u32 color);

    /**
     * @brief 绘制边框
     * @param bounds 边界矩形
     * @param width 边框宽度
     * @param color 颜色（ARGB）
     */
    void drawBorder(const Rect& bounds, f32 width, u32 color);

    /**
     * @brief 绘制填充矩形
     * @param bounds 边界矩形
     * @param color 颜色（ARGB）
     */
    void drawFilledRect(const Rect& bounds, u32 color);

    /**
     * @brief 绘制九宫格图像
     * @param image 图像
     * @param center 中心区域（可拉伸部分）
     * @param dst 目标区域
     * @param tint 色调（ARGB）
     */
    void drawNinePatch(const paint::IImage& image, const Rect& center, const Rect& dst, u32 tint);

    /**
     * @brief 绘制文本
     * @param text 文本内容
     * @param x X坐标
     * @param y Y坐标
     * @param color 颜色（ARGB）
     */
    void drawText(const std::string& text, i32 x, i32 y, u32 color);

    /**
     * @brief 绘制图像
     * @param image 图像
     * @param dst 目标区域
     */
    void drawImage(const paint::IImage& image, const Rect& dst);

    /**
     * @brief 绘制图像
     * @param image 图像
     * @param x X坐标
     * @param y Y坐标
     */
    void drawImage(const paint::IImage& image, i32 x, i32 y);

    /**
     * @brief 绘制圆角矩形
     * @param bounds 边界矩形
     * @param radius 圆角半径
     * @param color 颜色（ARGB）
     */
    void drawRoundedRect(const Rect& bounds, f32 radius, u32 color);

    /**
     * @brief 绘制渐变矩形
     * @param bounds 边界矩形
     * @param startColor 起始颜色（ARGB）
     * @param endColor 结束颜色（ARGB）
     * @param vertical 是否垂直渐变（true=垂直，false=水平）
     */
    void drawGradientRect(const Rect& bounds, u32 startColor, u32 endColor, bool vertical);

    // ==================== 文本测量 ====================

    /**
     * @brief 获取文本宽度
     * @param text 文本内容
     * @return 文本宽度（像素）
     */
    [[nodiscard]] f32 getTextWidth(const std::string& text) const;

    /**
     * @brief 获取字体高度
     * @return 字体高度（像素）
     */
    [[nodiscard]] u32 getFontHeight() const;

    // ==================== 裁剪方法 ====================

    /**
     * @brief 推入裁剪区域
     * @param rect 裁剪矩形
     * @return 当前保存点
     */
    i32 pushClip(const Rect& rect);

    /**
     * @brief 推入圆角裁剪区域
     * @param bounds 边界矩形
     * @param radius 圆角半径
     * @return 当前保存点
     */
    i32 pushClipRounded(const Rect& bounds, f32 radius);

    /**
     * @brief 弹出裁剪区域
     * 恢复到上一个裁剪状态
     */
    void popClip();

    // ==================== 变换方法 ====================

    /**
     * @brief 保存当前状态
     * @return 保存点计数
     */
    i32 save();

    /**
     * @brief 恢复到上一个保存的状态
     */
    void restore();

    /**
     * @brief 平移
     */
    void translate(f32 dx, f32 dy);

    /**
     * @brief 应用变换矩阵
     */
    void concat(const paint::Matrix& matrix);

    // ==================== Paint 访问器 ====================

    /**
     * @brief 获取填充画笔（用于绘制填充形状）
     * @note 返回的画笔已被缓存，可直接修改
     */
    [[nodiscard]] paint::IPaint& fillPaint() { return *m_fillPaint; }
    [[nodiscard]] const paint::IPaint& fillPaint() const { return *m_fillPaint; }

    /**
     * @brief 获取描边画笔（用于绘制边框）
     * @note 返回的画笔已被缓存，可直接修改
     */
    [[nodiscard]] paint::IPaint& strokePaint() { return *m_strokePaint; }
    [[nodiscard]] const paint::IPaint& strokePaint() const { return *m_strokePaint; }

private:
    paint::ICanvas& m_canvas;

    // 缓存的 paint 对象，避免每帧创建临时对象
    std::unique_ptr<paint::IPaint> m_fillPaint;
    std::unique_ptr<paint::IPaint> m_strokePaint;
};

} // namespace mc::client::ui::kagero::widget
