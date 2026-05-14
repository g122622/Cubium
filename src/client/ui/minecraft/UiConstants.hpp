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

namespace mc::client::ui::minecraft {

/**
 * @brief UI 常量
 *
 * 菜单屏幕共用的 UI 常量定义。
 */
namespace UiConstants {

/// 标准按钮宽度
static constexpr i32 BUTTON_WIDTH = 200;

/// 标准按钮高度
static constexpr i32 BUTTON_HEIGHT = 20;

/// 按钮间距
static constexpr i32 BUTTON_SPACING = 4;

/// 标题 Y 偏移
static constexpr i32 TITLE_Y_OFFSET = 60;

/// 按钮 Y 起始位置
static constexpr i32 BUTTON_Y_START = 120;

/// 小按钮宽度
static constexpr i32 SMALL_BUTTON_WIDTH = 100;

/// 中等按钮宽度
static constexpr i32 MEDIUM_BUTTON_WIDTH = 150;

/// 表单标签宽度
static constexpr i32 LABEL_WIDTH = 120;

/// 表单输入框宽度
static constexpr i32 FIELD_WIDTH = 200;

/// 表单行高
static constexpr i32 ROW_HEIGHT = 30;

} // namespace UiConstants

} // namespace mc::client::ui::minecraft
