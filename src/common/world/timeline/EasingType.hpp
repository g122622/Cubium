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

#include <array>
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
 * 另支持携带四个控制点的三次贝塞尔（vanilla EasingType.CubicBezier）。
 */
class EasingType {
public:
    /// 三次贝塞尔求解时 Newton-Raphson 的最大迭代次数（vanilla NEWTON_RAPHSON_ITERATIONS）
    static constexpr i32 NEWTON_RAPHSON_ITERATIONS = 4;

    /// 默认缓动为线性
    constexpr EasingType() noexcept = default;

    /**
     * @brief 构造三次贝塞尔缓动
     *
     * 控制点为 (0,0)、(x1,y1)、(x2,y2)、(1,1)，x1/x2 必须落在 [0,1]。
     *
     * @param x1 第一个控制点横坐标
     * @param y1 第一个控制点纵坐标
     * @param x2 第二个控制点横坐标
     * @param y2 第二个控制点纵坐标
     */
    [[nodiscard]] static EasingType cubicBezier(f32 x1, f32 y1, f32 x2, f32 y2);

    /**
     * @brief 构造对称三次贝塞尔
     *
     * 第二个控制点为 (1 - x1, 1 - y1)，曲线关于对角线对称。
     *
     * @param x1 第一个控制点横坐标
     * @param y1 第一个控制点纵坐标
     */
    [[nodiscard]] static EasingType symmetricCubicBezier(f32 x1, f32 y1);

    [[nodiscard]] bool isCubicBezier() const noexcept { return m_name == Name::CubicBezier; }

    /// 三次贝塞尔控制点（x1, y1, x2, y2）；非贝塞尔缓动下无意义
    [[nodiscard]] const std::array<f32, 4>& bezierControls() const noexcept { return m_controls; }

    /**
     * @brief 应用缓动
     *
     * 具名缓动按枚举分派到对应函数；`LINEAR` / `CONSTANT` 直接返回，不引入函数间接调用。
     */
    f32 apply(f32 f) const noexcept
    {
        switch (m_name) {
            case Name::Linear:
                return f;
            case Name::Constant:
                return 0.0f;
            case Name::CubicBezier:
                return _applyBezier(f);
            default:
                break;
        }
        return _applyNamed(m_name, f);
    }

    /// 线性缓动：返回原值
    static const EasingType LINEAR;
    /// 常量缓动：始终返回 0.0
    static const EasingType CONSTANT;

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

    /// 缓动的具名标识（与 vanilla EasingType 的各个记录类一一对应）
    enum class Name : u8 {
        Linear,
        Constant,
        InBack,
        OutBack,
        InOutBack,
        InBounce,
        OutBounce,
        InOutBounce,
        InElastic,
        OutElastic,
        InOutElastic,
        InCubic,
        OutCubic,
        InOutCubic,
        InExpo,
        OutExpo,
        InOutExpo,
        InCirc,
        OutCirc,
        InOutCirc,
        InQuad,
        OutQuad,
        InOutQuad,
        InQuart,
        OutQuart,
        InOutQuart,
        InQuint,
        OutQuint,
        InOutQuint,
        InSine,
        OutSine,
        InOutSine,
        /// 三次贝塞尔：曲线由 m_controls（控制点 x1, y1, x2, y2）决定
        CubicBezier,
    };

private:
    constexpr EasingType(Name name) noexcept
        : m_name(name)
    {}

    /// 具名缓动标识
    Name m_name = Name::Linear;
    /// 三次贝塞尔控制点 (x1, y1, x2, y2)；仅 CubicBezier 使用
    std::array<f32, 4> m_controls = {0.0f, 0.0f, 1.0f, 1.0f};

    /**
     * @brief 三次曲线的多项式系数
     *
     * sample(t) = ((a*t + b)*t + c)*t，sampleGradient(t) = (3a*t + 2b)*t + c。
     * 首控制点固定在 0、末控制点固定在 1，因此常数项为 0。
     */
    struct _CubicCurve {
        f32 a;
        f32 b;
        f32 c;

