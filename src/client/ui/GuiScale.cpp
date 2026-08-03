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

#include "GuiScale.hpp"
#include "common/core/Types.hpp"

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
    const i32 targetScale =
        requestedScale <= 0 ? MAX_GUI_SCALE : std::clamp(requestedScale, MIN_GUI_SCALE, MAX_GUI_SCALE);

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