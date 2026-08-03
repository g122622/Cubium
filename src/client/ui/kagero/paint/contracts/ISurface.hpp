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

#include "ICanvas.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::kagero::paint {

/**
 * @brief 绘制表面接口
 *
 * 表示一个可绘制区域，持有画布对象并提供尺寸查询和刷新能力。
 * 典型实现包括窗口表面、离屏渲染目标等。
 */
class ISurface {
public:
    virtual ~ISurface() = default;

    /** @brief 获取用于绘制的画布 */
    [[nodiscard]] virtual ICanvas& canvas() = 0;
    /** @brief 获取用于绘制的画布（只读） */
    [[nodiscard]] virtual const ICanvas& canvas() const = 0;

    /** @brief 将画布内容提交到表面 */
    virtual void flush() = 0;

    /** @brief 调整表面尺寸 */
    virtual void resize(i32 width, i32 height) = 0;

    /** @brief 获取表面宽度 */
    [[nodiscard]] virtual i32 width() const = 0;
    /** @brief 获取表面高度 */
    [[nodiscard]] virtual i32 height() const = 0;
};

} // namespace mc::client::ui::kagero::paint
