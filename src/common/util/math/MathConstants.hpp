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

/// f32 最大值
constexpr f32 FLOAT_MAX = std::numeric_limits<f32>::max();

/// f32 最小值
constexpr f32 FLOAT_MIN = std::numeric_limits<f32>::lowest();

} // namespace mc::math
