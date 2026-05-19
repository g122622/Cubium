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

#include "client/ui/kagero/layout/algorithms/GridLayout.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using namespace mc::client::ui::kagero::widget;

class GridTestWidget : public Widget {
public:
    explicit GridTestWidget(const std::string& id)
        : Widget(id)
    {}
    void paint(PaintContext& ctx) override { (void)ctx; }
};

TEST(GridLayoutTest, BasicGridPlacement)
{
    GridLayout layout;
    layout.setColumns(2);
    layout.setColumnGap(10);
    layout.setRowGap(10);

    GridTestWidget w1("w1");
    GridTestWidget w2("w2");
    GridTestWidget w3("w3");

    WidgetLayoutAdaptor a1(&w1);
    WidgetLayoutAdaptor a2(&w2);
    WidgetLayoutAdaptor a3(&w3);

    std::vector<WidgetLayoutAdaptor*> children{&a1, &a2, &a3};
    const auto result = layout.compute(Rect{0, 0, 210, 110}, children);

    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].bounds.x, 0);
    EXPECT_EQ(result[1].bounds.x, 110);
    EXPECT_EQ(result[2].bounds.y, 60);
}
