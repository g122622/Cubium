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

/**
 * @file ImageWidgetTest.cpp
 * @brief ImageWidget 单元测试
 *
 * 覆盖：
 * - 构造与默认状态（非交互、默认 tint、无自定义 UV）
 * - 属性 setter/getter（spriteId、atlas、tint、autoWidth/Height、UV）
 * - 绘制行为：无图集时回退纯色矩形；不可见时不绘制
 * - 绘制行为：有图集时走 createTextureImage + drawImage 路径
 *
 * 图集访问通过 ISpriteAtlas 抽象接口注入 FakeSpriteAtlas 桩实现，
 * 无需 Vulkan 设备即可覆盖精灵绘制路径。
 */

#include "client/ui/kagero/widget/ImageWidget.hpp"
#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "client/renderer/trident/gui/ISpriteAtlas.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/paint/contracts/ICanvas.hpp"
#include "client/ui/kagero/paint/contracts/IImage.hpp"
#include "client/ui/kagero/paint/contracts/IPaint.hpp"
#include <unordered_map>
#include <gtest/gtest.h>
#include <vulkan/vulkan.h>

using namespace mc;
using namespace mc::client;
using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;

namespace {

/**
 * @brief 测试用精灵图集桩实现
 *
 * 实现 ISpriteAtlas 接口，不依赖 Vulkan，可在单元测试中注入 ImageWidget
 * 以验证精灵绘制路径（createTextureImage + drawImage）。
 */
class FakeSpriteAtlas final : public renderer::trident::gui::ISpriteAtlas {
public:
    /// 注册一个精灵，返回自身引用便于链式配置
    FakeSpriteAtlas& withSprite(const std::string& id, i32 w, i32 h)
    {
        renderer::trident::gui::GuiSprite sprite;
        sprite.id = id;
        sprite.width = w;
        sprite.height = h;
        sprites[id] = sprite;
        return *this;
    }

    [[nodiscard]] const renderer::trident::gui::GuiSprite* getSprite(const std::string& id) const override
    {
        auto it = sprites.find(id);
        return it != sprites.end() ? &it->second : nullptr;
    }

    [[nodiscard]] bool hasSprite(const std::string& id) const override { return sprites.count(id) > 0; }

    [[nodiscard]] paint::TextureImage createTextureImage(const std::string& spriteId) const override
    {
        const auto* sprite = getSprite(spriteId);
        if (sprite == nullptr) {
            return paint::TextureImage(VK_NULL_HANDLE, VK_NULL_HANDLE, 0, 0);
        }
        // 使用非空 VkImageView 标记有效性（isValid 检查 imageView != VK_NULL_HANDLE）
        const auto view = reinterpret_cast<VkImageView>(0x1234);
        const auto sampler = reinterpret_cast<VkSampler>(0x5678);
        return paint::TextureImage(view, sampler, sprite->width, sprite->height);
    }

    std::unordered_map<std::string, renderer::trident::gui::GuiSprite> sprites;
};

/**
 * @brief 记录绘制调用的测试画布
 *
 * 仅覆盖 ImageWidget 测试所需调用，其余接口保持空实现。
 */
class RecordingCanvas final : public paint::ICanvas {
public:
    void reset()
    {
        filledRectCalled = false;
        imageRectCalled = false;
        imageCalled = false;
        lastFilledRect = Rect{};
        lastFilledColor = 0;
        lastImageRectDst = Rect{};
    }

    void drawRect(const Rect& rect, const paint::IPaint& paint) override
    {
        filledRectCalled = true;
        lastFilledRect = rect;
        lastFilledColor = paint.color().toARGB();
    }

    void drawRRect(const paint::RRect&, const paint::IPaint&) override {}
    void drawCircle(f32, f32, f32, const paint::IPaint&) override {}
    void drawOval(const Rect&, const paint::IPaint&) override {}
    void drawPath(const paint::IPath&, const paint::IPaint&) override {}
    void drawLine(f32, f32, f32, f32, const paint::IPaint&) override {}
    void drawGradientRect(const Rect&, u32, u32, bool) override {}

