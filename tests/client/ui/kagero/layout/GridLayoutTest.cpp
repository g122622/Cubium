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
