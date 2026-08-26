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
// 密度函数求值器 eval 性能计数器插桩（量化 JIT 是否值得）
//
// 目的：CompiledDensityFunction::eval 是区块生成逐方块热路径（每区块数万次），
// 当前为"扁平指令序列 + switch 解释执行"。本插桩量化 eval 总时间里
// "解释器开销"与"外部调用开销"的占比，判定用 asmjit 把 eval JIT 成机器码是否值得。
//
// JIT 只能消除解释器自身开销（switch 分发、Op 取指、regs 间接寻址、常量加载），
// 救不了 eval 内部的外部 C++ 调用（NoiseSample→NormalNoise::getValue、
// Delegate→df->compute、Marker→缓存对象 compute 等真正的噪声采样/缓存查表）。
//
// 核心方法（双桶差值法）：
//   interpreterCycles = topLevelCycles − externalCycles   （JIT 能优化的上限）
//   topLevelCycles    = depth==0 eval 入口→Return 的总周期（覆盖整棵树含递归层）
//   externalCycles    = 所有"叶子外部调用"指令的 per-call 计时累加
//
// 外部调用分类（防双重计数）：
//   A. 叶子外部调用（直接计时）：NoiseSample/WeirdSampler/Delegate/EndIslands/
//      Beardifier —— 执行流离开 evalImpl 进入外部 C++ 函数，不递归调 eval。
//   B. 递归外部调用（不计时）：SharedSubtreeCall/Spline/FindTopSurface/Marker —— 调
//      sub->eval 或 cacheObj->compute（缓存未命中时经 Adapter→eval 递归）进入子层
//      evalImpl，子树耗时已含在顶层 topLevelCycles 内，子树内叶子外部调用由子层计时
//      累加。父层再计时会双重计数。其 Hermite/循环控制/缓存查表部分归入解释器开销
//      （算入 interpreterCycles），这会高估 JIT 收益（缓存查表 JIT 救不了），给出 JIT
//      收益的乐观上界——若上界仍不值得，JIT 肯定不值得。
//      （Marker 区块级走 cacheObj->compute，缓存对象 compute 未命中时调 filler→eval
//       递归，故归 B 类；早先误归 A 类致 externalCycles 双重计数超过 totalCycles。）
//
// 决策规则：interpreterRatio = interpreterCycles / topLevelCycles
//   ≥0.30 值得 JIT；≥0.40 强烈值得；<0.20 不值得转其他方向（SIMD 批量化/噪声层）。
//
// 临时性插桩：profiler 完成后本文件及 eval 内插桩代码将整体删除，不设编译开关。
// 插桩只对 5 个真叶子外部调用（NoiseSample/WeirdSampler/Delegate/EndIslands/Beardifier）
// 计时，rdtsc ~10ns 占比 <5%，可接受；内部类指令（LoadConst/Coord/Binary/Copy/Unary，
// JIT 收益主体）与 B 类递归调用（SharedSubtreeCall/Spline/FindTopSurface/Marker）完全零
// 插桩，不污染其耗时。Marker 缓存查表开销归 interpreterCycles（高估 JIT 收益，乐观上界）。
// ============================================================================

#include "common/core/Types.hpp"
#include "common/world/gen/density/ast/CompiledDensityFunction.hpp" // OpCode

#if defined(_WIN32)
#include <intrin.h> // __rdtsc
#endif

namespace mc::world::gen::density::ast {
namespace profiling {

/// 跨平台读取 CPU 周期计数器（原始 cycle，不换算纳秒——ratio 是同量纲比值无需换算）。
/// - Windows x64：__rdtsc（MSVC intrin，~10ns/20-40 周期）。
/// - macOS ARM64：__builtin_readcyclecounter（Clang，读 cntvct_el0）。
/// 不用 rdtscp（会等前序指令退休，在外部调用前用会把延迟计入 t0 之前的串行化等待，失真）。
/// 加编译器屏障防止 rdtsc 被重排到被测代码之外，但不加 CPU mfence（会序列化流水线放大开销）。
[[nodiscard]] inline u64 readTsc() noexcept
{
#if defined(_WIN32)
    _ReadWriteBarrier();
    const u64 t = static_cast<u64>(__rdtsc());
    _ReadWriteBarrier();
    return t;
#elif defined(__aarch64__)
    // __builtin_readcyclecounter 读 cntvct_el0；若编译器不支持回退 0（不影响 ratio 计算
    // 的正确性，仅丢失精度，但 Clang on Apple Silicon 支持）。
#if __has_builtin(__builtin_readcyclecounter)
    asm volatile("" ::: "memory");
    const u64 t = static_cast<u64>(__builtin_readcyclecounter());
    asm volatile("" ::: "memory");
    return t;
#else
    return 0;
#endif
#else
    // 未知平台回退：返回 0，ratio 退化为 0（不影响程序正确性，仅无 profiling 数据）。
    return 0;
#endif
}

/// 判定 opcode 是否为"叶子外部调用"（A 类，需 per-call 计时）。
/// 见文件头注释的外部调用分类。constexpr 便于编译期优化掉分支。
/// 注：本函数当前仅作文档性分类判定，CompiledDensityFunction.cpp 的计时是按 case 标签
/// 硬编码（不调用本函数）；故两者须同步维护——此处列出 5 个真叶子，Marker 不在其中
/// （Marker 区块级 cacheObj->compute 因缓存未命中经 Adapter 递归回 eval，属 B 类不计时）。
[[nodiscard]] constexpr bool isLeafExternalCall(OpCode code) noexcept
{
    switch (code) {
        case OpCode::NoiseSample:
        case OpCode::WeirdSampler:
        case OpCode::Delegate:
        case OpCode::EndIslands:
        case OpCode::Beardifier:
            return true;
        default:
            return false;
    }
}

/// 每线程累加器。eval 递归调用时各层共享同一 thread_local 实例，靠 depth 守卫区分顶层/内层。
struct DensityEvalAccumulator {
    u64 topLevelCycles = 0; ///< depth==0 eval 入口→Return 累加（覆盖整棵树含递归层）
    u64 externalCycles = 0; ///< A 类叶子外部调用 per-call 计时累加（跨层不重不漏）
    u64 topCalls = 0;       ///< depth==0 eval 调用次数
    u64 externalCalls = 0;  ///< A 类叶子外部调用指令总次数
    u32 depth = 0;          ///< 当前递归深度（reset 不归零，跨 eval 调用维持）

    /// 重置统计字段（depth 不归零——它反映调用栈状态，非统计量）。
    void reset() noexcept
    {
        topLevelCycles = 0;
        externalCycles = 0;
        topCalls = 0;
        externalCalls = 0;
    }
};

/// 每线程累加器实例。多 worker 线程并行生成不同区块（UniversalWorkerPool），
/// 单区块内串行求值，故 thread_local 无锁安全。
inline thread_local DensityEvalAccumulator g_densityEvalAcc;

/// 递归深度 RAII 守卫。保证 depth 在任何返回路径（含异常/early-return/missing-Return
/// assert 失败）前 -- 回 0，防止顶层判定错乱。eval 函数体顶部构造。
class DepthGuard {
public:
    DepthGuard() noexcept { ++g_densityEvalAcc.depth; }
    ~DepthGuard() { --g_densityEvalAcc.depth; }
    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;
};

} // namespace profiling

/// 供上报点（NoiseChunkGenerator）访问的累加器引用。
[[nodiscard]] inline profiling::DensityEvalAccumulator& densityEvalAccumulator() noexcept
{
    return profiling::g_densityEvalAcc;
}

} // namespace mc::world::gen::density::ast
