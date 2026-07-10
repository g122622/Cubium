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
#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "client/renderer/trident/gui/ISpriteAtlas.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/paint/TextureImage.hpp"
#include <string>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 图片组件
 *
 * 在 UI 上显示一张来自 GUI 精灵图集的纹理图片。
 *
 * 设计参考 MC Java 版 `net.minecraft.client.gui.components.ImageWidget`：
 * - 非交互组件（`isActive() == false`），不参与焦点导航
 * - 支持两种尺寸语义：
 *   1. 显式尺寸：通过 `setBounds` 或模板 `size` 属性指定
 *   2. 自动尺寸：`m_autoWidth`/`m_autoHeight` 为 true 时，按纹理原始像素尺寸绘制
 * - 支持自定义着色（tint），默认全不透明白色
 * - 支持自定义 UV 子区域，用于从大图集中截取局部精灵
 * - 当未关联 `GuiSpriteAtlas` 或精灵不存在时，回退到纯色矩形，便于开发期可视化排查
 *
 * 与 `HudWidget` 等具体业务组件一致，使用 `GuiSpriteAtlas::createTextureImage`
 * 生成轻量级 `TextureImage` 引用后，交给 `PaintContext::drawImage` 绘制。
 * `ImageWidget` 不拥有任何 Vulkan 资源，纹理生命周期由外部 `GuiSpriteAtlas` 管理。
 *
 * 使用示例：
 * @code
 * auto image = std::make_unique<ImageWidget>("logo");
 * image->setSpriteSource(atlas, "title_logo");
 * image->setBounds(Rect(10, 10, 200, 60));
 * container->addChild(std::move(image));
 * @endcode
 *
 * 模板使用：
 * @code{.xml}
 * <image id="logo" src="title_logo" size="200,60" tint="#FFFFFFFF"/>
 * <image id="icon" src="heart_full" size="auto,auto"/>
 * @endcode
 */
class ImageWidget : public Widget {
public:
    ImageWidget()
    {
        // 对齐 MC Java ImageWidget.isActive()==false 语义：非交互组件
        setActive(false);
    }

    /**
     * @brief 构造函数
     * @param id 组件ID
     */
    explicit ImageWidget(std::string id)
        : Widget(std::move(id))
    {
        // 对齐 MC Java ImageWidget.isActive()==false 语义：非交互组件
        setActive(false);
    }

    /**
     * @brief 构造函数（带边界）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    ImageWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        // 对齐 MC Java ImageWidget.isActive()==false 语义：非交互组件
        setActive(false);
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 设置精灵来源
     *
     * 绑定一个 GUI 精灵图集与精灵ID，绘制时从中取图。
     * `atlas` 由外部拥有，`ImageWidget` 仅持有裸指针，不参与生命周期。
     *
     * @param atlas 精灵图集指针（可为 nullptr，表示无纹理来源）
     * @param spriteId 精灵ID（在图集中注册的名称）
     */
    void setSpriteSource(renderer::trident::gui::ISpriteAtlas* atlas, std::string spriteId)
    {
        m_atlas = atlas;
        m_spriteId = std::move(spriteId);
    }

    /**
     * @brief 设置精灵ID（保持已绑定的图集不变）
     */
    void setSpriteId(std::string spriteId) { m_spriteId = std::move(spriteId); }

    /**
     * @brief 获取精灵ID
     */
    [[nodiscard]] const std::string& spriteId() const { return m_spriteId; }

    /**
     * @brief 设置图集指针
     */
    void setAtlas(renderer::trident::gui::ISpriteAtlas* atlas) { m_atlas = atlas; }

    /**
     * @brief 获取图集指针
     */
    [[nodiscard]] renderer::trident::gui::ISpriteAtlas* atlas() const { return m_atlas; }

    /**
     * @brief 设置着色颜色（ARGB）
     *
     * 默认 `0xFFFFFFFF`（白色，全不透明），等价于无着色。
     */
    void setTint(u32 tint) { m_tint = tint; }

    /**
     * @brief 获取着色颜色
     */
    [[nodiscard]] u32 tint() const { return m_tint; }

    /**
     * @brief 设置自动宽度
     *
     * 启用后，绘制宽度取纹理原始像素宽度，忽略 `bounds().width`。
     */
    void setAutoWidth(bool autoWidth) { m_autoWidth = autoWidth; }

    /**
     * @brief 是否启用自动宽度
     */
    [[nodiscard]] bool autoWidth() const { return m_autoWidth; }

    /**
     * @brief 设置自动高度
     *
     * 启用后，绘制高度取纹理原始像素高度，忽略 `bounds().height`。
     */
    void setAutoHeight(bool autoHeight) { m_autoHeight = autoHeight; }

    /**
     * @brief 是否启用自动高度
     */
    [[nodiscard]] bool autoHeight() const { return m_autoHeight; }

    /**
     * @brief 同时设置自动宽高
     */
    void setAutoSize(bool autoSize)
    {
        m_autoWidth = autoSize;
        m_autoHeight = autoSize;
    }

