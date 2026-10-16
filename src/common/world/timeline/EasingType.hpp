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
#include "common/util/math/MathUtils.hpp"

#include <cmath>

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 缓动类型
 *
 * MC 1.21.11 interface EasingType。
 * 定义关键帧之间插值进度的变换函数。
 * apply(f) 接收线性进度 [0,1]，返回变换后的进度。
 *
 * 具名缓动函数集合与 vanilla net.minecraft.util.Ease 逐一对齐；
 * vanilla 的 CubicBezier（带控制点的三次贝塞尔）需携带状态，与本类的
 * 函数指针模型不兼容，暂未实现（见文末 TODO）。
 */
class EasingType {
public:
    using ApplyFn = f32 (*)(f32);

    constexpr EasingType() noexcept = default;

    explicit constexpr EasingType(ApplyFn fn) noexcept
        : m_fn(fn)
    {}

    f32 apply(f32 f) const noexcept { return m_fn(f); }

    /// 常量缓动：始终返回 0.0
    static const EasingType CONSTANT;
    /// 线性缓动：返回原值
    static const EasingType LINEAR;

    static const EasingType IN_BACK;
    static const EasingType IN_BOUNCE;
    static const EasingType IN_CIRC;
    static const EasingType IN_CUBIC;
    static const EasingType IN_ELASTIC;
    static const EasingType IN_EXPO;
    static const EasingType IN_QUAD;
    static const EasingType IN_QUART;
    static const EasingType IN_QUINT;
    static const EasingType IN_SINE;
    static const EasingType IN_OUT_BACK;
    static const EasingType IN_OUT_BOUNCE;
    static const EasingType IN_OUT_CIRC;
    static const EasingType IN_OUT_CUBIC;
    static const EasingType IN_OUT_ELASTIC;
    static const EasingType IN_OUT_EXPO;
    static const EasingType IN_OUT_QUAD;
    static const EasingType IN_OUT_QUART;
    static const EasingType IN_OUT_QUINT;
    static const EasingType IN_OUT_SINE;
    static const EasingType OUT_BACK;
    static const EasingType OUT_BOUNCE;
    static const EasingType OUT_CIRC;
    static const EasingType OUT_CUBIC;
    static const EasingType OUT_ELASTIC;
    static const EasingType OUT_EXPO;
    static const EasingType OUT_QUAD;
    static const EasingType OUT_QUART;
    static const EasingType OUT_QUINT;
    static const EasingType OUT_SINE;

private:
    ApplyFn m_fn = nullptr;

    static f32 _constant(f32) { return 0.0f; }
    static f32 _linear(f32 f) { return f; }

    // 以下函数体逐条对齐 vanilla net.minecraft.util.Ease，常数与运算顺序保持一致。
    static f32 _inBack(f32 f)
    {
        constexpr f32 C1 = 1.70158f;
        constexpr f32 C2 = 2.70158f;
        return math::square(f) * (C2 * f - C1);
    }

    static f32 _outBounce(f32 f)
    {
        constexpr f32 N = 7.5625f;
        constexpr f32 D = 2.75f;
        if (f < 1.0f / D) {
            return N * math::square(f);
        }
        if (f < 2.0f / D) {
            f -= 1.5f / D;
            return N * math::square(f) + 0.75f;
        }
        if (f < 2.5f / D) {
            f -= 2.25f / D;
            return N * math::square(f) + 0.9375f;
        }
        f -= 2.625f / D;
        return N * math::square(f) + 0.984375f;
    }

    static f32 _inBounce(f32 f) { return 1.0f - _outBounce(1.0f - f); }

    static f32 _inCubic(f32 f) { return math::cube(f); }

    static f32 _inElastic(f32 f)
    {
        if (f == 0.0f) {
            return 0.0f;
        }
        if (f == 1.0f) {
            return 1.0f;
        }
        constexpr f32 C = static_cast<f32>(math::TWO_PI / 3.0);
        return static_cast<f32>(-std::pow(2.0, 10.0 * f - 10.0) * std::sin((f * 10.0f - 10.75f) * C));
    }

    static f32 _inExpo(f32 f) { return f == 0.0f ? 0.0f : static_cast<f32>(std::pow(2.0, 10.0 * f - 10.0)); }

