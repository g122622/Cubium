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

#include "IImage.hpp"
#include "IPaint.hpp"
#include "IPath.hpp"
#include "ITextBlob.hpp"
#include "client/ui/kagero/paint/Geometry.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc::client::ui::kagero::paint {

/**
 * @brief 画布抽象接口
 */
class ICanvas {
public:
    virtual ~ICanvas() = default;

    virtual void drawRect(const Rect& rect, const IPaint& paint) = 0;
    virtual void drawRRect(const RRect& roundRect, const IPaint& paint) = 0;
    virtual void drawCircle(f32 cx, f32 cy, f32 radius, const IPaint& paint) = 0;
    virtual void drawOval(const Rect& bounds, const IPaint& paint) = 0;
    virtual void drawPath(const IPath& path, const IPaint& paint) = 0;
    virtual void drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const IPaint& paint) = 0;

    /**
     * @brief 绘制渐变矩形
     * @param rect 矩形区域
     * @param color1 起始颜色（ARGB）
     * @param color2 结束颜色（ARGB）
     * @param vertical true=垂直渐变（从上到下），false=水平渐变（从左到右）
     */
    virtual void drawGradientRect(const Rect& rect, u32 color1, u32 color2, bool vertical) = 0;

    virtual void drawImage(const IImage& image, f32 x, f32 y) = 0;
    virtual void drawImageRect(const IImage& image, const Rect& src, const Rect& dst) = 0;
    virtual void drawImageNine(
        const IImage& image, const Rect& center, const Rect& dst, const IPaint* paint = nullptr) = 0;

    virtual void drawText(const std::string& text, f32 x, f32 y, const IPaint& paint) = 0;
    virtual void drawTextBlob(const ITextBlob& blob, f32 x, f32 y, const IPaint& paint) = 0;

    virtual void clipRect(const Rect& rect) = 0;
    virtual void clipRRect(const RRect& roundRect) = 0;
    virtual void clipPath(const IPath& path) = 0;
    virtual void clipOutRect(const Rect& rect) = 0;
    [[nodiscard]] virtual bool clipIsEmpty() const = 0;
    [[nodiscard]] virtual Rect getClipBounds() const = 0;

    virtual void translate(f32 dx, f32 dy) = 0;
    virtual void scale(f32 sx, f32 sy) = 0;
    virtual void rotate(f32 degrees) = 0;
    virtual void concat(const Matrix& matrix) = 0;
    virtual void setMatrix(const Matrix& matrix) = 0;
    [[nodiscard]] virtual Matrix getTotalMatrix() const = 0;

    virtual i32 save() = 0;
    virtual void restore() = 0;
    virtual void restoreToCount(i32 saveCount) = 0;

    virtual i32 saveLayer(const Rect* bounds, const IPaint* paint) = 0;
    virtual i32 saveLayerAlpha(const Rect* bounds, u8 alpha) = 0;

    [[nodiscard]] virtual i32 width() const = 0;
    [[nodiscard]] virtual i32 height() const = 0;

    // ==================== 文本测量 ====================

    /**
     * @brief 获取文本宽度
     * @param text 文本内容
     * @return 文本宽度（像素）
     */
    [[nodiscard]] virtual f32 getTextWidth(const std::string& text) const = 0;

    /**
     * @brief 获取字体高度
     * @return 字体高度（像素）
     */
    [[nodiscard]] virtual u32 getFontHeight() const = 0;
};

} // namespace mc::client::ui::kagero::paint
