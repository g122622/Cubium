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

#pragma once

#include "client/renderer/map/MapRenderer.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/world/ClientMapDataCache.hpp"
#include "core/Types.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief 地图查看屏幕（kagero 体系）
 *
 * 玩家使用已填充地图物品时打开，全屏居中显示 128×128 地图颜色数据与装饰图标。
 * 继承 Screen（无容器槽位），背景半透明遮罩 + 居中正方形地图 + 边框。
 *
 * 地图渲染依赖 MapRenderer 与 ClientMapDataCache，二者经 setter 注入；未注入时
 * 只绘制占位矩形（与 CartographyScreen 地图预览相同的 null 安全策略）。当前
 * MapRenderer / ClientMapDataCache 在客户端尚无 owner 注入点，地图实际不渲染，
 * 待后续接入地图数据包缓存后生效。
 */
class MapScreen : public Screen {
public:
    /**
     * @brief 构造函数
     * @param mapId 要显示的地图 ID
     */
    explicit MapScreen(i32 mapId);

    /** @brief 设置地图渲染器（未注入则不渲染地图内容） */
    void setMapRenderer(MapRenderer* mapRenderer) { m_mapRenderer = mapRenderer; }

    /** @brief 设置地图数据缓存（未注入则不渲染地图内容） */
    void setMapDataCache(ClientMapDataCache* cache) { m_mapDataCache = cache; }

    /** @brief 设置屏幕尺寸（用于居中与铺满计算） */
    void setScreenSize(i32 width, i32 height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    /** @brief 获取当前显示的地图 ID */
    [[nodiscard]] i32 mapId() const { return m_mapId; }

    // ==================== 事件 ====================

    void paint(kagero::widget::PaintContext& ctx) override;
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    i32 m_mapId;
    i32 m_screenWidth = 0;
    i32 m_screenHeight = 0;

    MapRenderer* m_mapRenderer = nullptr;
    ClientMapDataCache* m_mapDataCache = nullptr;
};

} // namespace mc::client::ui::minecraft
