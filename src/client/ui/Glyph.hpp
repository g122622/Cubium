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

#include "common/core/Types.hpp"
#include <array>
#include <cstddef>

namespace mc::client {

#ifdef __APPLE__
using GuiVertexScalar = f32;
#else
using GuiVertexScalar = f64;
#endif

/**
 * @brief 字形数据结构
 *
 * 存储单个字符的渲染信息，包括纹理坐标和度量数据。
 */
struct Glyph {
    u32 codepoint = 0;        // Unicode码点
    f32 u0 = 0.0f, v0 = 0.0f; // 纹理左上角UV
    f32 u1 = 0.0f, v1 = 0.0f; // 纹理右下角UV
    f32 advance = 0.0f;       // 字符宽度（光标前进距离）
    f32 bearingX = 0.0f;      // 字形左侧到光标的水平偏移
    f32 bearingY = 0.0f;      // 字形顶部到基线的垂直偏移
    f32 width = 0.0f;         // 字形像素宽度
    f32 height = 0.0f;        // 字形像素高度

    Glyph() = default;

    /**
     * @brief 构造字形
     * @param cp Unicode码点
     * @param u0_ 纹理左上角U
     * @param v0_ 纹理左上角V
     * @param u1_ 纹理右下角U
     * @param v1_ 纹理右下角V
     * @param adv 前进宽度
     * @param bx 水平偏移
     * @param by 垂直偏移
     * @param w 字形宽度
     * @param h 字形高度
     */
    Glyph(u32 cp, f32 u0_, f32 v0_, f32 u1_, f32 v1_, f32 adv, f32 bx, f32 by, f32 w, f32 h)
        : codepoint(cp)
        , u0(u0_)
        , v0(v0_)
        , u1(u1_)
        , v1(v1_)
        , advance(adv)
        , bearingX(bx)
        , bearingY(by)
        , width(w)
        , height(h)
    {}

    /**
     * @brief 获取粗体偏移量
     */
    [[nodiscard]] static constexpr f32 getBoldOffset() { return 1.0f; }

    /**
     * @brief 获取阴影偏移量
     */
    [[nodiscard]] static constexpr f32 getShadowOffset() { return 1.0f; }
};

/**
 * @brief 空字形（用于空格等不可见字符）
 */
struct EmptyGlyph : public Glyph {
    EmptyGlyph()
        : Glyph(' ', 0, 0, 0, 0, 4.0f, 0, 0, 0, 0)
    {}
};

/**
 * @brief 字形顶点 (用于UI渲染)
 *
 * 2D屏幕空间顶点，包含位置、UV、颜色和图集槽位。
 *
 * ## 图集槽位说明
 *
 * 槽位ID用于在片段着色器中选择正确的纹理采样器：
 * - 槽位 0: 字体纹理
 * - 槽位 1: 物品纹理图集
 * - 槽位 2+: GUI纹理图集（icons、widgets等）
 *
 * 通过在顶点数据中嵌入槽位ID，单个绘制调用可以使用多个纹理图集。
 * 片段着色器根据槽位ID选择对应的采样器进行纹理采样。
 */
struct GuiVertex {
    GuiVertexScalar x, y; ///< 屏幕坐标 (像素)
    GuiVertexScalar u, v; ///< 纹理坐标
    u32 color;            ///< ARGB颜色
    u8 atlasSlot;         ///< 图集槽位ID (0-15)
    u8 padding[3];        ///< 对齐填充

    GuiVertex()
        : x(0)
        , y(0)
        , u(0)
        , v(0)
        , color(0xFFFFFFFF)
        , atlasSlot(0)
        , padding{0, 0, 0}
    {}

    /**
     * @brief 构造顶点（使用默认槽位0）
     * @param px X坐标
     * @param py Y坐标
     * @param pu U纹理坐标
     * @param pv V纹理坐标
     * @param col ARGB颜色
     * @param slot 图集槽位ID (默认0)
     */
    GuiVertex(f64 px, f64 py, f64 pu, f64 pv, u32 col, u8 slot = 0)
        : x(static_cast<GuiVertexScalar>(px))
        , y(static_cast<GuiVertexScalar>(py))
        , u(static_cast<GuiVertexScalar>(pu))
        , v(static_cast<GuiVertexScalar>(pv))
        , color(col)
        , atlasSlot(slot)
        , padding{0, 0, 0}
    {}

