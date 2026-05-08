#include <gtest/gtest.h>
#include "client/ui/kagero/layout/algorithms/AnchorLayout.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using namespace mc::client::ui::kagero::widget;

class AnchorTestWidget : public Widget {
public:
    explicit AnchorTestWidget(const mc::std::string& id) : Widget(id) {
        setSize(20, 10);
    }
    void paint(PaintContext& ctx) override {
        (void)ctx;
    }
};

TEST(AnchorLayoutTest, LeftTopAnchor) {
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
