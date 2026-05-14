#pragma once

#include "../../kagero/paint/PaintContext.hpp"
#include "../../kagero/widget/Widget.hpp"

namespace mc::client::ui::minecraft {

class ExperienceBar : public kagero::widget::Widget {
public:
    ExperienceBar();

    void setProgress(f32 progress);
    [[nodiscard]] f32 progress() const;

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    f32 m_progress = 0.0f;
};

} // namespace mc::client::ui::minecraft
