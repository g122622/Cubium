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
// 密度求值器 JIT trampoline 自由函数
//
// JIT 编译产物（asmjit 机器码）不直接调用 C++ 成员函数/访问对象布局，而是经
// cc.invoke 调用本文件的一组自由函数 trampoline。trampoline 内部完成：
//   1. 从 DensityEvalContext 取运行时对象表（objects/subEvaluators/splines）；
//   2. 按索引取出对象并调用其方法（虚分发/成员访问由 C++ 编译器处理）；
//   3. 复用 eval_helpers 纯函数（clampedLerp/getRarity/evalSpline/evalFindTopSurface）。
//
// 这样 JIT 代码只依赖 Op 指令的整数索引与坐标/值参数，完全不碰 C++ 对象内存布局，
// 维度级与区块级求值器可共享同一份 JIT 机器码（两者 Op 序列相同，仅对象表内容不同）。
//
// trampoline 签名设计为" POD 参数 + 整数索引"，便于 asmjit 经 cc.invoke 按系统调用约定
// 传参（f64 进 xmm0-2，整数进 ecx/edx/r8/r9，首参 ctx 进 rcx 转 callee-saved）。
// ============================================================================

#include "common/core/Types.hpp"
#include "common/world/gen/density/ast/AstNodes.hpp"                // WeirdType
#include "common/world/gen/density/ast/CompiledDensityFunction.hpp" // RuntimeObject/CompiledSpline/CompiledDensityFunction

#include <memory>

namespace mc::world::gen::density::ast {

// DensityEvalContext / DensityJitFn 定义在 CompiledDensityFunction.hpp（打破循环 include），
// 本头经上方 include 已可见。

// ============================================================================
// 外部调用 trampoline（JIT 经 cc.invoke 调用）
// 全部为 noexcept 边界函数：内部用 MC_ASSERT_RELEASE_MSG 校验索引/对象非空（与解释器一致）。
// ============================================================================

/// Y_GRADIENT：clampedMap(y, fromY, toY, fromValue, toValue)。
/// 内部复用 eval_helpers::clampedMap（与解释器逐位一致），避免 JIT 内手写 NaN 分支。
[[nodiscard]] f64 jitYGradient(i32 y, f64 fromY, f64 toY, f64 fromValue, f64 toValue) noexcept;

/// NOISE_SAMPLE：噪声采样。objIdx 索引 objects[objIdx].noise，调 getValue(x,y,z)。
[[nodiscard]] f64 jitNoiseSample(const DensityEvalContext* ctx, u32 objIdx, f64 x, f64 y, f64 z) noexcept;

/// WEIRD_SAMPLER：稀有度缩放噪声采样。r=getRarity(type,inputValue)，返回 abs(noise(x/r,y/r,z/r))*r。
[[nodiscard]] f64 jitWeirdSampler(
    const DensityEvalContext* ctx, u32 objIdx, WeirdType type, i32 x, i32 y, i32 z, f64 inputValue) noexcept;

/// DELEGATE：回退原版 DensityFunction.compute(x,y,z)。objIdx 索引 objects[objIdx].densityFunction。
[[nodiscard]] f64 jitDelegate(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept;

/// END_ISLANDS：末地岛屿。同 DELEGATE 走 densityFunction.compute。
[[nodiscard]] f64 jitEndIslands(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept;

/// BEARDIFIER：Beardifier 贡献。objIdx 索引 objects[objIdx].beardifier。
[[nodiscard]] f64 jitBeardifier(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept;

/// MARKER 区块级：缓存对象 compute（NoiseInterpolator/CellCache/CacheOnce/FlatCache/Cache2D）。
/// objIdx 索引 objects[objIdx].densityFunction（newInstance 注入的缓存对象，虚 compute）。
[[nodiscard]] f64 jitCacheCompute(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept;

/// MARKER 运行时 dispatch（维度级与区块级共享同一 JIT 代码）：
/// - objects[objIdx].densityFunction 非 null（区块级，newInstance 注入缓存对象）→ 走 jitCacheCompute 语义；
/// - 为 null（维度级占位）→ 透传 delegate 子求值器 subEvaluators[subIdx]->eval。
/// 对齐解释器 Marker case 的 cacheObj!=null 分支逻辑。JIT 代码只调本函数，不判空（判空在 C++ 侧）。
[[nodiscard]] f64 jitMarkerDispatch(
    const DensityEvalContext* ctx, u32 objIdx, u32 subIdx, i32 x, i32 y, i32 z) noexcept;

/// MARKER 维度级透传 + SHARED_SUBTREE_CALL：调用子求值器 eval(x,y,z)。
/// subIdx 索引 subEvaluators[subIdx]->eval。
[[nodiscard]] f64 jitDelegateSubEval(const DensityEvalContext* ctx, u32 subIdx, i32 x, i32 y, i32 z) noexcept;

/// SPLINE：样条求值。objIdx 索引 splines[objIdx]，二分+Hermite（见 eval_helpers::evalSpline）。
[[nodiscard]] f64 jitSpline(const DensityEvalContext* ctx, u32 objIdx, f64 point, i32 x, i32 y, i32 z) noexcept;

/// FIND_TOP_SURFACE：循环向下找首个 density>0 的 Y（见 eval_helpers::evalFindTopSurface）。
/// subIdx 索引 density 子求值器 subEvaluators[subIdx]。
[[nodiscard]] f64 jitFindTopSurface(
    const DensityEvalContext* ctx, u32 subIdx, f64 upper, i32 lowerBound, i32 cellHeight, i32 x, i32 z) noexcept;

// ============================================================================
// 数学函数 trampoline（SSE 无单条指令，调 libm）
// ============================================================================

/// UNARY(Sin)：std::sin(v)。SSE 无 sinsd 指令，走 libm。
[[nodiscard]] f64 jitSin(f64 v) noexcept;

/// UNARY(Cos)：std::cos(v)。SSE 无 cossd 指令，走 libm。
[[nodiscard]] f64 jitCos(f64 v) noexcept;

} // namespace mc::world::gen::density::ast
