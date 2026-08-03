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
#include <functional>
#include <type_traits>

namespace mc {
namespace math {

/**
 * @brief 2D向量模板类
 *
 * 用于表示平面位置、方向、UV坐标等2D量。
 *
 * @tparam T 标量类型 (f32, f64, i32, u32 等)
 */
template <typename T>
class Vector2 {
public:
    T x, y;

    // 构造函数
    Vector2() noexcept
        : x(static_cast<T>(0))
        , y(static_cast<T>(0))
    {}

    Vector2(T x, T y) noexcept
        : x(x)
        , y(y)
    {}

    explicit Vector2(T value) noexcept
        : x(value)
        , y(value)
    {}

    // 静态常量
    static Vector2 zero() { return Vector2(static_cast<T>(0), static_cast<T>(0)); }
    static Vector2 one() { return Vector2(static_cast<T>(1), static_cast<T>(1)); }

    // 算术运算
    [[nodiscard]] Vector2 operator+(const Vector2& other) const noexcept { return {x + other.x, y + other.y}; }

    [[nodiscard]] Vector2 operator-(const Vector2& other) const noexcept { return {x - other.x, y - other.y}; }

    [[nodiscard]] Vector2 operator*(T scalar) const noexcept { return {x * scalar, y * scalar}; }

    [[nodiscard]] Vector2 operator*(const Vector2& other) const noexcept { return {x * other.x, y * other.y}; }

    [[nodiscard]] Vector2 operator/(T scalar) const noexcept { return {x / scalar, y / scalar}; }

    [[nodiscard]] Vector2 operator/(const Vector2& other) const noexcept { return {x / other.x, y / other.y}; }

    Vector2& operator+=(const Vector2& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2& operator*=(T scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2& operator/=(T scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    [[nodiscard]] Vector2 operator-() const noexcept { return {-x, -y}; }

    // 比较运算
    [[nodiscard]] bool operator==(const Vector2& other) const noexcept { return x == other.x && y == other.y; }

    [[nodiscard]] bool operator!=(const Vector2& other) const noexcept { return !(*this == other); }

    // 向量运算 (仅浮点类型)
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T length() const noexcept
    {
        return static_cast<T>(std::sqrt(static_cast<f64>(x * x + y * y)));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T lengthSquared() const noexcept
    {
        return x * x + y * y;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector2 normalized() const noexcept
    {
        const T len = length();
        if (len > static_cast<T>(0)) {
            const T invLen = static_cast<T>(1) / len;
            return {x * invLen, y * invLen};
        }
        return zero();
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    void normalize() noexcept
    {
        *this = normalized();
    }

    [[nodiscard]] T dot(const Vector2& other) const noexcept { return x * other.x + y * other.y; }

    /**
     * @brief 2D叉积（返回标量）
     *
     * 结果为 this.x * other.y - this.y * other.x
     * 正值表示 other 在 this 的逆时针方向
     */
    [[nodiscard]] T cross(const Vector2& other) const noexcept { return x * other.y - y * other.x; }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T distance(const Vector2& other) const noexcept
    {
        return (*this - other).length();
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T distanceSquared(const Vector2& other) const noexcept
    {
        return (*this - other).lengthSquared();
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector2 lerp(const Vector2& target, T t) const noexcept
    {
        return {x + (target.x - x) * t, y + (target.y - y) * t};
    }

    // 浮点类型特化方法
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector2 rotated(T angle) const noexcept
    {
        const T c = static_cast<T>(std::cos(static_cast<f64>(angle)));
        const T s = static_cast<T>(std::sin(static_cast<f64>(angle)));
        return {x * c - y * s, x * s + y * c};
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector2 perpendicular() const noexcept
    {
        return {-y, x};
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T angle() const noexcept
    {
        return static_cast<T>(std::atan2(static_cast<f64>(y), static_cast<f64>(x)));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] static Vector2 fromAngle(T angle) noexcept
    {
        return {static_cast<T>(std::cos(static_cast<f64>(angle))), static_cast<T>(std::sin(static_cast<f64>(angle)))};
    }

    // 访问器
    [[nodiscard]] T& operator[](size_t index) noexcept { return index == 0 ? x : y; }

    [[nodiscard]] const T& operator[](size_t index) const noexcept { return index == 0 ? x : y; }

    // 类型转换
    template <typename U>
    [[nodiscard]] Vector2<U> cast() const noexcept
    {
        return {static_cast<U>(x), static_cast<U>(y)};
    }
};

// 标量 * 向量
template <typename T>
[[nodiscard]] inline Vector2<T> operator*(T scalar, const Vector2<T>& vec) noexcept
{
    return vec * scalar;
}

// 类型别名
using Vector2f = Vector2<f32>;
using Vector2d = Vector2<f64>;
using Vector2i = Vector2<i32>;

} // namespace math

// 为了方便使用，在 mc 命名空间也提供别名
using Vector2f = math::Vector2f;
using Vector2d = math::Vector2d;
using Vector2i = math::Vector2i;
// 向后兼容别名
using Vector2 = math::Vector2f;

} // namespace mc

// 哈希函数支持
namespace std {
template <typename T>
struct hash<mc::math::Vector2<T>> {
    size_t operator()(const mc::math::Vector2<T>& v) const noexcept
    {
        size_t h1 = hash<T>{}(v.x);
        size_t h2 = hash<T>{}(v.y);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std
