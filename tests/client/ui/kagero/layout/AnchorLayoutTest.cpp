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

#include "client/ui/kagero/layout/algorithms/AnchorLayout.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using namespace mc::client::ui::kagero::widget;

class AnchorTestWidget : public Widget {
public:
    explicit AnchorTestWidget(const std::string& id)
        : Widget(id)
    {
        setSize(20, 10);
    }
    void paint(PaintContext& ctx) override { (void)ctx; }
};

TEST(AnchorLayoutTest, LeftTopAnchor)
{
    AnchorLayout layout;

    AnchorTestWidget widget("w");
    WidgetLayoutAdaptor adaptor(&widget);
    adaptor.constraints().anchor.left = 10;
    adaptor.constraints().anchor.top = 5;

    std::vector<WidgetLayoutAdaptor*> children{&adaptor};
    const auto result = layout.compute(Rect{0, 0, 200, 100}, children);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].bounds.x, 10);
    EXPECT_EQ(result[0].bounds.y, 5);
}