    /**
     * @brief 设置自定义 UV 子区域
     *
     * 默认 UV 为 (0,0,1,1)，即整张纹理。
     * 当仅需绘制纹理的某个子区域时使用，单位为归一化纹理坐标。
     * 注意：当关联的来源是 `GuiSpriteAtlas` 精灵时，精灵自身已经携带了
     * 在图集中的 UV，自定义 UV 会被忽略；此接口主要供后续扩展直接纹理来源使用。
     *
     * @param u0 左上 U
     * @param v0 左上 V
     * @param u1 右下 U
     * @param v1 右下 V
     */
    void setUV(f32 u0, f32 v0, f32 u1, f32 v1)
    {
        m_u0 = u0;
        m_v0 = v0;
        m_u1 = u1;
        m_v1 = v1;
        m_customUV = true;
    }

    /**
     * @brief 清除自定义 UV，恢复为整张纹理
     */
    void clearUV()
    {
        m_u0 = 0.0f;
        m_v0 = 0.0f;
        m_u1 = 1.0f;
        m_v1 = 1.0f;
        m_customUV = false;
    }

    /**
     * @brief 是否设置了自定义 UV
     */
    [[nodiscard]] bool hasCustomUV() const { return m_customUV; }

    /**
     * @brief 绘制图片
     *
     * 当图集与精灵均可用时，使用 `GuiSpriteAtlas::createTextureImage` 生成
     * 轻量级 `TextureImage`，通过 `PaintContext::drawImage` 绘制到目标矩形。
     * 当图集不可用或精灵不存在时，回退为纯色矩形（使用 `m_tint` 作为填充色），
     * 便于在资源缺失时仍能可视化组件占位。
     *
     * @param ctx 绘图上下文
     */
    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;

        const Rect dst = _resolveDrawRect();

        if (!_tryDrawSprite(ctx, dst)) {
            _drawFallback(ctx, dst);
        }
    }

protected:
    /**
     * @brief 尺寸变化时无需特殊处理（绘制时动态解析）
     */
    void onSizeChanged() override {}

private:
    /// 默认着色（白色，全不透明）
    static constexpr u32 DEFAULT_TINT = 0xFFFFFFFF;

    /// 回退纯色（半透明品红，便于识别资源缺失）
    static constexpr u32 FALLBACK_COLOR = 0x80FF00FF;

    renderer::trident::gui::ISpriteAtlas* m_atlas = nullptr; ///< 精灵图集（非拥有）
    std::string m_spriteId;                                  ///< 精灵ID
    u32 m_tint = DEFAULT_TINT;                               ///< 着色（ARGB）
    bool m_autoWidth = false;                                ///< 是否按纹理宽度自适应
    bool m_autoHeight = false;                               ///< 是否按纹理高度自适应

    bool m_customUV = false; ///< 是否使用自定义 UV
    f32 m_u0 = 0.0f;         ///< 自定义 UV U0
    f32 m_v0 = 0.0f;         ///< 自定义 UV V0
    f32 m_u1 = 1.0f;         ///< 自定义 UV U1
    f32 m_v1 = 1.0f;         ///< 自定义 UV V1

    /**
     * @brief 解析实际绘制矩形
     *
     * 自动尺寸语义：
     * - 若启用 `m_autoWidth`，宽度取纹理原始像素宽度
     * - 若启用 `m_autoHeight`，高度取纹理原始像素高度
     * - 否则使用 `bounds()`
     *
     * 当无法获取纹理尺寸（无图集或精灵不存在）时，回退到 `bounds()`。
     */
    [[nodiscard]] Rect _resolveDrawRect() const
    {
        Rect dst = bounds();

        if (!m_autoWidth && !m_autoHeight) {
            return dst;
        }

        i32 texW = 0;
        i32 texH = 0;
        if (!_queryTextureSize(texW, texH)) {
            return dst;
        }

        if (m_autoWidth) {
            dst.width = texW;
        }
        if (m_autoHeight) {
            dst.height = texH;
        }
        return dst;
    }

    /**
     * @brief 查询当前精灵的纹理像素尺寸
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     * @return 是否成功查询到尺寸
     */
    [[nodiscard]] bool _queryTextureSize(i32& outWidth, i32& outHeight) const
    {
        if (m_atlas == nullptr || m_spriteId.empty()) {
            return false;
        }
        const auto* sprite = m_atlas->getSprite(m_spriteId);
        if (sprite == nullptr) {
            return false;
        }
        outWidth = sprite->width;
        outHeight = sprite->height;
        return true;
    }

    /**
     * @brief 尝试通过精灵图集绘制图片
     * @return 是否成功绘制（失败通常因图集未初始化或精灵未注册）
     */
    bool _tryDrawSprite(PaintContext& ctx, const Rect& dst) const
    {
        if (m_atlas == nullptr || m_spriteId.empty()) {
            return false;
        }
        if (!m_atlas->hasSprite(m_spriteId)) {
            return false;
        }

        auto image = m_atlas->createTextureImage(m_spriteId);
        if (!image.isValid()) {
            return false;
        }

        // drawImage(image, dstRect) 将整张纹理映射到 dst，
        // TextureImage 已携带精灵在图集中的 UV，故无需在此处理自定义 UV。
        ctx.drawImage(image, dst);
        return true;
    }

    /**
     * @brief 回退绘制：纯色矩形
     *
     * 当纹理不可用时，使用 `FALLBACK_COLOR` 绘制半透明矩形，
     * 便于在开发期直观发现资源缺失。
     */
    void _drawFallback(PaintContext& ctx, const Rect& dst) const
    {
        if (!dst.isValid()) return;
        ctx.drawFilledRect(dst, FALLBACK_COLOR);
    }
};

} // namespace mc::client::ui::kagero::widget
