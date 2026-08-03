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

#include "MapScreen.hpp"

#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/core/Types.hpp"
#include "common/world/map/MapData.hpp"

#include <algorithm>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

MapScreen::MapScreen(i32 mapId)
    : Screen("map")
    , m_mapId(mapId)
{
    setModal(true);
    setPauseScreen(false);
}

void MapScreen::paint(kagero::widget::PaintContext& ctx)
{
    (void)ctx; // 地图内容由 MapRenderer 经 GuiRenderer 直接绘制，不走 PaintContext

    if (m_mapRenderer == nullptr) {
        return;
    }

    // 居中绘制，尺寸取屏幕短边（全屏地图查看）
    const f64 screenWidth = static_cast<f64>(width());
    const f64 screenHeight = static_cast<f64>(height());
    const f64 mapSize = std::min(screenWidth, screenHeight);
    const f64 mapX = (screenWidth - mapSize) / 2.0;
    const f64 mapY = (screenHeight - mapSize) / 2.0;

    // 装饰绘制需要 MapData；缓存未就绪则只画地图像素不画装饰
    const mc::world::map::MapData* mapData =
        (m_mapDataCache != nullptr) ? m_mapDataCache->getMapData(m_mapId) : nullptr;

    m_mapRenderer->renderMap(m_mapId, mapX, mapY, mapSize, true, mapData);
}

bool MapScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;
    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        ScreenManager::instance().closeScreen();
        return true;
    }
    return false;
}

} // namespace mc::client::ui::minecraft
