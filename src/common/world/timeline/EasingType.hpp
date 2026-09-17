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

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 缓动类型
 *
 * MC 1.21.11 interface EasingType。
 * 定义关键帧之间插值进度的变换函数。
 * apply(f) 接收线性进度 [0,1]，返回变换后的进度。
 */
class EasingType {
public:
    using ApplyFn = float (*)(float);

    constexpr EasingType() noexcept = default;

    explicit constexpr EasingType(ApplyFn fn) noexcept
        : m_fn(fn)
    {}

    float apply(float f) const noexcept
    {
        return m_fn(f);
    }

    /// 常量缓动：始终返回 0.0
    static const EasingType CONSTANT;
    /// 线性缓动：返回原值
    static const EasingType LINEAR;

    // TODO: 其他缓动类型（IN_BACK/IN_BOUNCE/...）暂未实现，按需补充

private:
    ApplyFn m_fn = nullptr;

    static float _constant(float) { return 0.0f; }
    static float _linear(float f) { return f; }
};

inline const EasingType EasingType::CONSTANT{EasingType::_constant};
inline const EasingType EasingType::LINEAR{EasingType::_linear};

} // namespace timeline
} // namespace world
} // namespace mc