    void drawImage(const paint::IImage&, f32, f32) override { imageCalled = true; }

    void drawImageRect(const paint::IImage&, const Rect& src, const Rect& dst) override
    {
        (void)src;
        imageRectCalled = true;
        lastImageRectDst = dst;
    }

    void drawImageNine(const paint::IImage&, const Rect&, const Rect&, const paint::IPaint*) override {}

    void drawText(const std::string&, f32, f32, const paint::IPaint&) override {}
    void drawTextBlob(const paint::ITextBlob&, f32, f32, const paint::IPaint&) override {}

    void clipRect(const Rect&) override {}
    void clipRRect(const paint::RRect&) override {}
    void clipPath(const paint::IPath&) override {}
    void clipOutRect(const Rect&) override {}
    [[nodiscard]] bool clipIsEmpty() const override { return false; }
    [[nodiscard]] Rect getClipBounds() const override { return Rect{}; }
    void translate(f32, f32) override {}
    void scale(f32, f32) override {}
    void rotate(f32) override {}
    void concat(const paint::Matrix&) override {}
    void setMatrix(const paint::Matrix&) override {}
    [[nodiscard]] paint::Matrix getTotalMatrix() const override { return paint::Matrix::identity(); }
    i32 save() override { return 0; }
    void restore() override {}
    void restoreToCount(i32) override {}
    i32 saveLayer(const Rect*, const paint::IPaint*) override { return 0; }
    i32 saveLayerAlpha(const Rect*, u8) override { return 0; }
    [[nodiscard]] i32 width() const override { return 800; }
    [[nodiscard]] i32 height() const override { return 600; }
    [[nodiscard]] f32 getTextWidth(const std::string&) const override { return 0.0f; }
    [[nodiscard]] u32 getFontHeight() const override { return 12; }

    bool filledRectCalled = false;
    bool imageRectCalled = false;
    bool imageCalled = false;
    Rect lastFilledRect{};
    u32 lastFilledColor = 0;
    Rect lastImageRectDst{};
};

} // namespace

// ==================== 构造与默认状态测试 ====================

TEST(ImageWidgetTest, DefaultConstructorIsNonInteractive)
{
    ImageWidget image;
    // 对齐 MC Java ImageWidget.isActive()==false 语义：默认非交互
    EXPECT_FALSE(image.isActive());
    EXPECT_TRUE(image.isVisible());
    EXPECT_TRUE(image.id().empty());
}

TEST(ImageWidgetTest, ConstructWithId)
{
    ImageWidget image("logo");
    EXPECT_EQ("logo", image.id());
    EXPECT_FALSE(image.isActive());
}

TEST(ImageWidgetTest, ConstructWithBounds)
{
    ImageWidget image("logo", 10, 20, 200, 60);
    EXPECT_EQ("logo", image.id());
    EXPECT_EQ(10, image.x());
    EXPECT_EQ(20, image.y());
    EXPECT_EQ(200, image.width());
    EXPECT_EQ(60, image.height());
}

// ==================== 属性 setter/getter 测试 ====================

TEST(ImageWidgetTest, SetSpriteId)
{
    ImageWidget image("logo");
    EXPECT_TRUE(image.spriteId().empty());

    image.setSpriteId("title_logo");
    EXPECT_EQ("title_logo", image.spriteId());
}

TEST(ImageWidgetTest, SetAtlasReturnsSamePointer)
{
    ImageWidget image("logo");
    EXPECT_EQ(nullptr, image.atlas());

    // 使用 nullptr 验证 setter 不崩溃且可读回
    image.setAtlas(nullptr);
    EXPECT_EQ(nullptr, image.atlas());
}

TEST(ImageWidgetTest, SetTint)
{
    ImageWidget image("logo");
    // 默认 tint 为白色全不透明
    EXPECT_EQ(0xFFFFFFFFu, image.tint());

    image.setTint(0x80FF0000u);
    EXPECT_EQ(0x80FF0000u, image.tint());
}

