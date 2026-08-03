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

#include "TridentCanvas.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/Color.hpp"
#include "client/ui/kagero/paint/Geometry.hpp"
#include "client/ui/kagero/paint/contracts/IImage.hpp"
#include "client/ui/kagero/paint/contracts/IPaint.hpp"
#include "client/ui/kagero/paint/contracts/IPath.hpp"
#include "client/ui/kagero/paint/contracts/ITextBlob.hpp"
#include "common/core/Types.hpp"
#include "kagero/paint/TextureImage.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <spdlog/spdlog.h>

namespace mc::client::ui {

TridentCanvas::TridentCanvas(renderer::trident::gui::GuiRenderer& renderer, Font& font)
    : m_renderer(renderer)
    , m_font(font)
    , m_clipBounds{0, 0, 0, 0}
    , m_matrix(kagero::paint::Matrix::identity())
{}

void TridentCanvas::beginFrame()
{
    m_clipBounds = kagero::Rect{0, 0, m_width, m_height};
    m_matrix = kagero::paint::Matrix::identity();
    m_clipStack.clear();
    m_matrixStack.clear();
    m_alphaStack.clear();
}

void TridentCanvas::endFrame()
{
    // 所有绘制操作已直接提交到 GuiRenderer
    // 无需额外处理
}

void TridentCanvas::drawRect(const kagero::Rect& rect, const kagero::paint::IPaint& paint)
{
    const u32 color = _extractColor(paint);
    const kagero::paint::PaintStyle style = paint.style();

    f32 x1 = static_cast<f32>(rect.x);
    f32 y1 = static_cast<f32>(rect.y);
    f32 x2 = x1 + static_cast<f32>(rect.width);
    f32 y2 = y1 + static_cast<f32>(rect.height);

    // 应用变换
    _transformPoint(x1, y1);
    _transformPoint(x2, y2);

    // 确保坐标顺序正确
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    if (style == kagero::paint::PaintStyle::Fill) {
        m_renderer.fillRect(x1, y1, x2 - x1, y2 - y1, color);
    } else if (style == kagero::paint::PaintStyle::Stroke) {
        const f32 strokeWidth = paint.strokeWidth();
        const f32 half = strokeWidth * 0.5f;
        // 绘制4条边框线（上、下、左、右）
        m_renderer.fillRect(x1, y1 - half, x2 - x1, strokeWidth, color); // 上
        m_renderer.fillRect(x1, y2 - half, x2 - x1, strokeWidth, color); // 下
        m_renderer.fillRect(x1 - half, y1, strokeWidth, y2 - y1, color); // 左
        m_renderer.fillRect(x2 - half, y1, strokeWidth, y2 - y1, color); // 右
    }
}

void TridentCanvas::drawRRect(const kagero::paint::RRect& roundRect, const kagero::paint::IPaint& paint)
{
    // MC UI 是方正风格，不需要圆角矩形
    // 退化为普通矩形
    drawRect(roundRect.rect, paint);
}

void TridentCanvas::drawCircle(f32 cx, f32 cy, f32 radius, const kagero::paint::IPaint& paint)
{
    const u32 color = _extractColor(paint);
    _transformPoint(cx, cy);

    if (paint.style() == kagero::paint::PaintStyle::Fill) {
        m_renderer.fillRect(cx - radius, cy - radius, radius * 2, radius * 2, color);
    } else {
        // Stroke: 使用边框矩形近似
        const f32 halfStroke = paint.strokeWidth() / 2.0f;
        m_renderer.fillRect(cx - radius, cy - radius - halfStroke, radius * 2, paint.strokeWidth(), color); // 上
        m_renderer.fillRect(cx - radius, cy + radius - halfStroke, radius * 2, paint.strokeWidth(), color); // 下
        m_renderer.fillRect(cx - radius - halfStroke, cy - radius, paint.strokeWidth(), radius * 2, color); // 左
        m_renderer.fillRect(cx + radius - halfStroke, cy - radius, paint.strokeWidth(), radius * 2, color); // 右
    }
}

void TridentCanvas::drawOval(const kagero::Rect& bounds, const kagero::paint::IPaint& paint)
{
    // MC UI 是方正风格，不需要椭圆
    // 退化为普通矩形
    drawRect(bounds, paint);
}

void TridentCanvas::drawPath(const kagero::paint::IPath& path, const kagero::paint::IPaint& paint)
{
    // MC UI 是方正风格，不需要路径绘制
    (void)path;
    (void)paint;
}

void TridentCanvas::drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const kagero::paint::IPaint& paint)
{
    const u32 color = _extractColor(paint);
    const f32 strokeWidth = paint.strokeWidth();

    _transformPoint(x0, y0);
    _transformPoint(x1, y1);

    if (strokeWidth <= 0.0f) {
        return;
    }

    const f32 halfWidth = strokeWidth / 2.0f;
    const f32 dx = x1 - x0;
    const f32 dy = y1 - y0;
    const f32 len = std::sqrt(dx * dx + dy * dy);

    if (len < 0.001f) {
        m_renderer.fillRect(x0 - halfWidth, y0 - halfWidth, strokeWidth, strokeWidth, color);
        return;
    }

    // 水平线：直接用矩形绘制（精确）
    if (std::abs(dy) < 0.001f) {
        const f32 left = std::min(x0, x1);
        m_renderer.fillRect(left, y0 - halfWidth, std::abs(dx), strokeWidth, color);
        return;
    }

    // 垂直线：直接用矩形绘制（精确）
    if (std::abs(dx) < 0.001f) {
        const f32 top = std::min(y0, y1);
        m_renderer.fillRect(x0 - halfWidth, top, strokeWidth, std::abs(dy), color);
        return;
    }

    // 斜线：使用旋转矩形的包围盒近似
    // 精确渲染需要 fillQuad，当前使用更紧凑的包围盒
    const f32 nx = -dy / len * halfWidth;
    const f32 ny = dx / len * halfWidth;

    // 四个顶点
    const f32 vx0 = x0 + nx, vy0 = y0 + ny;
    const f32 vx1 = x0 - nx, vy1 = y0 - ny;
    const f32 vx2 = x1 - nx, vy2 = y1 - ny;
    const f32 vx3 = x1 + nx, vy3 = y1 + ny;

    // 使用包围盒绘制（比之前的包围盒更紧凑）
    const f32 minX = std::min({vx0, vx1, vx2, vx3});
    const f32 minY = std::min({vy0, vy1, vy2, vy3});
    const f32 maxX = std::max({vx0, vx1, vx2, vx3});
    const f32 maxY = std::max({vy0, vy1, vy2, vy3});

    m_renderer.fillRect(minX, minY, maxX - minX, maxY - minY, color);
}

