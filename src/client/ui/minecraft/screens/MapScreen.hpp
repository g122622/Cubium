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

#pragma once

#include "client/renderer/map/MapRenderer.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/world/ClientMapDataCache.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief 地图查看屏（玩家使用已填充地图时打开的全屏地图）
 *
 * 直接继承 Screen（无容器菜单、无槽位），在屏幕中央绘制指定 mapId 的地图内容
 * （含装饰）。MapRenderer 与 ClientMapDataCache 经 setter 注入；未注入则不绘制。
 *
 * 打开方式：纯客户端本地开屏（玩家右键手持已填充地图时，由 ClientApplicationInput
 * 调 ScreenManager::openScreen）。1.21.11 原版 MapScreen 也是客户端本地开，不走
 * 服务端 OpenScreen 下推。
 *
 * 关闭：Esc 键调 ScreenManager::closeScreen()。
 */
class MapScreen : public Screen {
public:
    /**
     * @brief 构造函数
     * @param mapId 要查看的地图 ID
     */
    explicit MapScreen(i32 mapId);

    /**
     * @brief 设置地图渲染器（未注入则不绘制地图内容）
     */
    void setMapRenderer(MapRenderer* renderer) { m_mapRenderer = renderer; }

    /**
     * @brief 设置客户端地图数据缓存（地图数据来源，用于绘制装饰）
     */
    void setMapDataCache(ClientMapDataCache* cache) { m_mapDataCache = cache; }

    /**
     * @brief 设置屏幕视口尺寸（由开屏方注入，paint 据此居中绘制地图）
     *
     * MapScreen 非容器屏，无 ContainerScreenBase 的尺寸机制；此方法把视口尺寸写入
     * Widget bounds，使基类 width()/height() 返回真实视口，供 paint 居中计算。
     */
    void setScreenSize(i32 width, i32 height) { setBounds(kagero::Rect{0, 0, width, height}); }

    /**
     * @brief 绘制地图内容
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 处理按键：Esc 关闭屏幕
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    i32 m_mapId;
    MapRenderer* m_mapRenderer = nullptr;
    ClientMapDataCache* m_mapDataCache = nullptr;
};

} // namespace mc::client::ui::minecraft
