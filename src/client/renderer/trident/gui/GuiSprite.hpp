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
#include <string>

namespace mc::client::renderer::trident::gui {

/**
 * @brief 九宫格拉伸区域定义
 *
 * 用于定义纹理的不可拉伸边距。当按钮等UI元素被拉伸时，
 * 边角区域保持原样，只有中心区域会被拉伸。
 *
 * 坐标系：左上角为原点
 *
 * @example
 * // 对于一个 200x20 的按钮纹理，边距各为 4 像素
 * GuiNinePatch ninePatch{4, 4, 196, 16}; // left=4, top=4, right=196, bottom=16
 */
struct GuiNinePatch {
    i32 left = 0;   ///< 左边不可拉伸区域宽度（像素）
    i32 top = 0;    ///< 上边不可拉伸区域高度（像素）
    i32 right = 0;  ///< 右边不可拉伸区域右边缘X坐标（像素，距左边缘的距离）
    i32 bottom = 0; ///< 下边不可拉伸区域下边缘Y坐标（像素，距上边缘的距离）

    /**
     * @brief 检查九宫格是否有效
     * @return 如果所有值都为0（即不使用九宫格）返回false
     */
    [[nodiscard]] bool isValid() const { return left > 0 || top > 0 || right > 0 || bottom > 0; }

    /**
     * @brief 获取中心区域宽度
     * @param textureWidth 纹理总宽度
     * @return 中心可拉伸区域宽度
     */
    [[nodiscard]] i32 centerWidth(i32 textureWidth) const { return textureWidth - left - (textureWidth - right); }

    /**
     * @brief 获取中心区域高度
     * @param textureHeight 纹理总高度
     * @return 中心可拉伸区域高度
     */
    [[nodiscard]] i32 centerHeight(i32 textureHeight) const { return textureHeight - top - (textureHeight - bottom); }
};

/**
 * @brief GUI精灵定义
 *
 * 表示GUI纹理图集中的单个精灵区域。
 * 包含UV坐标、像素尺寸和可选的九宫格拉伸信息。
 *
 * @note UV坐标是归一化的 [0, 1] 范围
 *
 * @example
 * // widgets.png 中的按钮精灵
 * GuiSprite buttonNormal;
 * buttonNormal.id = "button_normal";
 * buttonNormal.u0 = 0.0f / 256.0f;   // 左边缘
 * buttonNormal.v0 = 66.0f / 256.0f;  // 上边缘
 * buttonNormal.u1 = 200.0f / 256.0f; // 右边缘
 * buttonNormal.v1 = 86.0f / 256.0f;  // 下边缘
 * buttonNormal.width = 200;
 * buttonNormal.height = 20;
 */
struct GuiSprite {
    std::string id; ///< 精灵ID，如 "button_normal"

    // UV坐标（归一化）
    f64 u0 = 0.0; ///< 左边缘U坐标 [0, 1]
    f64 v0 = 0.0; ///< 上边缘V坐标 [0, 1]
    f64 u1 = 1.0; ///< 右边缘U坐标 [0, 1]
    f64 v1 = 1.0; ///< 下边缘V坐标 [0, 1]

    // 像素尺寸
    i32 width = 0;  ///< 精灵原始宽度（像素）
    i32 height = 0; ///< 精灵原始高度（像素）

    // 九宫格拉伸（可选）
    GuiNinePatch ninePatch; ///< 九宫格拉伸定义

    // 状态变体精灵ID（可选，用于按钮等）
    std::string hoverSprite;    ///< 悬停状态精灵ID
    std::string disabledSprite; ///< 禁用状态精灵ID

    /**
     * @brief 默认构造函数
     */
    GuiSprite() = default;

    /**
     * @brief 构造精灵
     * @param spriteId 精灵ID
     * @param x 纹理中的X坐标（像素）
     * @param y 纹理中的Y坐标（像素）
     * @param w 精灵宽度（像素）
     * @param h 精灵高度（像素）
     * @param atlasWidth 图集总宽度（像素）
     * @param atlasHeight 图集总高度（像素）
     */
    GuiSprite(const std::string& spriteId, i32 x, i32 y, i32 w, i32 h, i32 atlasWidth, i32 atlasHeight)
        : id(spriteId)
        , u0(static_cast<f64>(x) / static_cast<f64>(atlasWidth))
        , v0(static_cast<f64>(y) / static_cast<f64>(atlasHeight))
        , u1(static_cast<f64>(x + w) / static_cast<f64>(atlasWidth))
        , v1(static_cast<f64>(y + h) / static_cast<f64>(atlasHeight))
        , width(w)
        , height(h)
    {}

    /**
     * @brief 检查精灵是否有效
     * @return 如果精灵有有效尺寸返回true
     */
    [[nodiscard]] bool isValid() const { return width > 0 && height > 0; }

    /**
     * @brief 设置九宫格拉伸
     * @param left 左边距
     * @param top 上边距
     * @param right 右边缘坐标
     * @param bottom 下边缘坐标
     * @return 自身引用，用于链式调用
     */
    GuiSprite& setNinePatch(i32 left, i32 top, i32 right, i32 bottom)
    {
        ninePatch = {left, top, right, bottom};
        return *this;
    }

    /**
     * @brief 设置状态变体精灵
     * @param hover 悬停状态精灵ID
     * @param disabled 禁用状态精灵ID
     * @return 自身引用，用于链式调用
     */
    GuiSprite& setStateSprites(const std::string& hover, const std::string& disabled)
    {
        hoverSprite = hover;
        disabledSprite = disabled;
        return *this;
    }
};

} // namespace mc::client::renderer::trident::gui
