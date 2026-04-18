#include "GuiScale.hpp"

#include <algorithm>

namespace mc::client::ui {

namespace {

constexpr i32 MIN_GUI_WIDTH = 320;
constexpr i32 MIN_GUI_HEIGHT = 240;
constexpr i32 MIN_GUI_SCALE = 1;
constexpr i32 MAX_GUI_SCALE = 4;

} // namespace

GuiScaleState calculateGuiScale(i32 requestedScale, i32 windowWidth, i32 windowHeight)
{
    GuiScaleState state;

    if (windowWidth <= 0 || windowHeight <= 0) {
        return state;
    }

    i32 scaleFactor = MIN_GUI_SCALE;
    const i32 targetScale = requestedScale <= 0 ? MAX_GUI_SCALE : std::clamp(requestedScale, MIN_GUI_SCALE, MAX_GUI_SCALE);

    while (scaleFactor < targetScale) {
        const i32 nextScale = scaleFactor + 1;
        if (windowWidth / nextScale < MIN_GUI_WIDTH || windowHeight / nextScale < MIN_GUI_HEIGHT) {
            break;
        }
        scaleFactor = nextScale;
    }

    state.scaleFactor = scaleFactor;
    state.width = std::max(1, windowWidth / scaleFactor);
    state.height = std::max(1, windowHeight / scaleFactor);
    return state;
}

} // namespace mc::client::ui