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

#include "MapRenderer.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDecoration.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <array>
#include <cstddef>

namespace mc::client {

// ============================================================================
// 构造函数
// ============================================================================

MapRenderer::MapRenderer() = default;

// ============================================================================
// 纹理更新
// ============================================================================

bool MapRenderer::updateMapTexture(i32 mapId, const world::map::MapData& mapData)
{
    MapTextureEntry& entry = _getOrCreateEntry(mapId);

    const auto& colors = mapData.colors();
    bool changed = false;

    // 检查是否有变化
    for (i32 i = 0; i < MAP_SIZE * MAP_SIZE; ++i) {
        if (colors[static_cast<size_t>(i)] != entry.cachedColors[static_cast<size_t>(i)]) {
            changed = true;
            break;
        }
    }

    if (changed) {
        // 更新缓存的颜色数据
        entry.cachedColors = colors;
        // 转换为RGBA纹理
        _convertMapToTexture(colors, entry.textureData);
        entry.dirty = true;
    }

    return changed;
}

const std::array<u8, MapRenderer::TEXTURE_SIZE>* MapRenderer::getTextureData(i32 mapId) const
{
    auto it = m_textures.find(mapId);
    if (it != m_textures.end()) {
        return &it->second.textureData;
    }
    return nullptr;
}

// ============================================================================
// 渲染
// ============================================================================

void MapRenderer::renderMap(
    i32 mapId, f64 screenX, f64 screenY, f64 size, bool showDecorations, const world::map::MapData* mapData)
{
    if (m_gui == nullptr) {
        return;
    }

    auto it = m_textures.find(mapId);
    if (it == m_textures.end()) {
        return;
    }

    const auto& entry = it->second;

    // 绘制地图背景边框
    constexpr f64 BORDER = 2.0;
    m_gui->fillRect(screenX - BORDER, screenY - BORDER, size + BORDER * 2.0, size + BORDER * 2.0, 0xFF555555);

    // 逐行绘制地图像素
    // 由于GuiRenderer没有直接像素缓冲区上传接口，使用fillRect逐像素绘制
    // 对于性能优化，可以考虑未来添加纹理上传API
    const f64 pixelSize = size / static_cast<f64>(MAP_SIZE);

    for (i32 y = 0; y < MAP_SIZE; ++y) {
        for (i32 x = 0; x < MAP_SIZE; ++x) {
            const size_t pixelIndex = static_cast<size_t>(y * MAP_SIZE + x);
            const size_t textureOffset = pixelIndex * 4;

            const u8 a = entry.textureData[textureOffset + 3];
            if (a == 0) {
                continue; // 透明像素不绘制
            }

            const u8 r = entry.textureData[textureOffset];
            const u8 g = entry.textureData[textureOffset + 1];
            const u8 b = entry.textureData[textureOffset + 2];
            const u32 color = (static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) |
                static_cast<u32>(b);

            const f64 px = screenX + static_cast<f64>(x) * pixelSize;
            const f64 py = screenY + static_cast<f64>(y) * pixelSize;

            m_gui->fillRect(px, py, pixelSize, pixelSize, color);
        }
    }

    // 绘制装饰图标
    if (showDecorations && mapData != nullptr) {
        renderDecorations(screenX, screenY, size, *mapData);
    }
}

void MapRenderer::renderDecorations(f64 screenX, f64 screenY, f64 size, const world::map::MapData& mapData)
{
    if (m_gui == nullptr) {
        return;
    }

    const f64 pixelSize = size / static_cast<f64>(MAP_SIZE);
    constexpr f64 ICON_SIZE = 8.0; // 图标基础像素大小

    for (const auto& [name, decoration] : mapData.decorations()) {
        const u32 iconColor = _getDecorationColor(decoration.type());
        if (iconColor == 0) {
            continue;
        }

        // 装饰位置：x/y 是 -128~127 的字节值，映射到地图像素坐标
        const f64 decoX = screenX + (static_cast<f64>(decoration.x()) + 128.0) / 256.0 * size;
        const f64 decoY = screenY + (static_cast<f64>(decoration.y()) + 128.0) / 256.0 * size;

        // 绘制简单的方向箭头图标
        // 对于玩家标记，绘制一个指向方向的三角形
        const f64 iconHalf = ICON_SIZE * pixelSize * 0.5;

        // 绘制圆形背景
        m_gui->fillRect(decoX - iconHalf, decoY - iconHalf, iconHalf * 2.0, iconHalf * 2.0, iconColor);

        // 绘制白色边框
        const u32 borderColor = 0xFFFFFFFF;
        constexpr f64 BORDER_W = 1.0;
        m_gui->fillRect(decoX - iconHalf - BORDER_W, decoY - iconHalf, BORDER_W, iconHalf * 2.0, borderColor);
        m_gui->fillRect(decoX + iconHalf, decoY - iconHalf, BORDER_W, iconHalf * 2.0, borderColor);
        m_gui->fillRect(decoX - iconHalf, decoY - iconHalf - BORDER_W, iconHalf * 2.0, BORDER_W, borderColor);
        m_gui->fillRect(decoX - iconHalf, decoY + iconHalf, iconHalf * 2.0, BORDER_W, borderColor);
    }
}

void MapRenderer::removeMap(i32 mapId)
{
    m_textures.erase(mapId);
}

void MapRenderer::clear()
{
    m_textures.clear();
}

// ============================================================================
// 内部方法
// ============================================================================

MapRenderer::MapTextureEntry& MapRenderer::_getOrCreateEntry(i32 mapId)
{
    auto it = m_textures.find(mapId);
    if (it == m_textures.end()) {
        it = m_textures.try_emplace(mapId).first;
    }
    return it->second;
}

void MapRenderer::_convertMapToTexture(
    const std::array<u8, MAP_SIZE * MAP_SIZE>& colors, std::array<u8, TEXTURE_SIZE>& outTexture)
{
    for (i32 i = 0; i < MAP_SIZE * MAP_SIZE; ++i) {
        const u8 pixelByte = colors[static_cast<size_t>(i)];
        const u32 argb = world::map::MaterialColor::pixelToArgb(pixelByte);

        const size_t offset = static_cast<size_t>(i) * 4;
        outTexture[offset] = static_cast<u8>((argb >> 16) & 0xFF);     // R
        outTexture[offset + 1] = static_cast<u8>((argb >> 8) & 0xFF);  // G
        outTexture[offset + 2] = static_cast<u8>(argb & 0xFF);         // B
        outTexture[offset + 3] = static_cast<u8>((argb >> 24) & 0xFF); // A
    }
}

u32 MapRenderer::_getDecorationColor(world::map::DecorationType type)
{
    // 根据装饰类型返回ARGB颜色
    switch (type) {
        case world::map::DecorationType::PLAYER:
        case world::map::DecorationType::PLAYER_OFF_MAP:
        case world::map::DecorationType::PLAYER_OFF_LIMITS:
            return 0xFFFFFFFF; // 白色
        case world::map::DecorationType::FRAME:
            return 0xFF80FF80; // 绿色
        case world::map::DecorationType::RED_MARKER:
        case world::map::DecorationType::RED_X:
            return 0xFFFF0000; // 红色
        case world::map::DecorationType::BLUE_MARKER:
            return 0xFF0000FF; // 蓝色
        case world::map::DecorationType::TARGET_X:
        case world::map::DecorationType::TARGET_POINT:
            return 0xFFFF5555; // 浅红色
        case world::map::DecorationType::MANSION:
            return 0xFF55FF55; // 深绿色
        case world::map::DecorationType::MONUMENT:
            return 0xFF5555FF; // 深蓝色
        case world::map::DecorationType::BANNER_WHITE:
            return 0xFFFFFFFF;
        case world::map::DecorationType::BANNER_ORANGE:
            return 0xFFFF8800;
        case world::map::DecorationType::BANNER_MAGENTA:
            return 0xFFFF55FF;
        case world::map::DecorationType::BANNER_LIGHT_BLUE:
            return 0xFF55FFFF;
        case world::map::DecorationType::BANNER_YELLOW:
            return 0xFFFFFF55;
        case world::map::DecorationType::BANNER_LIME:
            return 0xFF55FF55;
        case world::map::DecorationType::BANNER_PINK:
            return 0xFFFF88AA;
        case world::map::DecorationType::BANNER_GRAY:
            return 0xFF888888;
        case world::map::DecorationType::BANNER_LIGHT_GRAY:
            return 0xFFBBBBBB;
        case world::map::DecorationType::BANNER_CYAN:
            return 0xFF00AAAA;
        case world::map::DecorationType::BANNER_PURPLE:
            return 0xFFAA00AA;
        case world::map::DecorationType::BANNER_BLUE:
            return 0xFF0000AA;
        case world::map::DecorationType::BANNER_BROWN:
            return 0xFF885500;
        case world::map::DecorationType::BANNER_GREEN:
            return 0xFF00AA00;
        case world::map::DecorationType::BANNER_RED:
            return 0xFFAA0000;
        case world::map::DecorationType::BANNER_BLACK:
            return 0xFF222222;
        default:
            return 0;
    }
}

} // namespace mc::client
