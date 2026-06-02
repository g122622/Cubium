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

#pragma once

#include "client/renderer/map/MapRenderer.hpp"
#include "common/screen/IScreen.hpp"
#include "common/world/map/MapData.hpp"
#include "core/Types.hpp"

namespace mc::client {

namespace renderer::trident::gui {
class GuiRenderer;
}

/**
 * @brief 地图查看屏幕
 *
 * 当玩家使用已填充地图物品时打开，全屏显示地图内容。
 * 显示128x128像素的地图颜色数据和装饰图标。
 */
class MapScreen : public IScreen {
public:
    /**
     * @brief 构造函数
     * @param mapId 要显示的地图ID
     */
    explicit MapScreen(i32 mapId);

    void init() override;
    void render(i32 mouseX, i32 mouseY, f32 partialTick) override;
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override;
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;
    void onClose() override;

    [[nodiscard]] bool isPauseScreen() const override { return false; }

    /**
     * @brief 设置GUI渲染器
     */
    void setGuiRenderer(renderer::trident::gui::GuiRenderer* gui) { m_gui = gui; }
    /**
     * @brief 设置地图渲染器
     */
    void setMapRenderer(MapRenderer* mapRenderer) { m_mapRenderer = mapRenderer; }

    /**
     * @brief 设置地图数据缓存
     */
    void setMapDataCache(ClientMapDataCache* cache) { m_mapDataCache = cache; }

    /**
     * @brief 设置屏幕尺寸
     */
    void setScreenSize(i32 width, i32 height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    /**
     * @brief 获取当前显示的地图ID
     */
    [[nodiscard]] i32 mapId() const { return m_mapId; }

private:
    i32 m_mapId;
    i32 m_screenWidth = 0;
    i32 m_screenHeight = 0;

    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
    MapRenderer* m_mapRenderer = nullptr;
    ClientMapDataCache* m_mapDataCache = nullptr;
};

} // namespace mc::client
