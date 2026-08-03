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

#include "Glyph.hpp"
#include "client/ui/kagero/paint/Geometry.hpp"
#include "client/ui/kagero/paint/contracts/IImage.hpp"
#include "client/ui/kagero/paint/contracts/IPaint.hpp"
#include "client/ui/kagero/paint/contracts/IPath.hpp"
#include "client/ui/kagero/paint/contracts/ITextBlob.hpp"
#include "common/core/Types.hpp"
#include "kagero/Types.hpp"
#include "kagero/paint/contracts/ICanvas.hpp"
#include <string>
#include <vector>

namespace mc::client {
class Font;
class FontRenderer;
} // namespace mc::client

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client::ui {

/**
 * @brief ICanvas 的 Trident 实现
 *
 * 简单适配器，将 ICanvas 调用转换为 GuiRenderer 调用。
 * 不持有任何 Vulkan 资源，只累积顶点数据并提交给 GuiRenderer。
 *
 * 注意：此类不是线程安全的，应在主线程使用。
 *
 * ## 实现限制
 *
 * 以下方法为简化实现或未实现（MC UI 不需要）：
 * - `drawRRect`：退化为普通矩形
 * - `drawCircle`/`drawOval`：退化为边界矩形
 * - `drawPath`：忽略（MC UI 是方正风格）
 * - `clipOutRect`：忽略并记录警告
 * - `saveLayer`/`saveLayerAlpha`：只保存裁剪和变换状态，不支持离屏渲染
 */
class TridentCanvas final : public kagero::paint::ICanvas {
public:
    /**
     * @brief 构造函数
     * @param renderer GuiRenderer 引用，用于实际渲染
     * @param font Font 引用，用于文本渲染
     */
    TridentCanvas(renderer::trident::gui::GuiRenderer& renderer, Font& font);
    ~TridentCanvas() override = default;

    // 禁止拷贝
    TridentCanvas(const TridentCanvas&) = delete;
    TridentCanvas& operator=(const TridentCanvas&) = delete;

    // ==================== 帧管理 ====================

    /**
     * @brief 开始新帧
     *
     * 清空内部状态，准备接收新的绘制命令。
     * 应在 GuiRenderer::beginFrame() 之后调用。
     */
    void beginFrame();

    /**
     * @brief 结束帧
     *
     * 完成当前帧的绘制。所有累积的顶点数据已通过 GuiRenderer 提交。
     * 应在 GuiRenderer::render() 之前调用。
     */
    void endFrame();

    // ==================== ICanvas 实现 ====================

    void drawRect(const kagero::Rect& rect, const kagero::paint::IPaint& paint) override;
    void drawRRect(const kagero::paint::RRect& roundRect, const kagero::paint::IPaint& paint) override;
    void drawCircle(f32 cx, f32 cy, f32 radius, const kagero::paint::IPaint& paint) override;
    void drawOval(const kagero::Rect& bounds, const kagero::paint::IPaint& paint) override;
    void drawPath(const kagero::paint::IPath& path, const kagero::paint::IPaint& paint) override;
    void drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const kagero::paint::IPaint& paint) override;

    void drawGradientRect(const kagero::Rect& rect, u32 color1, u32 color2, bool vertical) override;

    void drawImage(const kagero::paint::IImage& image, f32 x, f32 y) override;
    void drawImageRect(const kagero::paint::IImage& image, const kagero::Rect& src, const kagero::Rect& dst) override;
    void drawImageNine(const kagero::paint::IImage& image,
        const kagero::Rect& center,
        const kagero::Rect& dst,
        const kagero::paint::IPaint* paint) override;

    void drawText(const std::string& text, f32 x, f32 y, const kagero::paint::IPaint& paint) override;
    void drawTextBlob(const kagero::paint::ITextBlob& blob, f32 x, f32 y, const kagero::paint::IPaint& paint) override;

    void clipRect(const kagero::Rect& rect) override;
    void clipRRect(const kagero::paint::RRect& roundRect) override;
    void clipPath(const kagero::paint::IPath& path) override;
    void clipOutRect(const kagero::Rect& rect) override;
    [[nodiscard]] bool clipIsEmpty() const override;
    [[nodiscard]] kagero::Rect getClipBounds() const override;

    void translate(f32 dx, f32 dy) override;
    void scale(f32 sx, f32 sy) override;
    void rotate(f32 degrees) override;
    void concat(const kagero::paint::Matrix& matrix) override;
    void setMatrix(const kagero::paint::Matrix& matrix) override;
    [[nodiscard]] kagero::paint::Matrix getTotalMatrix() const override;

    i32 save() override;
    void restore() override;
    void restoreToCount(i32 saveCount) override;

    i32 saveLayer(const kagero::Rect* bounds, const kagero::paint::IPaint* paint) override;
    i32 saveLayerAlpha(const kagero::Rect* bounds, u8 alpha) override;

    [[nodiscard]] i32 width() const override;
    [[nodiscard]] i32 height() const override;

    // ==================== 文本测量 ====================

    [[nodiscard]] f32 getTextWidth(const std::string& text) const override;
    [[nodiscard]] u32 getFontHeight() const override;

    // ==================== 扩展接口 ====================

    /**
     * @brief 调整画布尺寸
     */
    void resize(i32 width, i32 height);

    /**
     * @brief 获取关联的 Font
     */
    [[nodiscard]] Font& font() { return m_font; }
    [[nodiscard]] const Font& font() const { return m_font; }

    /**
     * @brief 获取关联的 GuiRenderer
     */
    [[nodiscard]] renderer::trident::gui::GuiRenderer& renderer() { return m_renderer; }
    [[nodiscard]] const renderer::trident::gui::GuiRenderer& renderer() const { return m_renderer; }

private:
    /**
     * @brief 应用当前变换到点
     */
    void _transformPoint(f32& x, f32& y) const;

    /**
     * @brief 从 IPaint 提取颜色
     */
    [[nodiscard]] u32 _extractColor(const kagero::paint::IPaint& paint) const;

    renderer::trident::gui::GuiRenderer& m_renderer;
    Font& m_font;

    i32 m_width = 0;
    i32 m_height = 0;
    kagero::Rect m_clipBounds;
    kagero::paint::Matrix m_matrix;
    std::vector<kagero::Rect> m_clipStack;
    std::vector<kagero::paint::Matrix> m_matrixStack;
    std::vector<f32> m_alphaStack; ///< 用于 saveLayerAlpha
};

} // namespace mc::client::ui
