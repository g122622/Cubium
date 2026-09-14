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
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc::world::gen::surface {

/**
 * @brief Y 坐标锚点类型
 *
 * MC 1.18+ 引入的垂直锚点系统，用于 SurfaceRules 中的 Y 条件判断。
 * 锚点可以指定为绝对 Y 坐标、相对于世界底部或相对于世界顶部。
 */
enum class VerticalAnchorType : u8 { Absolute, AboveBottom, BelowTop };

/**
 * @brief Y 坐标锚点（MC 1.18+）
 *
 * 表示一个相对于世界坐标系的 Y 位置，支持三种锚定方式：
 * - Absolute: 绝对 Y 坐标
 * - AboveBottom: 相对于世界底部的偏移
 * - BelowTop: 相对于世界顶部的偏移
 */
struct VerticalAnchor {
    VerticalAnchorType type;
    i32 value;

    /** 创建绝对 Y 坐标锚点 */
    static VerticalAnchor absolute(i32 y) { return {VerticalAnchorType::Absolute, y}; }

    /** 创建相对于世界底部的锚点（偏移量 >= 0） */
    static VerticalAnchor aboveBottom(i32 offset) { return {VerticalAnchorType::AboveBottom, offset}; }

    /** 创建相对于世界顶部的锚点（偏移量 >= 0） */
    static VerticalAnchor belowTop(i32 offset) { return {VerticalAnchorType::BelowTop, offset}; }

    /** 将锚点解析为世界 Y 坐标 */
    i32 resolveY(i32 minY, i32 height) const;
};

} // namespace mc::world::gen::surface
