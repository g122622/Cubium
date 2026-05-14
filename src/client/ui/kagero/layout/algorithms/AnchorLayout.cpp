#include "AnchorLayout.hpp"

namespace mc::client::ui::kagero::layout {

std::vector<LayoutResult> AnchorLayout::compute(
    const Rect& containerBounds, const std::vector<WidgetLayoutAdaptor*>& children) const
{
    std::vector<LayoutResult> results;
    results.reserve(children.size());

    for (const auto* child : children) {
        if (child == nullptr || !child->constraints().enabled) {
            results.emplace_back(containerBounds.x, containerBounds.y, 0, 0);
            continue;
        }

        results.emplace_back(computeChildBounds(containerBounds, *child));
    }

    return results;
}

Rect AnchorLayout::computeChildBounds(const Rect& containerBounds, const WidgetLayoutAdaptor& child) const
{
    const auto& c = child.constraints();
    const auto& anchor = c.anchor;

    i32 width = c.preferredWidth > 0 ? c.preferredWidth : std::max(1, child.currentSize().width);
    i32 height = c.preferredHeight > 0 ? c.preferredHeight : std::max(1, child.currentSize().height);

    i32 x = containerBounds.x;
    i32 y = containerBounds.y;

    if (anchor.isStretchHorizontal()) {
        x = containerBounds.x + anchor.left.value_or(0);
        width = std::max(1, containerBounds.width - anchor.left.value_or(0) - anchor.right.value_or(0));
    } else if (anchor.centerX) {
        x = containerBounds.x + (containerBounds.width - width) / 2;
    } else if (anchor.left.has_value()) {
        x = containerBounds.x + anchor.left.value();
    } else if (anchor.right.has_value()) {
        x = containerBounds.right() - anchor.right.value() - width;
    }

    if (anchor.isStretchVertical()) {
        y = containerBounds.y + anchor.top.value_or(0);
        height = std::max(1, containerBounds.height - anchor.top.value_or(0) - anchor.bottom.value_or(0));
    } else if (anchor.centerY) {
        y = containerBounds.y + (containerBounds.height - height) / 2;
    } else if (anchor.top.has_value()) {
        y = containerBounds.y + anchor.top.value();
    } else if (anchor.bottom.has_value()) {
        y = containerBounds.bottom() - anchor.bottom.value() - height;
    }

    if (anchor.leftPercent.has_value()) {
        x = containerBounds.x + static_cast<i32>(containerBounds.width * anchor.leftPercent.value());
    }
    if (anchor.topPercent.has_value()) {
        y = containerBounds.y + static_cast<i32>(containerBounds.height * anchor.topPercent.value());
    }

    x += anchor.offsetX;
    y += anchor.offsetY;

    return Rect{x, y, width, height};
}

} // namespace mc::client::ui::kagero::layout
