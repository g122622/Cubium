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
