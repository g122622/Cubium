/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to furnished do so, subject to the following conditions:
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

#include "MapScreen.hpp"

#include "client/ui/Glyph.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/world/map/MapData.hpp"
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

namespace {

// 地图显示边距（屏幕四边留白）
constexpr f64 MAP_PADDING = 40.0;
// 地图边框宽度
constexpr f64 MAP_BORDER = 2.0;

} // namespace

MapScreen::MapScreen(i32 mapId)
    : Screen("map")
    , m_mapId(mapId)
{
    setModal(true);
    setPauseScreen(false);
}

void MapScreen::paint(kagero::widget::PaintContext& ctx)
{
    if (m_screenWidth <= 0 || m_screenHeight <= 0) {
        return;
    }

    const i32 screenW = m_screenWidth;
    const i32 screenH = m_screenHeight;

    // 半透明背景遮罩
    ctx.drawFilledRect(kagero::Rect(0, 0, screenW, screenH), Colors::fromARGB(128, 0, 0, 0));

    // 居中正方形地图尺寸（适应屏幕，正方形）
    const f64 maxWidth = static_cast<f64>(screenW) - MAP_PADDING * 2.0;
    const f64 maxHeight = static_cast<f64>(screenH) - MAP_PADDING * 2.0;
    const f64 mapSize = std::min(maxWidth, maxHeight);
    const f64 mapX = (static_cast<f64>(screenW) - mapSize) / 2.0;
    const f64 mapY = (static_cast<f64>(screenH) - mapSize) / 2.0;

    // 更新地图纹理并渲染（MapRenderer / ClientMapDataCache 未注入时只画占位）
    if (m_mapRenderer != nullptr && m_mapDataCache != nullptr) {
        world::map::MapData* mapData = m_mapDataCache->getMapData(m_mapId);
        if (mapData != nullptr) {
            m_mapRenderer->updateMapTexture(m_mapId, *mapData);
            m_mapRenderer->renderMap(m_mapId, mapX, mapY, mapSize, true, mapData);
        } else {
            ctx.drawFilledRect(kagero::Rect(static_cast<i32>(mapX),
                                   static_cast<i32>(mapY),
                                   static_cast<i32>(mapSize),
                                   static_cast<i32>(mapSize)),
                Colors::fromARGB(255, 240, 240, 240));
        }
    } else {
        ctx.drawFilledRect(
            kagero::Rect(
                static_cast<i32>(mapX), static_cast<i32>(mapY), static_cast<i32>(mapSize), static_cast<i32>(mapSize)),
            Colors::fromARGB(255, 240, 240, 240));
    }

    // 地图边框
    ctx.drawBorder(kagero::Rect(static_cast<i32>(mapX - MAP_BORDER),
                       static_cast<i32>(mapY - MAP_BORDER),
                       static_cast<i32>(mapSize + MAP_BORDER * 2.0),
                       static_cast<i32>(mapSize + MAP_BORDER * 2.0)),
        static_cast<f32>(MAP_BORDER),
        Colors::fromARGB(255, 85, 85, 85));
}

bool MapScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    // ESC 或 E 键关闭地图屏幕
    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_E) && action == GLFW_PRESS) {
        ScreenManager::instance().closeScreen();
        return true;
    }
    return false;
}

} // namespace mc::client::ui::minecraft