void TridentCanvas::drawGradientRect(const kagero::Rect& rect, u32 color1, u32 color2, bool vertical)
{
    f32 x = static_cast<f32>(rect.x);
    f32 y = static_cast<f32>(rect.y);
    const f32 w = static_cast<f32>(rect.width);
    const f32 h = static_cast<f32>(rect.height);

    _transformPoint(x, y);

    if (vertical) {
        m_renderer.fillGradientRect(x, y, w, h, color1, color2);
    } else {
        m_renderer.fillGradientRectHorizontal(x, y, w, h, color1, color2);
    }
}

void TridentCanvas::drawImage(const kagero::paint::IImage& image, f32 x, f32 y)
{
    // 尝试转换为 TextureImage
    const auto* textureImage = dynamic_cast<const kagero::paint::TextureImage*>(&image);
    if (textureImage == nullptr) {
        spdlog::warn("TridentCanvas::drawImage - unsupported image type");
        return;
    }

    if (!textureImage->isValid()) {
        return;
    }

    _transformPoint(x, y);

    const f32 w = static_cast<f32>(textureImage->width());
    const f32 h = static_cast<f32>(textureImage->height());

    // 绘制纹理矩形，传递图集槽位与着色（tint）
    m_renderer.drawTexturedRect(x,
        y,
        w,
        h,
        textureImage->u0(),
        textureImage->v0(),
        textureImage->u1(),
        textureImage->v1(),
        textureImage->tint(),
        textureImage->atlasSlot());
}

