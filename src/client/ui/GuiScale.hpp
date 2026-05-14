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

namespace mc::client::ui {

/**
 * @brief GUI 缩放计算结果
 *
 * scaleFactor 表示最终用于渲染和输入换算的缩放倍率。
 * width 和 height 表示缩放后的逻辑 GUI 分辨率。
 */
struct GuiScaleState {
    i32 scaleFactor = 1;
    i32 width = 0;
    i32 height = 0;
};

/**
 * @brief 计算 GUI 缩放状态
 *
 * 规则接近 Minecraft 1.21：
 * - 0 表示自动缩放
 * - 1 到 4 表示手动指定缩放
 * - 实际缩放不会让逻辑分辨率低于 320x240
 * - 结果会被限制在 1 到 4 之间
 *
 * @param requestedScale 用户设置的 GUI 缩放值
 * @param windowWidth 窗口宽度
 * @param windowHeight 窗口高度
 * @return GUI 缩放状态
 */
[[nodiscard]] GuiScaleState calculateGuiScale(i32 requestedScale, i32 windowWidth, i32 windowHeight);

} // namespace mc::client::ui