TEST(ImageWidgetTest, SetAutoWidthAndHeight)
{
    ImageWidget image("logo");
    EXPECT_FALSE(image.autoWidth());
    EXPECT_FALSE(image.autoHeight());

    image.setAutoWidth(true);
    EXPECT_TRUE(image.autoWidth());
    EXPECT_FALSE(image.autoHeight());

    image.setAutoHeight(true);
    EXPECT_TRUE(image.autoWidth());
    EXPECT_TRUE(image.autoHeight());
}

TEST(ImageWidgetTest, SetAutoSizeSetsBoth)
{
    ImageWidget image("logo");
    EXPECT_FALSE(image.autoWidth());
    EXPECT_FALSE(image.autoHeight());

    image.setAutoSize(true);
    EXPECT_TRUE(image.autoWidth());
    EXPECT_TRUE(image.autoHeight());

    image.setAutoSize(false);
    EXPECT_FALSE(image.autoWidth());
    EXPECT_FALSE(image.autoHeight());
}

TEST(ImageWidgetTest, SetUVAndClear)
{
    ImageWidget image("logo");
    EXPECT_FALSE(image.hasCustomUV());

    image.setUV(0.25f, 0.5f, 0.75f, 1.0f);
    EXPECT_TRUE(image.hasCustomUV());

    image.clearUV();
    EXPECT_FALSE(image.hasCustomUV());
}

// ==================== 绘制行为测试 ====================

TEST(ImageWidgetTest, PaintWhenInvisibleDoesNothing)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    ImageWidget image("logo", 10, 20, 100, 50);

    image.setVisible(false);
    image.paint(ctx);

    EXPECT_FALSE(canvas.filledRectCalled);
    EXPECT_FALSE(canvas.imageRectCalled);
    EXPECT_FALSE(canvas.imageCalled);
}

TEST(ImageWidgetTest, PaintWithoutAtlasFallsBackToFilledRect)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    ImageWidget image("logo", 10, 20, 100, 50);

    image.paint(ctx);

    // 无图集时回退到纯色矩形（品红色半透明，便于识别资源缺失）
    EXPECT_TRUE(canvas.filledRectCalled);
    EXPECT_FALSE(canvas.imageRectCalled);
    EXPECT_EQ(10, canvas.lastFilledRect.x);
    EXPECT_EQ(20, canvas.lastFilledRect.y);
    EXPECT_EQ(100, canvas.lastFilledRect.width);
    EXPECT_EQ(50, canvas.lastFilledRect.height);
    // 0x80FF00FF = 半透明品红（FALLBACK_COLOR）
    EXPECT_EQ(0x80FF00FFu, canvas.lastFilledColor);
}

TEST(ImageWidgetTest, PaintFallbackRespectsBounds)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    ImageWidget image("logo");
    image.setBounds(Rect(5, 8, 42, 16));

    image.paint(ctx);

    EXPECT_TRUE(canvas.filledRectCalled);
    EXPECT_EQ(5, canvas.lastFilledRect.x);
    EXPECT_EQ(8, canvas.lastFilledRect.y);
    EXPECT_EQ(42, canvas.lastFilledRect.width);
    EXPECT_EQ(16, canvas.lastFilledRect.height);
}

TEST(ImageWidgetTest, PaintFallbackSkipsInvalidBounds)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    ImageWidget image("logo", 0, 0, 0, 0); // width=0, height=0 无效

    image.paint(ctx);

    // drawFilledRect 在 bounds 无效时不应被调用
    EXPECT_FALSE(canvas.filledRectCalled);
}

TEST(ImageWidgetTest, PaintFallbackWithAutoSizeFallsBackToZeroBoundsWhenNoTexture)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    ImageWidget image("logo", 10, 20, 0, 0);
    image.setAutoSize(true);

    // 无图集时无法查询纹理尺寸，回退到 bounds()（0,0,0,0）→ 无效 → 不绘制
    image.paint(ctx);

    EXPECT_FALSE(canvas.filledRectCalled);
}

