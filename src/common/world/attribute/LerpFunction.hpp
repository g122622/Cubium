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

    explicit LerpFunction(std::function<T(float, T, T)> fn)
        : m_fn(std::move(fn))
    {}

    T apply(float progress, T from, T to) const { return m_fn(progress, from, to); }

    /**
     * @brief 浮点线性插值
     */
    static LerpFunction<float> ofFloat()
    {
        return LerpFunction<float>([](float progress, float from, float to) { return from + progress * (to - from); });
    }

    /**
     * @brief 角度插值（处理角度环绕）
     */
    static LerpFunction<float> ofDegrees(float threshold)
    {
        return LerpFunction<float>([threshold](float progress, float from, float to) {
            float diff = _wrapDegrees(to - from);
            return std::abs(diff) >= threshold ? to : from + progress * diff;
        });
    }

    /**
     * @brief 常量插值（始终返回 from）
     */
    template <typename U>
    static LerpFunction<U> ofConstant()
    {
        return LerpFunction<U>([](float, U from, U) { return from; });
    }

    /**
     * @brief 阶跃插值
     *
     * progress >= threshold 时返回 to，否则返回 from。
     */
    static LerpFunction<T> ofStep(float threshold)
    {
        return LerpFunction<T>([threshold](float progress, T from, T to) { return progress >= threshold ? to : from; });
    }

    /**
     * @brief 颜色插值（sRGB 线性混合）
     */
    static LerpFunction<int> ofColor()
    {
        return LerpFunction<int>([](float progress, int from, int to) { return _srgbLerp(progress, from, to); });
    }

private:
    std::function<T(float, T, T)> m_fn;

    /// 角度归一化到 [-180, 180)
    static float _wrapDegrees(float degrees)
    {
        float x = degrees;
        while (x >= 180.0f)
            x -= 360.0f;
        while (x < -180.0f)
            x += 360.0f;
        return x;
    }

    /// sRGB 颜色线性插值
    static int _srgbLerp(float progress, int from, int to)
    {
        auto lerpChannel = [progress](int c1, int c2) { return static_cast<int>(c1 + progress * (c2 - c1)); };

        const int a1 = (from >> 24) & 0xFF;
        const int r1 = (from >> 16) & 0xFF;
        const int g1 = (from >> 8) & 0xFF;
        const int b1 = from & 0xFF;

        const int a2 = (to >> 24) & 0xFF;
        const int r2 = (to >> 16) & 0xFF;
        const int g2 = (to >> 8) & 0xFF;
        const int b2 = to & 0xFF;

        const int a = lerpChannel(a1, a2);
        const int r = lerpChannel(r1, r2);
        const int g = lerpChannel(g1, g2);
        const int b = lerpChannel(b1, b2);

        return (a << 24) | (r << 16) | (g << 8) | b;
    }
};

} // namespace attribute
} // namespace world
} // namespace mc