        [[nodiscard]] constexpr f32 sample(f32 t) const noexcept { return ((a * t + b) * t + c) * t; }
        [[nodiscard]] constexpr f32 sampleGradient(f32 t) const noexcept { return (3.0f * a * t + 2.0f * b) * t + c; }
    };

    [[nodiscard]] static constexpr _CubicCurve _curveFromControls(f32 first, f32 second) noexcept
    {
        return _CubicCurve{3.0f * first - 3.0f * second + 1.0f, -6.0f * first + 3.0f * second, 3.0f * first};
    }

    /**
     * @brief 按 x 坐标反解参数 t，再求 y 坐标
     *
     * 与 vanilla 一致：以 Newton-Raphson 迭代 4 次逼近 x(t) = f，导数过小时提前退出。
     */
    [[nodiscard]] f32 _applyBezier(f32 f) const noexcept
    {
        const _CubicCurve xCurve = _curveFromControls(m_controls[0], m_controls[2]);
        const _CubicCurve yCurve = _curveFromControls(m_controls[1], m_controls[3]);

        f32 t = f;
        for (i32 i = 0; i < NEWTON_RAPHSON_ITERATIONS; ++i) {
            const f32 gradient = xCurve.sampleGradient(t);
            if (gradient < 1.0e-5f) {
                break;
            }

            t -= (xCurve.sample(t) - f) / gradient;
        }

        return yCurve.sample(t);
    }

    /// 把具名标识分派到对应的缓动函数
    [[nodiscard]] static f32 _applyNamed(Name name, f32 f) noexcept
    {
        switch (name) {
            case Name::InBack:
                return _inBack(f);
            case Name::OutBack:
                return _outBack(f);
            case Name::InOutBack:
                return _inOutBack(f);
            case Name::InBounce:
                return _inBounce(f);
            case Name::OutBounce:
                return _outBounce(f);
            case Name::InOutBounce:
                return _inOutBounce(f);
            case Name::InElastic:
                return _inElastic(f);
            case Name::OutElastic:
                return _outElastic(f);
            case Name::InOutElastic:
                return _inOutElastic(f);
            case Name::InCubic:
                return _inCubic(f);
            case Name::OutCubic:
                return _outCubic(f);
            case Name::InOutCubic:
                return _inOutCubic(f);
            case Name::InExpo:
                return _inExpo(f);
            case Name::OutExpo:
                return _outExpo(f);
            case Name::InOutExpo:
                return _inOutExpo(f);
            case Name::InCirc:
                return _inCirc(f);
            case Name::OutCirc:
                return _outCirc(f);
            case Name::InOutCirc:
                return _inOutCirc(f);
            case Name::InQuad:
                return _inQuad(f);
            case Name::OutQuad:
                return _outQuad(f);
            case Name::InOutQuad:
                return _inOutQuad(f);
            case Name::InQuart:
                return _inQuart(f);
            case Name::OutQuart:
                return _outQuart(f);
            case Name::InOutQuart:
                return _inOutQuart(f);
            case Name::InQuint:
                return _inQuint(f);
            case Name::OutQuint:
                return _outQuint(f);
            case Name::InOutQuint:
                return _inOutQuint(f);
            case Name::InSine:
                return _inSine(f);
            case Name::OutSine:
                return _outSine(f);
            case Name::InOutSine:
                return _inOutSine(f);
            case Name::Linear:
                return f;
            case Name::Constant:
            case Name::CubicBezier:
                break;
        }
        return 0.0f;
    }