// ==================== 精灵绘制路径测试（FakeSpriteAtlas） ====================

TEST(ImageWidgetTest, PaintWithAtlasAndSpriteDrawsImage)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    FakeSpriteAtlas atlas;
    atlas.withSprite("logo", 200, 60);

    ImageWidget image("logo", 10, 20, 200, 60);
    image.setSpriteSource(&atlas, "logo");

    image.paint(ctx);

    // 走 createTextureImage + drawImage 路径
    EXPECT_TRUE(canvas.imageRectCalled);
    EXPECT_FALSE(canvas.filledRectCalled);
    // drawImage(image, dstRect) 将纹理映射到 dst
    EXPECT_EQ(10, canvas.lastImageRectDst.x);
    EXPECT_EQ(20, canvas.lastImageRectDst.y);
    EXPECT_EQ(200, canvas.lastImageRectDst.width);
    EXPECT_EQ(60, canvas.lastImageRectDst.height);
}

TEST(ImageWidgetTest, PaintWithAtlasButMissingSpriteFallsBack)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    FakeSpriteAtlas atlas;
    // 不注册 "missing" 精灵

    ImageWidget image("logo", 10, 20, 100, 50);
    image.setSpriteSource(&atlas, "missing");

    image.paint(ctx);

    // 精灵不存在 → 回退纯色矩形
    EXPECT_FALSE(canvas.imageRectCalled);
    EXPECT_TRUE(canvas.filledRectCalled);
}

TEST(ImageWidgetTest, PaintWithAtlasAndAutoSizeUsesTextureSize)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    FakeSpriteAtlas atlas;
    atlas.withSprite("icon", 16, 16); // 纹理原始尺寸 16x16

    ImageWidget image("icon", 5, 8, 0, 0);
    image.setSpriteSource(&atlas, "icon");
    image.setAutoSize(true);

    image.paint(ctx);

    // 自动尺寸：dst 宽高取纹理原始像素尺寸
    EXPECT_TRUE(canvas.imageRectCalled);
    EXPECT_EQ(5, canvas.lastImageRectDst.x);
    EXPECT_EQ(8, canvas.lastImageRectDst.y);
    EXPECT_EQ(16, canvas.lastImageRectDst.width);
    EXPECT_EQ(16, canvas.lastImageRectDst.height);
}

TEST(ImageWidgetTest, PaintWithAtlasAutoWidthOnlyUsesTextureWidth)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    FakeSpriteAtlas atlas;
    atlas.withSprite("bar", 182, 5);

    ImageWidget image("bar", 0, 0, 0, 22); // 高度固定 22，宽度 auto
    image.setSpriteSource(&atlas, "bar");
    image.setAutoWidth(true);

    image.paint(ctx);

    EXPECT_TRUE(canvas.imageRectCalled);
    EXPECT_EQ(182, canvas.lastImageRectDst.width); // 宽度取纹理
    EXPECT_EQ(22, canvas.lastImageRectDst.height); // 高度保留 bounds
}

TEST(ImageWidgetTest, PaintWithAtlasNullPointerFallsBack)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    ImageWidget image("logo", 10, 20, 100, 50);
    image.setSpriteSource(nullptr, "logo");

    image.paint(ctx);

    // atlas 为 nullptr → 回退
    EXPECT_FALSE(canvas.imageRectCalled);
    EXPECT_TRUE(canvas.filledRectCalled);
}

TEST(ImageWidgetTest, PaintWithAtlasEmptySpriteIdFallsBack)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    FakeSpriteAtlas atlas;
    atlas.withSprite("logo", 100, 50);

    ImageWidget image("logo", 10, 20, 100, 50);
    image.setSpriteSource(&atlas, ""); // 空 spriteId

    image.paint(ctx);

    EXPECT_FALSE(canvas.imageRectCalled);
    EXPECT_TRUE(canvas.filledRectCalled);
}