    /**
     * @brief 计算顶点结构大小
     * @return 顶点字节数
     */
    [[nodiscard]] static constexpr size_t stride() { return sizeof(GuiVertex); }
};

/**
 * @brief GUI矩形 (用于绘制填充矩形)
 */
struct GuiRect {
    f32 x, y; // 左上角
    f32 width, height;
    u32 color; // ARGB颜色

    GuiRect() = default;
    GuiRect(f32 px, f32 py, f32 w, f32 h, u32 col = 0xFFFFFFFF)
        : x(px)
        , y(py)
        , width(w)
        , height(h)
        , color(col)
    {}
};

/**
 * @brief 文本样式
 */
struct TextStyle {
    u32 color = 0xFFFFFFFF;     // 文本颜色
    bool shadow = true;         // 是否绘制阴影
    bool bold = false;          // 粗体
    bool italic = false;        // 斜体
    bool strikethrough = false; // 删除线
    bool underline = false;     // 下划线
    bool obfuscated = false;    // 混淆（§k，随机替换字符）

    TextStyle() = default;
    explicit TextStyle(u32 col, bool sh = true)
        : color(col)
        , shadow(sh)
    {}
};

/**
 * @brief 颜色常量
 */
namespace Colors {
constexpr u32 WHITE = 0xFFFFFFFF;
constexpr u32 BLACK = 0xFF000000;
constexpr u32 RED = 0xFFFF0000;
constexpr u32 GREEN = 0xFF00FF00;
constexpr u32 BLUE = 0xFF0000FF;
constexpr u32 YELLOW = 0xFFFFFF00;
constexpr u32 CYAN = 0xFF00FFFF;
constexpr u32 MAGENTA = 0xFFFF00FF;
constexpr u32 GRAY = 0xFF808080;
constexpr u32 DARK_GRAY = 0xFF404040;
constexpr u32 LIGHT_GRAY = 0xFFC0C0C0;
constexpr u32 ORANGE = 0xFFFFA500;

// 聊天颜色
constexpr u32 MC_BLACK = 0xFF000000;
constexpr u32 MC_DARK_BLUE = 0xFF0000AA;
constexpr u32 MC_DARK_GREEN = 0xFF00AA00;
constexpr u32 MC_DARK_AQUA = 0xFF00AAAA;
constexpr u32 MC_DARK_RED = 0xFFAA0000;
constexpr u32 MC_DARK_PURPLE = 0xFFAA00AA;
constexpr u32 MC_GOLD = 0xFFFFAA00;
constexpr u32 MC_GRAY = 0xFFAAAAAA;
constexpr u32 MC_DARK_GRAY = 0xFF555555;
constexpr u32 MC_BLUE = 0xFF5555FF;
constexpr u32 MC_GREEN = 0xFF55FF55;
constexpr u32 MC_AQUA = 0xFF55FFFF;
constexpr u32 MC_RED = 0xFFFF5555;
constexpr u32 MC_LIGHT_PURPLE = 0xFFFF55FF;
constexpr u32 MC_YELLOW = 0xFFFFFF55;
constexpr u32 MC_WHITE = 0xFFFFFFFF;

/**
 * @brief 从ARGB分量创建颜色
 */
[[nodiscard]] constexpr u32 fromARGB(u8 a, u8 r, u8 g, u8 b)
{
    return (static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) |
        (static_cast<u32>(b));
}

/**
 * @brief 从RGB分量创建颜色 (Alpha默认255)
 */
[[nodiscard]] constexpr u32 fromRGB(u8 r, u8 g, u8 b)
{
    return fromARGB(255, r, g, b);
}

/**
 * @brief 获取颜色的Alpha分量
 */
[[nodiscard]] constexpr u8 getAlpha(u32 color)
{
    return static_cast<u8>((color >> 24) & 0xFF);
}

/**
 * @brief 获取颜色的红色分量
 */
[[nodiscard]] constexpr u8 getRed(u32 color)
{
    return static_cast<u8>((color >> 16) & 0xFF);
}

/**
 * @brief 获取颜色的绿色分量
 */
[[nodiscard]] constexpr u8 getGreen(u32 color)
{
    return static_cast<u8>((color >> 8) & 0xFF);
}

/**
 * @brief 获取颜色的蓝色分量
 */
[[nodiscard]] constexpr u8 getBlue(u32 color)
{
    return static_cast<u8>(color & 0xFF);
}
} // namespace Colors

} // namespace mc::client
