#pragma once

#include "../../kagero/paint/PaintContext.hpp"
#include "../../kagero/widget/Widget.hpp"

namespace mc::client::ui::minecraft {

class HungerBarWidget : public kagero::widget::Widget {
public:
    HungerBarWidget();

    void setHunger(i32 hunger);
    [[nodiscard]] i32 hunger() const;

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    i32 m_hunger = 20;
};

} // namespace mc::client::ui::minecraft
