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
#include "MathUtils.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>
#include <cstddef>
#include <functional>
#include <type_traits>

namespace mc {
namespace math {

/**
 * @brief 3D向量模板类
 *
 * 用于表示位置、方向、速度等3D量
 *
 * @tparam T 标量类型 (f32, f64, i32, u32 等)
 */
template <typename T>
class Vector3 {
public:
    T x, y, z;

    // 构造函数
    Vector3() noexcept
        : x(static_cast<T>(0))
        , y(static_cast<T>(0))
        , z(static_cast<T>(0))
    {}

    Vector3(T x, T y, T z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {}

    explicit Vector3(T value) noexcept
        : x(value)
        , y(value)
        , z(value)
    {}

    // 静态常量方法
    static Vector3 zero() { return Vector3(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)); }
    static Vector3 one() { return Vector3(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1)); }

    // 算术运算
    [[nodiscard]] Vector3 operator+(const Vector3& other) const noexcept
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] Vector3 operator-(const Vector3& other) const noexcept
    {
        return {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] Vector3 operator*(T scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }

    [[nodiscard]] Vector3 operator*(const Vector3& other) const noexcept
    {
        return {x * other.x, y * other.y, z * other.z};
    }

    [[nodiscard]] Vector3 operator/(T scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }

    [[nodiscard]] Vector3 operator/(const Vector3& other) const noexcept
    {
        return {x / other.x, y / other.y, z / other.z};
    }

    Vector3& operator+=(const Vector3& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3& operator*=(T scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3& operator/=(T scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    [[nodiscard]] Vector3 operator-() const noexcept { return {-x, -y, -z}; }

    // 比较运算
    [[nodiscard]] bool operator==(const Vector3& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] bool operator!=(const Vector3& other) const noexcept { return !(*this == other); }

    // 向量运算 (仅浮点类型)
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T length() const noexcept
    {
        return static_cast<T>(std::sqrt(static_cast<f64>(x * x + y * y + z * z)));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T lengthSquared() const noexcept
    {
        return x * x + y * y + z * z;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T lengthHorizontal() const noexcept
    {
        return static_cast<T>(std::sqrt(static_cast<f64>(x * x + z * z)));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector3 normalized() const noexcept
    {
        const T len = length();
        if (len > static_cast<T>(EPSILON)) {
            const T invLen = static_cast<T>(1) / len;
            return {x * invLen, y * invLen, z * invLen};
        }
        return zero();
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    void normalize() noexcept
    {
        *this = normalized();
    }

    [[nodiscard]] T dot(const Vector3& other) const noexcept { return x * other.x + y * other.y + z * other.z; }

    [[nodiscard]] Vector3 cross(const Vector3& other) const noexcept
    {
        return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T distance(const Vector3& other) const noexcept
    {
        return (*this - other).length();
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T distanceSquared(const Vector3& other) const noexcept
    {
        return (*this - other).lengthSquared();
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T distanceHorizontal(const Vector3& other) const noexcept
    {
        const T dx = x - other.x;
        const T dz = z - other.z;
        return static_cast<T>(std::sqrt(static_cast<f64>(dx * dx + dz * dz)));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector3 lerp(const Vector3& target, T t) const noexcept
    {
        return {mc::math::lerp(x, target.x, t), mc::math::lerp(y, target.y, t), mc::math::lerp(z, target.z, t)};
    }

    // 角度计算 (仅浮点类型)
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T pitch() const noexcept
    {
        return static_cast<T>(-std::asin(static_cast<f64>(y / length())));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] T yaw() const noexcept
    {
        return static_cast<T>(std::atan2(static_cast<f64>(z), static_cast<f64>(x)));
    }

    // 从角度创建方向向量 (仅浮点类型)
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] static Vector3 fromAngles(T pitch, T yaw) noexcept
    {
        const T cosPitch = static_cast<T>(std::cos(static_cast<f64>(pitch)));
        return {cosPitch * static_cast<T>(std::cos(static_cast<f64>(yaw))),
            static_cast<T>(-std::sin(static_cast<f64>(pitch))),
            cosPitch * static_cast<T>(std::sin(static_cast<f64>(yaw)))};
    }

    // 浮点类型特化方法：坐标转换（向下取整到方块坐标）
    // 对于整数类型 Vector3i，直接访问 x/y/z 成员即可
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] BlockCoord blockX() const noexcept
    {
        return static_cast<BlockCoord>(std::floor(x));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] BlockCoord blockY() const noexcept
    {
        return static_cast<BlockCoord>(std::floor(y));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] BlockCoord blockZ() const noexcept
    {
        return static_cast<BlockCoord>(std::floor(z));
    }

    // 浮点类型特化方法：取整
    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector3 floored() const noexcept
    {
        return {static_cast<T>(std::floor(x)), static_cast<T>(std::floor(y)), static_cast<T>(std::floor(z))};
    }

    template <typename U = T, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    [[nodiscard]] Vector3 ceiled() const noexcept
    {
        return {static_cast<T>(std::ceil(x)), static_cast<T>(std::ceil(y)), static_cast<T>(std::ceil(z))};
    }

    // 访问器
    [[nodiscard]] T& operator[](size_t index) noexcept
    {
        switch (index) {
            case 0:
                return x;
            case 1:
                return y;
            default:
                return z;
        }
    }

    [[nodiscard]] const T& operator[](size_t index) const noexcept
    {
        switch (index) {
            case 0:
                return x;
            case 1:
                return y;
            default:
                return z;
        }
    }

    // 类型转换
    template <typename U>
    [[nodiscard]] Vector3<U> cast() const noexcept
    {
        return {static_cast<U>(x), static_cast<U>(y), static_cast<U>(z)};
    }
};

// 标量 * 向量
template <typename T>
[[nodiscard]] inline Vector3<T> operator*(T scalar, const Vector3<T>& vec) noexcept
{
    return vec * scalar;
}

// 类型别名
using Vector3f = Vector3<f32>;
using Vector3d = Vector3<f64>;
using Vector3i = Vector3<i32>;

} // namespace math

// 为了方便使用，在 mc 命名空间也提供别名
using Vector3f = math::Vector3f;
using Vector3d = math::Vector3d;
using Vector3i = math::Vector3i;
// 向后兼容别名
using Vector3 = math::Vector3f;
using Position = math::Vector3f;
using Velocity = math::Vector3f;

} // namespace mc

// 哈希函数支持
namespace std {
template <typename T>
struct hash<mc::math::Vector3<T>> {
    size_t operator()(const mc::math::Vector3<T>& v) const noexcept
    {
        size_t h1 = hash<T>{}(v.x);
        size_t h2 = hash<T>{}(v.y);
        size_t h3 = hash<T>{}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std