void TridentCanvas::drawImageRect(const kagero::paint::IImage& image, const kagero::Rect& src, const kagero::Rect& dst)
{
    const auto* textureImage = dynamic_cast<const kagero::paint::TextureImage*>(&image);
    if (textureImage == nullptr) {
        spdlog::warn("TridentCanvas::drawImageRect - unsupported image type");
        return;
    }

    if (!textureImage->isValid()) {
        return;
    }

    f32 x = static_cast<f32>(dst.x);
    f32 y = static_cast<f32>(dst.y);
    _transformPoint(x, y);

    // 计算源区域对应的 UV 坐标
    const f32 imgW = static_cast<f32>(textureImage->width());
    const f32 imgH = static_cast<f32>(textureImage->height());

    // 从纹理空间到 UV 空间的映射
    const f32 u0 = textureImage->u0() + (textureImage->u1() - textureImage->u0()) * (static_cast<f32>(src.x) / imgW);
    const f32 v0 = textureImage->v0() + (textureImage->v1() - textureImage->v0()) * (static_cast<f32>(src.y) / imgH);
    const f32 u1 =
        textureImage->u0() + (textureImage->u1() - textureImage->u0()) * (static_cast<f32>(src.x + src.width) / imgW);
    const f32 v1 =
        textureImage->v0() + (textureImage->v1() - textureImage->v0()) * (static_cast<f32>(src.y + src.height) / imgH);

    m_renderer.drawTexturedRect(x,
        y,
        static_cast<f32>(dst.width),
        static_cast<f32>(dst.height),
        u0,
        v0,
        u1,
        v1,
        textureImage->tint(),
        textureImage->atlasSlot());
}