    // 以下函数体与 vanilla net.minecraft.util.Ease 逐一对应，常数与运算顺序保持一致。
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
        return static_cast<f32>(-std::pow(2.0, 10.0 * f - 10.0) * math::mthSin((f * 10.0f - 10.75f) * C));
    }

    static f32 _inExpo(f32 f) { return f == 0.0f ? 0.0f : static_cast<f32>(std::pow(2.0, 10.0 * f - 10.0)); }

    static f32 _inQuart(f32 f) { return math::square(math::square(f)); }

    static f32 _inQuint(f32 f) { return math::square(math::square(f)) * f; }

    static f32 _inSine(f32 f) { return 1.0f - math::mthCos(f * math::HALF_PI); }

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
        return static_cast<f32>(std::pow(2.0, -10.0 * f) * math::mthSin((f * 10.0f - 0.75f) * C) + 1.0);
    }

    static f32 _outExpo(f32 f) { return f == 1.0f ? 1.0f : 1.0f - static_cast<f32>(std::pow(2.0, -10.0 * f)); }

    static f32 _outQuad(f32 f) { return 1.0f - math::square(1.0f - f); }

    static f32 _outQuint(f32 f) { return 1.0f - static_cast<f32>(std::pow(1.0 - f, 5.0)); }

    static f32 _outSine(f32 f) { return math::mthSin(f * math::HALF_PI); }

    static f32 _inOutSine(f32 f) { return -(math::mthCos(math::PI * f) - 1.0f) / 2.0f; }

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
        const f32 d = math::mthSin((20.0f * f - 11.125f) * C);
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

inline const EasingType EasingType::LINEAR{Name::Linear};
inline const EasingType EasingType::CONSTANT{Name::Constant};

inline const EasingType EasingType::IN_BACK{Name::InBack};
inline const EasingType EasingType::IN_BOUNCE{Name::InBounce};
inline const EasingType EasingType::IN_CIRC{Name::InCirc};
inline const EasingType EasingType::IN_CUBIC{Name::InCubic};
inline const EasingType EasingType::IN_ELASTIC{Name::InElastic};
inline const EasingType EasingType::IN_EXPO{Name::InExpo};
inline const EasingType EasingType::IN_QUAD{Name::InQuad};
inline const EasingType EasingType::IN_QUART{Name::InQuart};
inline const EasingType EasingType::IN_QUINT{Name::InQuint};
inline const EasingType EasingType::IN_SINE{Name::InSine};
inline const EasingType EasingType::IN_OUT_BACK{Name::InOutBack};
inline const EasingType EasingType::IN_OUT_BOUNCE{Name::InOutBounce};
inline const EasingType EasingType::IN_OUT_CIRC{Name::InOutCirc};
inline const EasingType EasingType::IN_OUT_CUBIC{Name::InOutCubic};
inline const EasingType EasingType::IN_OUT_ELASTIC{Name::InOutElastic};
inline const EasingType EasingType::IN_OUT_EXPO{Name::InOutExpo};
inline const EasingType EasingType::IN_OUT_QUAD{Name::InOutQuad};
inline const EasingType EasingType::IN_OUT_QUART{Name::InOutQuart};
inline const EasingType EasingType::IN_OUT_QUINT{Name::InOutQuint};
inline const EasingType EasingType::IN_OUT_SINE{Name::InOutSine};
inline const EasingType EasingType::OUT_BACK{Name::OutBack};
inline const EasingType EasingType::OUT_BOUNCE{Name::OutBounce};
inline const EasingType EasingType::OUT_CIRC{Name::OutCirc};
inline const EasingType EasingType::OUT_CUBIC{Name::OutCubic};
inline const EasingType EasingType::OUT_ELASTIC{Name::OutElastic};
inline const EasingType EasingType::OUT_EXPO{Name::OutExpo};
inline const EasingType EasingType::OUT_QUAD{Name::OutQuad};
inline const EasingType EasingType::OUT_QUART{Name::OutQuart};
inline const EasingType EasingType::OUT_QUINT{Name::OutQuint};
inline const EasingType EasingType::OUT_SINE{Name::OutSine};

inline EasingType EasingType::cubicBezier(f32 x1, f32 y1, f32 x2, f32 y2)
{
    EasingType easing{Name::CubicBezier};
    easing.m_controls = {x1, y1, x2, y2};
    return easing;
}

inline EasingType EasingType::symmetricCubicBezier(f32 x1, f32 y1)
{
    return cubicBezier(x1, y1, 1.0f - x1, 1.0f - y1);
}

} // namespace timeline
} // namespace world
} // namespace mc
