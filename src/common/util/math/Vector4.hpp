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

#include "../../core/Types.hpp"
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace mc {
namespace math {

/**
 * @brief 4D向量模板类
 *
 * 用于表示颜色(RGBA)、位置齐次坐标、四元数等4D量
 *
 * @tparam T 标量类型 (f32, f64, i32, u32 等)
 */
template <typename T>
class Vector4 {
public:
    T x, y, z, w;

    // 构造函数
    Vector4() noexcept
        : x(static_cast<T>(0))
        , y(static_cast<T>(0))
        , z(static_cast<T>(0))
        , w(static_cast<T>(0))
    {}

    Vector4(T x, T y, T z, T w) noexcept
        : x(x)
        , y(y)
        , z(z)
        , w(w)
    {}

    explicit Vector4(T value) noexcept
        : x(value)
        , y(value)
        , z(value)
        , w(value)
    {}

    // 静态常量
    static Vector4 zero()
    {
        return Vector4(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
    }
    static Vector4 one() { return Vector4(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1), static_cast<T>(1)); }

    // 算术运算
    [[nodiscard]] Vector4 operator+(const Vector4& other) const noexcept
    {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    [[nodiscard]] Vector4 operator-(const Vector4& other) const noexcept
    {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    [[nodiscard]] Vector4 operator*(T scalar) const noexcept
    {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }

    [[nodiscard]] Vector4 operator*(const Vector4& other) const noexcept
    {
        return {x * other.x, y * other.y, z * other.z, w * other.w};
    }

    [[nodiscard]] Vector4 operator/(T scalar) const noexcept
    {
        return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    [[nodiscard]] Vector4 operator/(const Vector4& other) const noexcept
    {
        return {x / other.x, y / other.y, z / other.z, w / other.w};
    }

    Vector4& operator+=(const Vector4& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    Vector4& operator-=(const Vector4& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    Vector4& operator*=(T scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    Vector4& operator/=(T scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    [[nodiscard]] Vector4 operator-() const noexcept { return {-x, -y, -z, -w}; }

    // 比较运算
    [[nodiscard]] bool operator==(const Vector4& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    [[nodiscard]] bool operator!=(const Vector4& other) const noexcept { return !(*this == other); }

    // 向量运算 (仅浮点类型)
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T length() const noexcept
    {
        return static_cast<T>(std::sqrt(static_cast<f64>(x * x + y * y + z * z + w * w)));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T lengthSquared() const noexcept
    {
        return x * x + y * y + z * z + w * w;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector4 normalized() const noexcept
    {
        const T len = length();
        if (len > static_cast<T>(0)) {
            const T invLen = static_cast<T>(1) / len;
            return {x * invLen, y * invLen, z * invLen, w * invLen};
        }
        return zero();
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    void normalize() noexcept
    {
        *this = normalized();
    }

    [[nodiscard]] T dot(const Vector4& other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector4 lerp(const Vector4& target, T t) const noexcept
    {
        return {x + (target.x - x) * t, y + (target.y - y) * t, z + (target.z - z) * t, w + (target.w - w) * t};
    }

    // 访问器
    [[nodiscard]] T& operator[](size_t index) noexcept
    {
        switch (index) {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                return w;
        }
    }

    [[nodiscard]] const T& operator[](size_t index) const noexcept
    {
        switch (index) {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                return w;
        }
    }

    // 类型转换
    template <typename U>
    [[nodiscard]] Vector4<U> cast() const noexcept
    {
        return {static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w)};
    }
};

// 标量 * 向量
template <typename T>
[[nodiscard]] inline Vector4<T> operator*(T scalar, const Vector4<T>& vec) noexcept
{
    return vec * scalar;
}

// 类型别名
using Vector4f = Vector4<f32>;
using Vector4d = Vector4<f64>;
using Vector4i = Vector4<i32>;
using Vector4u = Vector4<u32>;

// 兼容旧代码的别名 (在 math 命名空间内)
// 注意：使用时需要 math::Vector4f 或 using math::Vector4f

} // namespace math

// 为了方便使用，在 mc 命名空间也提供别名
using Vector4f = math::Vector4f;
using Vector4d = math::Vector4d;
using Vector4i = math::Vector4i;
using Vector4u = math::Vector4u;

} // namespace mc
