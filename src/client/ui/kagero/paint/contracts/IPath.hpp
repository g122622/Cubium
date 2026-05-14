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

#include "../Geometry.hpp"
#include <vector>

namespace mc::client::ui::kagero::paint {

enum class PathCommand : u8 { MoveTo, LineTo, QuadTo, CubicTo, Close };

struct PathPoint {
    f32 x = 0.0f;
    f32 y = 0.0f;
};

/**
 * @brief 路径接口
 */
class IPath {
public:
    virtual ~IPath() = default;

    virtual void reset() = 0;
    virtual void moveTo(f32 x, f32 y) = 0;
    virtual void lineTo(f32 x, f32 y) = 0;
    virtual void quadTo(f32 x1, f32 y1, f32 x2, f32 y2) = 0;
    virtual void cubicTo(f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3) = 0;
    virtual void close() = 0;

    virtual void addRect(const Rect& rect) = 0;
    virtual void addRRect(const RRect& roundRect) = 0;
    virtual void addCircle(f32 cx, f32 cy, f32 radius) = 0;

    [[nodiscard]] virtual bool isEmpty() const = 0;
};

} // namespace mc::client::ui::kagero::paint
