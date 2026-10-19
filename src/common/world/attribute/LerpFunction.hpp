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
#include <functional>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 关键帧插值函数
 *
 * 函数式接口，定义两个关键帧值之间如何插值。
 * apply(progress, from, to) 返回插值结果：
 * - progress ∈ [0, 1] 表示插值进度
 * - from / to 分别为起始/结束关键帧的值
 */
template <typename T>
class LerpFunction {
public:
    LerpFunction() = default;

    explicit LerpFunction(std::function<T(f32, T, T)> fn)
        : m_fn(std::move(fn))
    {}

    T apply(f32 progress, T from, T to) const { return m_fn(progress, from, to); }

    /**
     * @brief 以后处理函数转换插值目标类型
     *
     * 返回的函数对 from/to 先做转换再插值。用于把"参数类型"上的关键帧插值
     * 套用到"值类型"上（参数与值存在一一对应关系时）。
     *
     * @tparam Output 输出类型
     * @param convert 输入值 → 输出值 的转换（在每次 apply 时调用）
     * @return 输出类型上的插值函数
     */
    template <typename Output>
    [[nodiscard]] LerpFunction<Output> map(std::function<Output(T)> convert) const
    {
        const LerpFunction<T> self = *this;
        return LerpFunction<Output>([self, convert = std::move(convert)](f32 progress, Output from, Output to) {
            return self.apply(progress, convert(from), convert(to));
        });
    }

    /**
     * @brief 浮点线性插值
     */
    static LerpFunction<f32> ofFloat()
    {
        return LerpFunction<f32>([](f32 progress, f32 from, f32 to) { return from + progress * (to - from); });
    }

    /**
     * @brief 角度插值（处理角度环绕）
     */
    static LerpFunction<f32> ofDegrees(f32 threshold)
    {
        return LerpFunction<f32>([threshold](f32 progress, f32 from, f32 to) {
            const f32 diff = _wrapDegrees(to - from);
            return std::abs(diff) >= threshold ? to : from + progress * diff;
        });
    }

    /**
     * @brief 常量插值（始终返回 from）
     */
    template <typename U>
    static LerpFunction<U> ofConstant()
    {
        return LerpFunction<U>([](f32, U from, U) { return from; });
    }

    /**
     * @brief 阶跃插值
     *
     * progress >= threshold 时返回 to，否则返回 from。
     */
    static LerpFunction<T> ofStep(f32 threshold)
    {
        return LerpFunction<T>([threshold](f32 progress, T from, T to) { return progress >= threshold ? to : from; });
    }

    /**
     * @brief 颜色插值（sRGB 线性混合）
     */
    static LerpFunction<i32> ofColor()
    {
        return LerpFunction<i32>([](f32 progress, i32 from, i32 to) { return srgbLerp(progress, from, to); });
    }

    /**
     * @brief sRGB 颜色分通道线性插值
     *
     * 与 ARGB.srgbLerp / Mth.lerpInt 一致：通道结果取整采用 round-toward-negative-infinity
     * （负数向下取整、非负截断），而非默认的向零截断。
     *
     * @param progress 插值进度
     * @param from 起始颜色（ARGB）
     * @param to 结束颜色（ARGB）
     * @return 插值得到的颜色（ARGB）
     */
    static i32 srgbLerp(f32 progress, i32 from, i32 to)
    {
        auto lerpChannel = [progress](i32 c1, i32 c2) {
            const f32 result = c1 + progress * (c2 - c1);
            return static_cast<i32>(result < 0.0f ? result - 1.0f : result);
        };

        const i32 a1 = (from >> 24) & 0xFF;
        const i32 r1 = (from >> 16) & 0xFF;
        const i32 g1 = (from >> 8) & 0xFF;
        const i32 b1 = from & 0xFF;

        const i32 a2 = (to >> 24) & 0xFF;
        const i32 r2 = (to >> 16) & 0xFF;
        const i32 g2 = (to >> 8) & 0xFF;
        const i32 b2 = to & 0xFF;

        const i32 a = lerpChannel(a1, a2);
        const i32 r = lerpChannel(r1, r2);
        const i32 g = lerpChannel(g1, g2);
        const i32 b = lerpChannel(b1, b2);

        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    /// 角度归一化到 [-180, 180)
    static f32 _wrapDegrees(f32 degrees) { return math::wrapDegrees(degrees); }

    std::function<T(f32, T, T)> m_fn;
};

} // namespace attribute
} // namespace world
} // namespace mc
