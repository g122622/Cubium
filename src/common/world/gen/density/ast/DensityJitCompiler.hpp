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
// 密度函数求值器 JIT 编译器（asmjit，仅 Windows x64）
//
// 把 CompiledDensityFunction 的扁平指令序列（std::vector<Op>）翻译成原生机器码，
// 消除解释器的 switch 分发 / Op 取指 / regs 间接寻址开销（可行性评估测得该开销
// 占 eval 总耗时 65.4%，见 docs/iterations/密度函数求值器JIT可行性评估.md）。
//
// 翻译策略：逐条 Op 顺序翻译为 asmjit 虚拟 xmm 寄存器指令，asmjit Compiler 不做
// IR 优化（无 SSA/常量传播/DCE/重排），只做寄存器分配（线性扫描 + liveness），
// 故浮点累加顺序与解释器逐条一致 → 数值 bit-exact（1e-9 基线）。
//
// 外部调用（NoiseSample/Delegate/Marker 缓存/Spline/FindTopSurface/SharedSubtree）
// 经 cc.invoke 调用 DensityJitTrampolines 的自由函数，JIT 代码完全不碰 C++ 对象布局。
//
// 平台：仅 Windows x64 落地（x86::Compiler + SSE2/SSE4.1 标量指令）。
//       macOS ARM64 留 TODO，非 Win x64 平台 compileDensityJit 返回 nullptr 自动回退解释器。
//
// 头文件刻意不 include asmjit（避免 asmjit 头污染上层编译单元），asmjit 用法全部
// 封装在 DensityJitCompiler.cpp 内。
// ============================================================================

#include "common/core/Types.hpp"
#include "common/world/gen/density/ast/CompiledDensityFunction.hpp" // Op/OpCode/RuntimeObject
#include "common/world/gen/density/ast/DensityJitTrampolines.hpp"   // DensityEvalContext

#include <vector>

namespace mc::world::gen::density::ast {

// DensityEvalContext / DensityJitFn 定义在 CompiledDensityFunction.hpp（打破循环 include），
// 本头经上方 include 已可见。

/// JIT 编译入口：把 Op 序列翻译成机器码并加入全局 JIT 运行时。
///
/// \param ops 指令序列（维度级与区块级字节相同，故 JIT 产物可跨实例共享）。
/// \param regCount 寄存器 slot 数（每 slot 一个虚拟 xmm）。
/// \return JIT 函数指针；编译失败或非 Win x64 平台返回 nullptr（调用方回退解释器）。
///
/// 失败原因：asmjit 任一步 Error、Op 序列含未识别指令等——记 spdlog::warn（英文）后返回 nullptr。
/// JIT 机器码内存由进程级 JitRuntime 单例持有至进程结束（求值器不可变，无需 release）。
[[nodiscard]] DensityJitFn compileDensityJit(const std::vector<Op>& ops, u32 regCount) noexcept;

} // namespace mc::world::gen::density::ast
