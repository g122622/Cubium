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
#include <limits>

namespace mc::math {

// ============================================================================
// 数学常量
// ============================================================================

/// 圆周率 (f32)
constexpr f32 PI = 3.14159265358979323846f;

/// 圆周率 (f64)
constexpr f64 PI_DOUBLE = 3.14159265358979323846;

/// 2π (f32)
constexpr f32 TWO_PI = 2.0f * PI;

/// τ = 2π (f32)
constexpr f32 TAU = TWO_PI;

/// τ = 2π (f32 alias)
constexpr f32 TAU_F = TAU;

/// π/2 (f32)
constexpr f32 HALF_PI = PI / 2.0f;

/// π/4 (f32)
constexpr f32 QUARTER_PI = PI / 4.0f;

/// 自然常数 e (f32)
constexpr f32 E = 2.71828182845904523536f;

/// √2 (f32) - 用于对角线移动计算
constexpr f32 SQRT2 = 1.41421356237309504880f;

/// 1/√2 (f32) - 用于对角线计算
constexpr f32 INV_SQRT2 = 0.70710678118654752440f;

/// 小 epsilon，用于浮点比较
constexpr f32 EPSILON = 1e-6f;

/// 大 epsilon，用于更宽松的浮点比较
constexpr f32 LARGE_EPSILON = 1e-4f;

/// 精确 epsilon (f64)，用于射线投射、碰撞计算等高精度场景
constexpr f64 EPSILON_PRECISE = 1.0e-7;

/// 地面探测精度，用于检测实体是否在地面
constexpr f32 EPSILON_GROUND_PROBE = 0.01f;

/// 碰撞计算精度，用于碰撞偏移计算
constexpr f32 EPSILON_COLLISION = 1.0e-7f;

/// f32 最大值
constexpr f32 FLOAT_MAX = std::numeric_limits<f32>::max();

/// f32 最小值
constexpr f32 FLOAT_MIN = std::numeric_limits<f32>::lowest();

} // namespace mc::math