    static f32 _inQuart(f32 f) { return math::square(math::square(f)); }

    static f32 _inQuint(f32 f) { return math::square(math::square(f)) * f; }

    static f32 _inSine(f32 f) { return 1.0f - std::cos(f * math::HALF_PI); }

    static f32 _inOutBounce(f32 f)
    {
        return f < 0.5f ? (1.0f - _outBounce(1.0f - 2.0f * f)) / 2.0f : (1.0f + _outBounce(2.0f * f - 1.0f)) / 2.0f;
    }

    static f32 _inOutCirc(f32 f)
    {
        return f < 0.5f ? static_cast<f32>((1.0 - std::sqrt(1.0 - std::pow(2.0 * f, 2.0))) / 2.0)
                        : static_cast<f32>((std::sqrt(1.0 - std::pow(-2.0 * f + 2.0, 2.0)) + 1.0) / 2.0);
    }

    static f32 _inOutCubic(f32 f)
    {
        return f < 0.5f ? 4.0f * math::cube(f) : static_cast<f32>(1.0 - std::pow(-2.0 * f + 2.0, 3.0) / 2.0);
    }

    static f32 _inOutQuad(f32 f)
    {
        return f < 0.5f ? 2.0f * math::square(f) : static_cast<f32>(1.0 - std::pow(-2.0 * f + 2.0, 2.0) / 2.0);
    }

    static f32 _inOutQuart(f32 f)
    {
        return f < 0.5f ? 8.0f * math::square(math::square(f))
                        : static_cast<f32>(1.0 - std::pow(-2.0 * f + 2.0, 4.0) / 2.0);
    }

    static f32 _inOutQuint(f32 f)
    {
        return f < 0.5f ? 16.0f * f * f * f * f * f : static_cast<f32>(1.0 - std::pow(-2.0 * f + 2.0, 5.0) / 2.0);
    }

    static f32 _outElastic(f32 f)
    {
        constexpr f32 C = static_cast<f32>(math::TWO_PI / 3.0);
        if (f == 0.0f) {
            return 0.0f;
        }
        if (f == 1.0f) {
            return 1.0f;
        }
        return static_cast<f32>(std::pow(2.0, -10.0 * f) * std::sin((f * 10.0f - 0.75f) * C) + 1.0);
    }

    static f32 _outExpo(f32 f) { return f == 1.0f ? 1.0f : 1.0f - static_cast<f32>(std::pow(2.0, -10.0 * f)); }

    static f32 _outQuad(f32 f) { return 1.0f - math::square(1.0f - f); }

    static f32 _outQuint(f32 f) { return 1.0f - static_cast<f32>(std::pow(1.0 - f, 5.0)); }

    static f32 _outSine(f32 f) { return std::sin(f * math::HALF_PI); }

    static f32 _inOutSine(f32 f) { return -(std::cos(math::PI * f) - 1.0f) / 2.0f; }

    static f32 _outBack(f32 f)
    {
        constexpr f32 C1 = 1.70158f;
        constexpr f32 C2 = 2.70158f;
        return 1.0f + C2 * math::cube(f - 1.0f) + C1 * math::square(f - 1.0f);
    }

    static f32 _outQuart(f32 f) { return 1.0f - math::square(math::square(1.0f - f)); }

    static f32 _outCubic(f32 f) { return 1.0f - math::cube(1.0f - f); }

    static f32 _inOutExpo(f32 f)
    {
        if (f < 0.5f) {
            return f == 0.0f ? 0.0f : static_cast<f32>(std::pow(2.0, 20.0 * f - 10.0) / 2.0);
        }
        return f == 1.0f ? 1.0f : static_cast<f32>((2.0 - std::pow(2.0, -20.0 * f + 10.0)) / 2.0);
    }

    static f32 _inQuad(f32 f) { return f * f; }

    static f32 _outCirc(f32 f) { return static_cast<f32>(std::sqrt(1.0f - math::square(f - 1.0f))); }

