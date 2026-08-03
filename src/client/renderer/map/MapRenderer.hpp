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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDecoration.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <array>
#include <unordered_map>
#include <vector>

namespace mc::client {

namespace renderer::trident::gui {
class GuiRenderer;
}

/**
 * @brief 地图渲染器
 *
 * 将MapData的颜色数据转换为RGBA纹理，供GuiRenderer绘制。
 * 管理多张地图纹理缓存，按需更新脏区域。
 *
 * 使用方式：
 * 1. 调用updateMapTexture()更新指定地图的纹理数据
 * 2. 调用renderMap()在指定位置绘制地图
 * 3. 调用renderDecorations()在地图上绘制装饰图标
 *
 * 颜色计算：
 * - MapData中每个像素是u8，编码为 colorIndex * 4 + shadeIndex
 * - colorIndex=0表示透明（AIR）
 * - 通过MaterialColor::pixelToArgb()转换为ARGB颜色
 */
class MapRenderer {
public:
    /** 地图像素尺寸 */
    static constexpr i32 MAP_SIZE = world::map::MapData::MAP_SIZE;

    /** 纹理尺寸（RGBA） */
    static constexpr i32 TEXTURE_SIZE = MAP_SIZE * MAP_SIZE * 4;

    MapRenderer();
    ~MapRenderer() = default;

    // 禁止拷贝
    MapRenderer(const MapRenderer&) = delete;
    MapRenderer& operator=(const MapRenderer&) = delete;

    /**
     * @brief 更新指定地图的纹理数据
     *
     * 比较缓存的像素与MapData中的像素，仅更新变化部分。
     *
     * @param mapId 地图ID
     * @param mapData 地图数据
     * @return true 如果纹理有变化
     */
    bool updateMapTexture(i32 mapId, const world::map::MapData& mapData);

    /**
     * @brief 获取指定地图的RGBA纹理数据
     *
     * @param mapId 地图ID
     * @return RGBA数据指针，如果不存在返回nullptr
     */
    [[nodiscard]] const std::array<u8, TEXTURE_SIZE>* getTextureData(i32 mapId) const;

    /**
     * @brief 在GuiRenderer上绘制地图
     *
     * @param mapId 地图ID
     * @param screenX 屏幕左上角X坐标
     * @param screenY 屏幕左上角Y坐标
     * @param size 绘制尺寸（正方形）
     * @param showDecorations 是否显示装饰
     * @param mapData 地图数据（用于获取装饰信息），可为nullptr
     */
    void renderMap(
        i32 mapId, f64 screenX, f64 screenY, f64 size, bool showDecorations, const world::map::MapData* mapData);

    /**
     * @brief 在地图上绘制装饰图标
     *
     * @param screenX 地图左上角X坐标
     * @param screenY 地图左上角Y坐标
     * @param size 地图绘制尺寸
     * @param mapData 地图数据
     */
    void renderDecorations(f64 screenX, f64 screenY, f64 size, const world::map::MapData& mapData);

    /**
     * @brief 移除指定地图的缓存
     */
    void removeMap(i32 mapId);

    /**
     * @brief 清除所有缓存
     */
    void clear();

    /**
     * @brief 设置GUI渲染器（用于drawText等）
     */
    void setGuiRenderer(renderer::trident::gui::GuiRenderer* gui) { m_gui = gui; }

private:
    /**
     * @brief 地图纹理缓存条目
     */
    struct MapTextureEntry {
        /** RGBA纹理数据 (MAP_SIZE * MAP_SIZE * 4) */
        std::array<u8, TEXTURE_SIZE> textureData{};
        /** 上次更新时的颜色数据（用于脏检查） */
        std::array<u8, MAP_SIZE * MAP_SIZE> cachedColors{};
        /** 是否需要上传纹理 */
        bool dirty = true;
    };

    /**
     * @brief 获取或创建纹理缓存条目
     */
    MapTextureEntry& _getOrCreateEntry(i32 mapId);

    /**
     * @brief 将MapData像素转换为RGBA纹理数据
     */
    static void _convertMapToTexture(
        const std::array<u8, MAP_SIZE * MAP_SIZE>& colors, std::array<u8, TEXTURE_SIZE>& outTexture);

    /**
     * @brief 获取装饰类型的颜色（用于简单图标绘制）
     */
    [[nodiscard]] static u32 _getDecorationColor(world::map::DecorationType type);

    /** 纹理缓存 */
    std::unordered_map<i32, MapTextureEntry> m_textures;

    /** GUI渲染器 */
    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
};

} // namespace mc::client