void TridentCanvas::drawImageNine(const kagero::paint::IImage& image,
    const kagero::Rect& center,
    const kagero::Rect& dst,
    const kagero::paint::IPaint* paint)
{
    const auto* textureImage = dynamic_cast<const kagero::paint::TextureImage*>(&image);
    if (textureImage == nullptr) {
        spdlog::warn("TridentCanvas::drawImageNine - unsupported image type");
        return;
    }

    if (!textureImage->isValid()) {
        return;
    }

    // 提取颜色（如果有），否则回退到纹理自身的着色
    u32 tint = textureImage->tint();
    if (paint != nullptr) {
        tint = _extractColor(*paint);
    }

    f32 x = static_cast<f32>(dst.x);
    f32 y = static_cast<f32>(dst.y);
    _transformPoint(x, y);

    const f32 imgW = static_cast<f32>(textureImage->width());
    const f32 imgH = static_cast<f32>(textureImage->height());
    const f32 dstW = static_cast<f32>(dst.width);
    const f32 dstH = static_cast<f32>(dst.height);

    // 九宫格区域：
    // 左宽度 = center.x
    // 右宽度 = imgW - (center.x + center.width)
    // 上高度 = center.y
    // 下高度 = imgH - (center.y + center.height)

    const f32 leftW = static_cast<f32>(center.x);
    const f32 rightW = imgW - static_cast<f32>(center.x + center.width);
    const f32 topH = static_cast<f32>(center.y);
    const f32 bottomH = imgH - static_cast<f32>(center.y + center.height);

    // 目标区域尺寸
    const f32 dstLeftW = leftW;
    const f32 dstRightW = rightW;
    const f32 dstTopH = topH;
    const f32 dstBottomH = bottomH;
    const f32 dstCenterW = dstW - leftW - rightW;
    const f32 dstCenterH = dstH - topH - bottomH;

    // UV 坐标计算辅助函数
    auto toU = [&](f32 px) -> f32 {
        return textureImage->u0() + (textureImage->u1() - textureImage->u0()) * (px / imgW);
    };
    auto toV = [&](f32 py) -> f32 {
        return textureImage->v0() + (textureImage->v1() - textureImage->v0()) * (py / imgH);
    };

    // UV 坐标
    const f32 u0 = toU(0), u1 = toU(leftW), u2 = toU(leftW + center.width), u3 = toU(imgW);
    const f32 v0 = toV(0), v1 = toV(topH), v2 = toV(topH + center.height), v3 = toV(imgH);

    // Y 坐标
    const f32 y0 = y;
    const f32 y1 = y + dstTopH;
    const f32 y2 = y + dstTopH + dstCenterH;
    const f32 y3 = y + dstH;

    // X 坐标
    const f32 x0 = x;
    const f32 x1 = x + dstLeftW;
    const f32 x2 = x + dstLeftW + dstCenterW;
    const f32 x3 = x + dstW;

    // 获取图集槽位
    const u8 atlasSlot = textureImage->atlasSlot();

    // 绘制函数
    auto drawRegion = [&](f32 dx, f32 dy, f32 dw, f32 dh, f32 su0, f32 sv0, f32 su1, f32 sv1) {
        m_renderer.drawTexturedRect(dx, dy, dw, dh, su0, sv0, su1, sv1, tint, atlasSlot);
    };

    // 行 0: 上边
    drawRegion(x0, y0, dstLeftW, dstTopH, u0, v0, u1, v1); // 左上
    if (dstCenterW > 0) {
        drawRegion(x1, y0, dstCenterW, dstTopH, u1, v0, u2, v1); // 中上
    }
    drawRegion(x2, y0, dstRightW, dstTopH, u2, v0, u3, v1); // 右上

    // 行 1: 中间
    if (dstCenterH > 0) {
        drawRegion(x0, y1, dstLeftW, dstCenterH, u0, v1, u1, v2); // 左中
        if (dstCenterW > 0) {
            drawRegion(x1, y1, dstCenterW, dstCenterH, u1, v1, u2, v2); // 中中
        }
        drawRegion(x2, y1, dstRightW, dstCenterH, u2, v1, u3, v2); // 右中
    }

    // 行 2: 下边
    drawRegion(x0, y2, dstLeftW, dstBottomH, u0, v2, u1, v3); // 左下
    if (dstCenterW > 0) {
        drawRegion(x1, y2, dstCenterW, dstBottomH, u1, v2, u2, v3); // 中下
    }
    drawRegion(x2, y2, dstRightW, dstBottomH, u2, v2, u3, v3); // 右下
}

void TridentCanvas::drawText(const std::string& text, f32 x, f32 y, const kagero::paint::IPaint& paint)
{
    const u32 color = _extractColor(paint);
    _transformPoint(x, y);

    // text已经是UTF-8编码，直接传递给渲染器
    m_renderer.drawText(text, x, y, color, false);
}

void TridentCanvas::drawTextBlob(const kagero::paint::ITextBlob& blob, f32 x, f32 y, const kagero::paint::IPaint& paint)
{
    const u32 color = _extractColor(paint);
    _transformPoint(x, y);

    // blob.text()已经是UTF-8编码，直接传递给渲染器
    const std::string& text = blob.text();
    m_renderer.drawText(text, x, y, color, false);
}

void TridentCanvas::clipRect(const kagero::Rect& rect)
{
    m_clipBounds = m_clipBounds.intersection(rect);
}

void TridentCanvas::clipRRect(const kagero::paint::RRect& roundRect)
{
    // 简化实现：退化为矩形裁剪
    clipRect(roundRect.rect);
}

void TridentCanvas::clipPath(const kagero::paint::IPath& path)
{
    // 路径裁剪暂不实现
    (void)path;
}

void TridentCanvas::clipOutRect(const kagero::Rect& rect)
{
    // MC UI 不使用反向裁剪，忽略并记录警告
    spdlog::warn("TridentCanvas::clipOutRect not implemented - MC UI does not require this");
    (void)rect;
}

