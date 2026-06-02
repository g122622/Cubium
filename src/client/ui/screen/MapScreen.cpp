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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MapScreen.hpp"

#include <GLFW/glfw3.h>

#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/world/ClientMapDataCache.hpp"
#include "common/util/assert/AssertMacros.hpp"

namespace mc::client {

MapScreen::MapScreen(i32 mapId)
    : m_mapId(mapId)
{}

void MapScreen::init()
{
    // 无需特殊初始化
}

void MapScreen::render(i32 mouseX, i32 mouseY, f32 partialTick)
{
    MC_UNUSED(mouseX);
    MC_UNUSED(mouseY);
    MC_UNUSED(partialTick);

    if (m_gui == nullptr) {
        return;
    }

    m_gui->beginFrame(static_cast<f32>(m_screenWidth), static_cast<f32>(m_screenHeight));

    // 绘制半透明背景
    m_gui->fillRect(0.0, 0.0, static_cast<f64>(m_screenWidth), static_cast<f64>(m_screenHeight), 0x80000000);

    // 计算地图显示尺寸（居中，正方形，适应屏幕）
    constexpr f64 MAP_ASPECT = 1.0; // 地图是正方形
    const f64 padding = 40.0;
    const f64 maxWidth = static_cast<f64>(m_screenWidth) - padding * 2.0;
    const f64 maxHeight = static_cast<f64>(m_screenHeight) - padding * 2.0;
    const f64 mapSize = std::min(maxWidth, maxHeight * MAP_ASPECT);

    const f64 mapX = (static_cast<f64>(m_screenWidth) - mapSize) / 2.0;
    const f64 mapY = (static_cast<f64>(m_screenHeight) - mapSize) / 2.0;

    // 更新地图纹理并渲染
    if (m_mapRenderer != nullptr && m_mapDataCache != nullptr) {
        world::map::MapData* mapData = m_mapDataCache->getMapData(m_mapId);
        if (mapData != nullptr) {
            m_mapRenderer->updateMapTexture(m_mapId, *mapData);
            m_mapRenderer->renderMap(m_mapId, mapX, mapY, mapSize, true, mapData);
        } else {
            // 没有地图数据，显示空白地图背景
            m_gui->fillRect(mapX, mapY, mapSize, mapSize, 0xFFF0F0F0);
        }
    } else {
        // 没有渲染器，显示占位
        m_gui->fillRect(mapX, mapY, mapSize, mapSize, 0xFFF0F0F0);
    }

    // 绘制地图边框
    constexpr f64 BORDER = 2.0;
    m_gui->drawRect(mapX - BORDER, mapY - BORDER, mapSize + BORDER * 2.0, mapSize + BORDER * 2.0, 0xFF555555);
}

bool MapScreen::onClick(i32 mouseX, i32 mouseY, i32 button)
{
    MC_UNUSED(mouseX);
    MC_UNUSED(mouseY);
    MC_UNUSED(button);
    return true;
}

bool MapScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    MC_UNUSED(scanCode);
    MC_UNUSED(mods);

    // ESC或E键关闭地图屏幕
    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_E) && action == GLFW_PRESS) {
        onClose();
        return true;
    }
    return false;
}

void MapScreen::onClose()
{
    // 屏幕关闭时的回调
}

} // namespace mc::client
