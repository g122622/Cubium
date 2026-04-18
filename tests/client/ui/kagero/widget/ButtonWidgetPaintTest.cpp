/**
 * @file ButtonWidgetPaintTest.cpp
 * @brief ButtonWidget 绘制行为测试
 */

#include <gtest/gtest.h>
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/Types.hpp"

using namespace mc;
using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;

namespace {

/**
 * @brief 记录绘制调用的测试画布
 *
 * 只覆盖本测试需要的调用，其余接口保持空实现。
 */
class RecordingCanvas final : public paint::ICanvas {
public:
    /**
     * @brief 重置记录状态
     */
    void reset() {
        filledRectCalled = false;
        textCalled = false;
        lastText.clear();
    }

    void drawRect(const Rect& rect, const paint::IPaint& paint) override {
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
    void drawImage(const paint::IImage&, f32, f32) override {}
    void drawImageRect(const paint::IImage&, const Rect&, const Rect&) override {}
    void drawImageNine(const paint::IImage&, const Rect&, const Rect&, const paint::IPaint*) override {}

    void drawText(const String& text, f32 x, f32 y, const paint::IPaint& paint) override {
        textCalled = true;
        lastText = text;
        lastTextX = x;
        lastTextY = y;
        lastTextColor = paint.color().toARGB();
    }

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
    [[nodiscard]] i32 width() const override { return 0; }
    [[nodiscard]] i32 height() const override { return 0; }
    [[nodiscard]] f32 getTextWidth(const String& text) const override { return static_cast<f32>(text.size()) * 6.0f; }
    [[nodiscard]] u32 getFontHeight() const override { return 12; }

    bool filledRectCalled = false;
    bool textCalled = false;
    Rect lastFilledRect{};
    u32 lastFilledColor = 0;
    String lastText;
    f32 lastTextX = 0.0f;
    f32 lastTextY = 0.0f;
    u32 lastTextColor = 0;
};

} // namespace

TEST(ButtonWidgetPaintTest, PaintDrawsBackgroundBorderAndCenteredText) {
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    ButtonWidget button("btn_play", 10, 20, 100, 40, "Play");

    button.paint(ctx);

    EXPECT_TRUE(canvas.filledRectCalled);
    EXPECT_TRUE(canvas.textCalled);
    EXPECT_EQ("Play", canvas.lastText);
    EXPECT_EQ(10, canvas.lastFilledRect.x);
    EXPECT_EQ(20, canvas.lastFilledRect.y);
    EXPECT_EQ(100, canvas.lastFilledRect.width);
    EXPECT_EQ(40, canvas.lastFilledRect.height);
    EXPECT_EQ(60.0f, canvas.lastTextX);
    EXPECT_EQ(40.0f, canvas.lastTextY);
}