    static f32 _inOutElastic(f32 f)
    {
        constexpr f32 C = math::PI * 4.0f / 9.0f;
        if (f == 0.0f) {
            return 0.0f;
        }
        if (f == 1.0f) {
            return 1.0f;
        }
        const f32 d = std::sin((20.0f * f - 11.125f) * C);
        if (f < 0.5f) {
            return static_cast<f32>(-(std::pow(2.0, 20.0 * f - 10.0) * d) / 2.0);
        }
        return static_cast<f32>(std::pow(2.0, -20.0 * f + 10.0) * d / 2.0 + 1.0);
    }

    static f32 _inCirc(f32 f) { return static_cast<f32>(-std::sqrt(1.0f - f * f)) + 1.0f; }

    static f32 _inOutBack(f32 f)
    {
        constexpr f32 C2 = 2.5949094f;
        if (f < 0.5f) {
            return 4.0f * f * f * ((C2 + 1.0f) * 2.0f * f - C2) / 2.0f;
        }
        const f32 t = 2.0f * f - 2.0f;
        return (t * t * ((C2 + 1.0f) * t + C2) + 2.0f) / 2.0f;
    }
};

inline const EasingType EasingType::CONSTANT{EasingType::_constant};
inline const EasingType EasingType::LINEAR{EasingType::_linear};

inline const EasingType EasingType::IN_BACK{EasingType::_inBack};
inline const EasingType EasingType::IN_BOUNCE{EasingType::_inBounce};
inline const EasingType EasingType::IN_CIRC{EasingType::_inCirc};
inline const EasingType EasingType::IN_CUBIC{EasingType::_inCubic};
inline const EasingType EasingType::IN_ELASTIC{EasingType::_inElastic};
inline const EasingType EasingType::IN_EXPO{EasingType::_inExpo};
inline const EasingType EasingType::IN_QUAD{EasingType::_inQuad};
inline const EasingType EasingType::IN_QUART{EasingType::_inQuart};
inline const EasingType EasingType::IN_QUINT{EasingType::_inQuint};
inline const EasingType EasingType::IN_SINE{EasingType::_inSine};
inline const EasingType EasingType::IN_OUT_BACK{EasingType::_inOutBack};
inline const EasingType EasingType::IN_OUT_BOUNCE{EasingType::_inOutBounce};
inline const EasingType EasingType::IN_OUT_CIRC{EasingType::_inOutCirc};
inline const EasingType EasingType::IN_OUT_CUBIC{EasingType::_inOutCubic};
inline const EasingType EasingType::IN_OUT_ELASTIC{EasingType::_inOutElastic};
inline const EasingType EasingType::IN_OUT_EXPO{EasingType::_inOutExpo};
inline const EasingType EasingType::IN_OUT_QUAD{EasingType::_inOutQuad};
inline const EasingType EasingType::IN_OUT_QUART{EasingType::_inOutQuart};
inline const EasingType EasingType::IN_OUT_QUINT{EasingType::_inOutQuint};
inline const EasingType EasingType::IN_OUT_SINE{EasingType::_inOutSine};
inline const EasingType EasingType::OUT_BACK{EasingType::_outBack};
inline const EasingType EasingType::OUT_BOUNCE{EasingType::_outBounce};
inline const EasingType EasingType::OUT_CIRC{EasingType::_outCirc};
inline const EasingType EasingType::OUT_CUBIC{EasingType::_outCubic};
inline const EasingType EasingType::OUT_ELASTIC{EasingType::_outElastic};
inline const EasingType EasingType::OUT_EXPO{EasingType::_outExpo};
inline const EasingType EasingType::OUT_QUAD{EasingType::_outQuad};
inline const EasingType EasingType::OUT_QUART{EasingType::_outQuart};
inline const EasingType EasingType::OUT_QUINT{EasingType::_outQuint};
inline const EasingType EasingType::OUT_SINE{EasingType::_outSine};

// TODO: vanilla EasingType 另支持 CubicBezier（带 x1/y1/x2/y2 控制点的三次贝塞尔，
//   经 Newton-Raphson 迭代求解）。它需要携带四个控制点的状态，与本类的无状态
//   函数指针模型（ApplyFn）不兼容；待 timeline 数据包加载器需要该类型时，再引入
//   独立的 CubicBezier 值类型（含 controls 字段与 apply 实现）。

} // namespace timeline
} // namespace world
} // namespace mc
