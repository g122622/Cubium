#pragma once

#include "TargetInfo.hpp"

#include "../../kagero/paint/PaintContext.hpp"
#include "../../kagero/widget/Widget.hpp"

namespace mc::client::ui::minecraft::targetinfo {

class TargetInfoWidget : public kagero::widget::Widget {
public:
    TargetInfoWidget();
    ~TargetInfoWidget() override = default;

    void setTargetInfo(TargetInfoSnapshot targetInfo);

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    TargetInfoSnapshot m_targetInfo;
};

} // namespace mc::client::ui::minecraft::targetinfo