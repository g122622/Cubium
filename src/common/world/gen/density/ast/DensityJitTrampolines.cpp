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

#include "common/world/gen/density/ast/DensityJitTrampolines.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/density/Beardifier.hpp"
#include "common/world/gen/density/ast/DensityEvalHelpers.hpp"
#include "common/world/gen/density/ast/DensityEvalProfiler.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"

#include <cmath>

namespace mc::world::gen::density::ast {

f64 jitYGradient(i32 y, f64 fromY, f64 toY, f64 fromValue, f64 toValue) noexcept
{
    return eval_helpers::clampedMap(static_cast<f64>(y), fromY, toY, fromValue, toValue);
}

f64 jitNoiseSample(const DensityEvalContext* ctx, u32 objIdx, f64 x, f64 y, f64 z) noexcept
{
    const auto* noise = ctx->objects[objIdx].noise;
    MC_ASSERT_RELEASE_MSG(noise != nullptr, "jitNoiseSample: noise object is null");
    // A 类叶子外部调用计时（口径与解释器 evalImpl NoiseSample case 一致），累加 externalCycles。
    // 临时性插桩，profiler 完成后整体删除。
    const u64 _t0 = profiling::readTsc();
    const f64 r = noise->getValue(x, y, z);
    profiling::g_densityEvalAcc.externalCycles += (profiling::readTsc() - _t0);
    profiling::g_densityEvalAcc.externalCalls += 1;
    return r;
}

f64 jitWeirdSampler(
    const DensityEvalContext* ctx, u32 objIdx, WeirdType type, i32 x, i32 y, i32 z, f64 inputValue) noexcept
{
    const auto* noise = ctx->objects[objIdx].noise;
    MC_ASSERT_RELEASE_MSG(noise != nullptr, "jitWeirdSampler: noise object is null");
    const f64 r = eval_helpers::getRarity(type, inputValue);
    // A 类叶子外部调用计时（口径与解释器 evalImpl WeirdSampler case 一致）：getRarity 不计时，
    // 计时包住 noise->getValue(...)*r 表达式。
    const u64 _t0 = profiling::readTsc();
    const f64 result =
        std::abs(noise->getValue(static_cast<f64>(x) / r, static_cast<f64>(y) / r, static_cast<f64>(z) / r)) * r;
    profiling::g_densityEvalAcc.externalCycles += (profiling::readTsc() - _t0);
    profiling::g_densityEvalAcc.externalCalls += 1;
    return result;
}

f64 jitDelegate(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept
{
    const auto* df = ctx->objects[objIdx].densityFunction;
    MC_ASSERT_RELEASE_MSG(df != nullptr, "jitDelegate: density function is null");
    // A 类叶子外部调用计时（口径与解释器 evalImpl Delegate case 一致）。
    const u64 _t0 = profiling::readTsc();
    const f64 r = df->compute(x, y, z);
    profiling::g_densityEvalAcc.externalCycles += (profiling::readTsc() - _t0);
    profiling::g_densityEvalAcc.externalCalls += 1;
    return r;
}

f64 jitEndIslands(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept
{
    const auto* df = ctx->objects[objIdx].densityFunction;
    MC_ASSERT_RELEASE_MSG(df != nullptr, "jitEndIslands: density function is null");
    // A 类叶子外部调用计时（口径与解释器 evalImpl EndIslands case 一致）。
    const u64 _t0 = profiling::readTsc();
    const f64 r = df->compute(x, y, z);
    profiling::g_densityEvalAcc.externalCycles += (profiling::readTsc() - _t0);
    profiling::g_densityEvalAcc.externalCalls += 1;
    return r;
}

f64 jitBeardifier(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept
{
    // 维度级编译期 Beardifier 未注入（区块特定），占位返回 0.0（与解释器一致）。
    const auto* beardifier = ctx->objects[objIdx].beardifier;
    // A 类叶子外部调用计时（口径与解释器 evalImpl Beardifier case 一致）。
    const u64 _t0 = profiling::readTsc();
    const f64 r = (beardifier != nullptr) ? beardifier->compute(x, y, z) : 0.0;
    profiling::g_densityEvalAcc.externalCycles += (profiling::readTsc() - _t0);
    profiling::g_densityEvalAcc.externalCalls += 1;
    return r;
}

f64 jitCacheCompute(const DensityEvalContext* ctx, u32 objIdx, i32 x, i32 y, i32 z) noexcept
{
    // MARKER 区块级缓存对象 compute（虚调用，缓存未命中时经 Adapter 回 eval 递归）。
    const auto* cacheObj = ctx->objects[objIdx].densityFunction;
    MC_ASSERT_RELEASE_MSG(cacheObj != nullptr, "jitCacheCompute: cache object is null");
    return cacheObj->compute(x, y, z);
}

f64 jitMarkerDispatch(const DensityEvalContext* ctx, u32 objIdx, u32 subIdx, i32 x, i32 y, i32 z) noexcept
{
    // MARKER 运行时判空：区块级走缓存对象 compute，维度级透传 delegate 子求值器 eval。
    // 对齐解释器 Marker case（CompiledDensityFunction.cpp），维度级与区块级共享同一 JIT 代码。
    const auto* cacheObj = ctx->objects[objIdx].densityFunction;
    if (cacheObj != nullptr) {
        return cacheObj->compute(x, y, z);
    }
    const auto& delegate = ctx->subEvaluators[subIdx];
    MC_ASSERT_RELEASE_MSG(delegate != nullptr, "jitMarkerDispatch: delegate sub-evaluator is null");
    return delegate->eval(x, y, z);
}

f64 jitDelegateSubEval(const DensityEvalContext* ctx, u32 subIdx, i32 x, i32 y, i32 z) noexcept
{
    const auto& sub = ctx->subEvaluators[subIdx];
    MC_ASSERT_RELEASE_MSG(sub != nullptr, "jitDelegateSubEval: sub-evaluator is null");
    return sub->eval(x, y, z);
}

f64 jitSpline(const DensityEvalContext* ctx, u32 objIdx, f64 point, i32 x, i32 y, i32 z) noexcept
{
    const auto& spline = ctx->splines[objIdx];
    MC_ASSERT_RELEASE_MSG(spline != nullptr, "jitSpline: spline data is null");
    return eval_helpers::evalSpline(*spline, point, x, y, z);
}

f64 jitFindTopSurface(
    const DensityEvalContext* ctx, u32 subIdx, f64 upper, i32 lowerBound, i32 cellHeight, i32 x, i32 z) noexcept
{
    const auto& densitySub = ctx->subEvaluators[subIdx];
    MC_ASSERT_RELEASE_MSG(densitySub != nullptr, "jitFindTopSurface: density sub-evaluator is null");
    return eval_helpers::evalFindTopSurface(*densitySub, upper, lowerBound, cellHeight, x, z);
}

f64 jitSin(f64 v) noexcept
{
    return std::sin(v);
}

f64 jitCos(f64 v) noexcept
{
    return std::cos(v);
}

} // namespace mc::world::gen::density::ast