bool TridentCanvas::clipIsEmpty() const
{
    return !m_clipBounds.isValid();
}

kagero::Rect TridentCanvas::getClipBounds() const
{
    return m_clipBounds;
}

void TridentCanvas::translate(f32 dx, f32 dy)
{
    // 3x3 仿射矩阵的平移
    m_matrix.m[2] += dx;
    m_matrix.m[5] += dy;
}

void TridentCanvas::scale(f32 sx, f32 sy)
{
    // 3x3 仿射矩阵的缩放
    m_matrix.m[0] *= sx;
    m_matrix.m[4] *= sy;
}

void TridentCanvas::rotate(f32 degrees)
{
    m_matrix.rotate(degrees);
}

void TridentCanvas::concat(const kagero::paint::Matrix& matrix)
{
    m_matrix = m_matrix * matrix;
}

void TridentCanvas::setMatrix(const kagero::paint::Matrix& matrix)
{
    m_matrix = matrix;
}

kagero::paint::Matrix TridentCanvas::getTotalMatrix() const
{
    return m_matrix;
}

i32 TridentCanvas::save()
{
    m_clipStack.push_back(m_clipBounds);
    m_matrixStack.push_back(m_matrix);
    return static_cast<i32>(m_clipStack.size());
}

void TridentCanvas::restore()
{
    if (!m_clipStack.empty()) {
        m_clipBounds = m_clipStack.back();
        m_clipStack.pop_back();
    }
    if (!m_matrixStack.empty()) {
        m_matrix = m_matrixStack.back();
        m_matrixStack.pop_back();
    }
    if (!m_alphaStack.empty()) {
        m_alphaStack.pop_back();
    }
}

void TridentCanvas::restoreToCount(i32 saveCount)
{
    while (static_cast<i32>(m_clipStack.size()) > saveCount) {
        restore();
    }
}

i32 TridentCanvas::saveLayer(const kagero::Rect* bounds, const kagero::paint::IPaint* paint)
{
    if (bounds != nullptr) {
        clipRect(*bounds);
    }
    // 简化实现：只保存裁剪和变换状态，不支持离屏渲染
    // MC UI 不需要真正的图层合成
    (void)paint;
    return save();
}

i32 TridentCanvas::saveLayerAlpha(const kagero::Rect* bounds, u8 alpha)
{
    if (bounds != nullptr) {
        clipRect(*bounds);
    }
    m_alphaStack.push_back(static_cast<f32>(alpha) / 255.0f);
    // 简化实现：只保存裁剪、变换和 alpha 状态，不支持离屏渲染
    return save();
}

i32 TridentCanvas::width() const
{
    return m_width;
}

i32 TridentCanvas::height() const
{
    return m_height;
}

f32 TridentCanvas::getTextWidth(const std::string& text) const
{
    return m_font.getStringWidth(text);
}

u32 TridentCanvas::getFontHeight() const
{
    return m_font.getFontHeight();
}

void TridentCanvas::resize(i32 width, i32 height)
{
    m_width = width;
    m_height = height;
    m_clipBounds = kagero::Rect{0, 0, width, height};
}

void TridentCanvas::_transformPoint(f32& x, f32& y) const
{
    m_matrix.transformPoint(x, y);
}

u32 TridentCanvas::_extractColor(const kagero::paint::IPaint& paint) const
{
    kagero::paint::Color color = paint.color();

    // 应用 alpha 堆栈
    f32 alpha = color.a;
    for (f32 a : m_alphaStack) {
        alpha *= a;
    }

    // 转换为 ARGB
    const u32 aa = static_cast<u32>(alpha * 255.0f) & 0xFF;
    const u32 rr = static_cast<u32>(color.r * 255.0f) & 0xFF;
    const u32 gg = static_cast<u32>(color.g * 255.0f) & 0xFF;
    const u32 bb = static_cast<u32>(color.b * 255.0f) & 0xFF;

    return (aa << 24) | (rr << 16) | (gg << 8) | bb;
}

} // namespace mc::client::ui
