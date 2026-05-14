#pragma once

#include "../core/LayoutResult.hpp"
#include "../integration/WidgetLayoutAdaptor.hpp"
#include <vector>

namespace mc::client::ui::kagero::layout {

/**
 * @brief Anchor布局算法
 */
class AnchorLayout {
public:
    [[nodiscard]] std::vector<LayoutResult> compute(
        const Rect& containerBounds, const std::vector<WidgetLayoutAdaptor*>& children) const;

private:
    [[nodiscard]] Rect computeChildBounds(const Rect& containerBounds, const WidgetLayoutAdaptor& child) const;
};

} // namespace mc::client::ui::kagero::layout
