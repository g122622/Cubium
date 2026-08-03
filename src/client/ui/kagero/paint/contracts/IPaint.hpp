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

#include "client/ui/kagero/paint/Color.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::kagero::paint {

enum class PaintStyle : u8 { Fill, Stroke, StrokeAndFill };

enum class StrokeCap : u8 { Butt, Round, Square };

enum class StrokeJoin : u8 { Miter, Round, Bevel };

/**
 * @brief 画笔接口
 */
class IPaint {
public:
    virtual ~IPaint() = default;

    virtual void setColor(const Color& color) = 0;
    [[nodiscard]] virtual Color color() const = 0;

    virtual void setStyle(PaintStyle style) = 0;
    [[nodiscard]] virtual PaintStyle style() const = 0;

    virtual void setStrokeWidth(f32 width) = 0;
    [[nodiscard]] virtual f32 strokeWidth() const = 0;

    virtual void setStrokeCap(StrokeCap cap) = 0;
    [[nodiscard]] virtual StrokeCap strokeCap() const = 0;

    virtual void setStrokeJoin(StrokeJoin join) = 0;
    [[nodiscard]] virtual StrokeJoin strokeJoin() const = 0;

    virtual void setAntiAlias(bool enabled) = 0;
    [[nodiscard]] virtual bool antiAlias() const = 0;

    virtual void setAlpha(f32 alpha) = 0;
    [[nodiscard]] virtual f32 alpha() const = 0;
};

} // namespace mc::client::ui::kagero::paint
