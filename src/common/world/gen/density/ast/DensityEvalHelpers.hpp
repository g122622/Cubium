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

// ============================================================================
// 密度求值纯函数集（解释器 evalImpl 与 JIT trampoline 共享）
//
// 这些函数从原 CompiledDensityFunction.cpp 匿名命名空间提取，供：
//   1. 解释器 evalImpl（switch case 内联调用）
//   2. JIT trampoline（DensityJitTrampolines.cpp 经 cc.invoke 调用的外部函数）
// 共同复用，避免逻辑重复。全部为无状态纯函数（除 evalSpline/evalFindTopSurface
// 会回调子求值器 eval，但其本身不持有可变状态）。
//
// 放独立头而非匿名命名空间：trampoline 是独立编译单元，需可见链接。
// ============================================================================

#include "common/core/Types.hpp"
#include "common/world/gen/density/ast/AstNodes.hpp"                // WeirdType
#include "common/world/gen/density/ast/CompiledDensityFunction.hpp" // CompiledSpline/CompiledDensityFunction

#include <vector>

namespace mc::world::gen::density::ast::eval_helpers {

/// clampedLerp（密度函数 clampedLerp 语义）：delta<=0→from, delta>=1→to, else from+delta*(to-from)。
[[nodiscard]] f64 clampedLerp(f64 delta, f64 from, f64 to) noexcept;

/// clampedMap：把 value 从 [fromMin,fromMax] 线性映射到 [toMin,toMax]，越界钳位。
[[nodiscard]] f64 clampedMap(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax) noexcept;

/// WeirdScaledSampler 的 rarity 映射（Type1 maxRarity=2.0 / Type2 maxRarity=3.0）。
[[nodiscard]] f64 getRarity(WeirdType type, f64 value) noexcept;

/// 样条二分查找：返回最大 r 使 locations[r] <= point；point < locations[0] 返回 -1，
/// point >= locations[last] 返回 last。
[[nodiscard]] i64 findSplineRange(const std::vector<f64>& locations, f64 point) noexcept;

/// 样条越界线性外推。
[[nodiscard]] f64 splineLinearExtend(f64 point, f64 location, f64 value, f64 derivative) noexcept;

/// 在预编译样条上求值：二分查找 + Hermite 三次插值。
/// valueEvaluators 各控制点的值由子求值器 eval(x,y,z) 提供（常量点亦封装为求值器）。
[[nodiscard]] f64 evalSpline(const CompiledSpline& spline, f64 point, i32 x, i32 y, i32 z);

/// FindTopSurface 求值：从 floor(upper/cellH)*cellH 向下逐 cellH 找首个 density>0 的 Y。
/// density 子树由 densitySub.eval(x,j,z) 提供。
[[nodiscard]] f64 evalFindTopSurface(
    const CompiledDensityFunction& densitySub, f64 upperVal, i32 lowerBound, i32 cellHeight, i32 x, i32 z);

} // namespace mc::world::gen::density::ast::eval_helpers
