#pragma once

#include "../Color.hpp"

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
