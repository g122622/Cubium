/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/world/gen/density/ast/DensityJitCompiler.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/density/ast/AstNodes.hpp" // WeirdType
#include "common/world/gen/density/ast/DensityEvalHelpers.hpp"

#include <spdlog/spdlog.h>

// 仅 Windows x64 启用 JIT。其它平台（macOS ARM64 等）compileDensityJit 直接返回 nullptr 回退解释器。
// TODO: macOS ARM64 用 a64::Compiler 落地 JIT（注意避免 fmadd 融合以保证与 x64 bit-exact）。
#if defined(_WIN32) && (defined(__x86_64__) || defined(_M_X64))
#define MC_DENSITY_JIT_ENABLED 1
#else
#define MC_DENSITY_JIT_ENABLED 0
#endif

#if MC_DENSITY_JIT_ENABLED

#include <asmjit/x86.h>

#include <unordered_set>

// 注意：不使用 `using namespace asmjit`——asmjit::Error（uint32_t typedef）会与项目 mc::Error
// （Result.hpp 的类）在非限定查找时冲突。故全部用 asmjit:: 显式限定。

namespace mc::world::gen::density::ast {

namespace {

/// 进程级 JitRuntime 单例（JitAllocator 线程安全，多 worker 并行 add 安全）。
/// 持有所有 JIT 机器码内存至进程结束（求值器不可变，无需 release）。
[[nodiscard]] asmjit::JitRuntime& jitRuntime()
{
    static asmjit::JitRuntime rt;
    return rt;
}

// ============================================================================
// Op → asmjit 翻译器（适配 vcpkg 版 asmjit，camelCase API）
// ============================================================================
class OpTranslator {
public:
    OpTranslator(asmjit::x86::Compiler& cc,
        const std::vector<Op>& ops,
        u32 regCount,
        const asmjit::x86::Gp& ctxGp,
        const asmjit::x86::Gp& xGp,
        const asmjit::x86::Gp& yGp,
        const asmjit::x86::Gp& zGp)
        : m_cc(cc)
        , m_ops(ops)
        , m_ctxGp(ctxGp)
        , m_xGp(xGp)
        , m_yGp(yGp)
        , m_zGp(zGp)
    {
        m_regs.resize(regCount);
    }

    /// 执行翻译。返回 asmjit Error（kErrorOk 表示成功）。
    [[nodiscard]] asmjit::Error translate()
    {
        // 1. 为每个 reg slot 创建虚拟 xmm 寄存器。
        for (u32 i = 0; i < m_regs.size(); ++i) {
            m_regs[i] = m_cc.newXmmSd();
        }

        // 2. 收集所有跳转目标 pc，预创建 label（asmjit label 必须先 new）。
        std::unordered_set<u32> jumpTargets;
        for (const Op& op : m_ops) {
            switch (op.code) {
                case OpCode::Jump:
                    jumpTargets.insert(op.jumpTarget);
                    break;
                case OpCode::JumpIfCmp:
                    jumpTargets.insert(op.jumpTarget);
                    break;
                case OpCode::RangeChoice:
                    jumpTargets.insert(op.jumpTarget);
                    jumpTargets.insert(op.jumpTarget2);
                    break;
                default:
                    break;
            }
        }
        m_labels.resize(m_ops.size());
        for (u32 pc = 0; pc < m_ops.size(); ++pc) {
            if (jumpTargets.count(pc) != 0) {
                m_labels[pc] = m_cc.newLabel();
            }
        }

        // 3. 逐条翻译。
        for (u32 pc = 0; pc < m_ops.size(); ++pc) {
            if (m_labels[pc].isValid()) {
                ASMJIT_PROPAGATE(m_cc.bind(m_labels[pc]));
            }
            ASMJIT_PROPAGATE(emitOp(m_ops[pc]));
        }
        return asmjit::kErrorOk;
    }

private:
    asmjit::x86::Compiler& m_cc;
    const std::vector<Op>& m_ops;
    const asmjit::x86::Gp& m_ctxGp;
    const asmjit::x86::Gp& m_xGp;
    const asmjit::x86::Gp& m_yGp;
    const asmjit::x86::Gp& m_zGp;
    std::vector<asmjit::x86::Xmm> m_regs;
    std::vector<asmjit::Label> m_labels;

    /// 加载 f64 常量到指定 xmm（常量池自动管理）。
    asmjit::Error loadConst(const asmjit::x86::Xmm& dst, f64 value)
    {
        asmjit::x86::Mem mem = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, value);
        return m_cc.movsd(dst, mem);
    }

    /// 取坐标轴对应的 i32 Gp（Coord 指令用）。
    [[nodiscard]] const asmjit::x86::Gp& axisGp(RegAxis axis) const noexcept
    {
        return (axis == RegAxis::X) ? m_xGp : (axis == RegAxis::Y) ? m_yGp : m_zGp;
    }

    /// x64 二元算术（两操作数，dst 被覆盖）：先 movsd(dst,a) 再 op(dst,b)。
    template <typename EmitFn>
    asmjit::Error binary2(
        const asmjit::x86::Xmm& dst, const asmjit::x86::Xmm& a, const asmjit::x86::Xmm& b, EmitFn emit)
    {
        ASMJIT_PROPAGATE(m_cc.movsd(dst, a));
        return emit(dst, b);
    }

    /// 发起 trampoline 调用，返回存放结果的临时 xmm（调用方负责使用）。
    /// 签名固定为 f64 ret；参数由 argsFn 逐个 setArg。
    template <typename ArgsFn>
    asmjit::Error invokeTrampoline(
        asmjit::x86::Xmm& retOut, const void* fnPtr, const asmjit::FuncSignature& sig, ArgsFn argsFn)
    {
        asmjit::x86::Xmm ret = m_cc.newXmmSd();
        asmjit::InvokeNode* call = nullptr;
        ASMJIT_PROPAGATE(m_cc.invoke(&call, asmjit::imm(fnPtr), sig));
        ASMJIT_PROPAGATE(argsFn(*call));
        call->setRet(0, ret);
        retOut = ret;
        return asmjit::kErrorOk;
    }

    /// 比较 v 与 imm（f64），NaN 时跳到 nanLabel（排 NaN），满足 cond 跳到 takenLabel。
    /// cmpOp: Gt(v>imm 用 ja)/Lt(v<imm 用 jb)。
    /// 语义对齐 C++（NaN 时比较为 false，即不跳 takenLabel）。
    asmjit::Error compareAndJump(
        const asmjit::x86::Xmm& v, f64 imm, CmpOp cmpOp, const asmjit::Label& takenLabel, const asmjit::Label& nanLabel)
    {
        asmjit::x86::Mem immMem = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, imm);
        ASMJIT_PROPAGATE(m_cc.ucomisd(v, immMem));
        // NaN 时 PF=1，先跳 nanLabel（排除 NaN，保证 C++ 语义 NaN→false）。
        ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kP, nanLabel));
        switch (cmpOp) {
            case CmpOp::Gt:
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kA, takenLabel)); // v > imm
                break;
            case CmpOp::Lt:
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kB, takenLabel)); // v < imm
                break;
        }
        return asmjit::kErrorOk;
    }

    asmjit::Error emitOp(const Op& op)
    {
        switch (op.code) {
            case OpCode::Return:
                return m_cc.ret(m_regs[op.dst]);
            case OpCode::LoadConst:
                return loadConst(m_regs[op.dst], op.imm);
            case OpCode::Coord: {
                const RegAxis axis = static_cast<RegAxis>((op.opFlags >> 4) & 0x0F);
                return m_cc.cvtsi2sd(m_regs[op.dst], axisGp(axis));
            }
            case OpCode::YGradient: {
                // clampedMap(y, imm3=fromY, imm4=toY, imm=fromValue, imm2=toValue)。
                // 走 trampoline 复用 eval_helpers::clampedMap，与解释器逐位一致，避免 JIT 内 NaN 分支。
                // 注意：asmjit invoke 不支持 Imm 直接作为 f64 参数（moveImmToRegArg 仅处理整型 TypeId，
                // f64 走 default 返回 kErrorInvalidAssignment）。故先把 4 个 f64 常量加载到临时 Xmm，
                // 再用 Xmm 传参。
                asmjit::x86::Xmm fromY = m_cc.newXmmSd();
                asmjit::x86::Xmm toY = m_cc.newXmmSd();
                asmjit::x86::Xmm fromValue = m_cc.newXmmSd();
                asmjit::x86::Xmm toValue = m_cc.newXmmSd();
                ASMJIT_PROPAGATE(loadConst(fromY, op.imm3));
                ASMJIT_PROPAGATE(loadConst(toY, op.imm4));
                ASMJIT_PROPAGATE(loadConst(fromValue, op.imm));
                ASMJIT_PROPAGATE(loadConst(toValue, op.imm2));
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitYGradient),
                    asmjit::FuncSignature::build<f64, i32, f64, f64, f64, f64>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_yGp);
                        call.setArg(1, fromY);
                        call.setArg(2, toY);
                        call.setArg(3, fromValue);
                        call.setArg(4, toValue);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::NoiseSample: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitNoiseSample),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, f64, f64, f64>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.objIdx));
                        call.setArg(2, m_regs[op.srcA]);
                        call.setArg(3, m_regs[op.srcB]);
                        call.setArg(4, m_regs[op.srcC]);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::WeirdSampler: {
                const WeirdType type = (op.opFlags & 0x0F) == 0 ? WeirdType::Type1 : WeirdType::Type2;
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitWeirdSampler),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, WeirdType, i32, i32, i32, f64>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.objIdx));
                        call.setArg(2, asmjit::imm(static_cast<u32>(type)));
                        call.setArg(3, m_xGp);
                        call.setArg(4, m_yGp);
                        call.setArg(5, m_zGp);
                        call.setArg(6, m_regs[op.srcA]);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::Delegate: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitDelegate),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, i32, i32, i32>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.objIdx));
                        call.setArg(2, m_xGp);
                        call.setArg(3, m_yGp);
                        call.setArg(4, m_zGp);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::EndIslands: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitEndIslands),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, i32, i32, i32>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.objIdx));
                        call.setArg(2, m_xGp);
                        call.setArg(3, m_yGp);
                        call.setArg(4, m_zGp);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::Beardifier: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitBeardifier),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, i32, i32, i32>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.objIdx));
                        call.setArg(2, m_xGp);
                        call.setArg(3, m_yGp);
                        call.setArg(4, m_zGp);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::SharedSubtreeCall: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitDelegateSubEval),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, i32, i32, i32>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.subIdx));
                        call.setArg(2, m_xGp);
                        call.setArg(3, m_yGp);
                        call.setArg(4, m_zGp);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::Unary:
                return emitUnary(op);
            case OpCode::NegMul: {
                // input <= 0 ? input*negMul : input
                asmjit::x86::Xmm v = m_regs[op.srcA];
                asmjit::x86::Xmm dst = m_regs[op.dst];
                asmjit::x86::Mem zero = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, 0.0);
                asmjit::Label mulBranch = m_cc.newLabel();
                asmjit::Label end = m_cc.newLabel();
                ASMJIT_PROPAGATE(m_cc.ucomisd(v, zero));
                // v <= 0 → mul 分支。NaN: v<=0 为 false → 走 else（input）。
                // jbe 在 NaN 时 CF=1 会跳，需先排 NaN。
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kP, end));        // NaN → else（input）
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kBE, mulBranch)); // v <= 0
                // else: dst = input
                ASMJIT_PROPAGATE(m_cc.movsd(dst, v));
                ASMJIT_PROPAGATE(m_cc.jmp(end));
                ASMJIT_PROPAGATE(m_cc.bind(mulBranch));
                ASMJIT_PROPAGATE(m_cc.movsd(dst, v));
                asmjit::x86::Mem negMul = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, op.imm);
                ASMJIT_PROPAGATE(m_cc.mulsd(dst, negMul));
                ASMJIT_PROPAGATE(m_cc.bind(end));
                return asmjit::kErrorOk;
            }
            case OpCode::Clamp: {
                // std::clamp(src, imm, imm2) = max(lo, min(src, hi))。
                // 实现：dst = min(src, hi); dst = max(dst, lo)。NaN 语义与 std::clamp 一致
                // （std::clamp 用 < 比较，NaN 比较为 false；minsd/maxsd 的 NaN 语义不同，但密度值无 NaN 路径）。
                // TODO: Clamp 的 NaN 语义未严格对齐 std::clamp（minsd/maxsd vs std::clamp 的 < 比较），
                //       密度函数树不产生 NaN 故无影响；若未来出现 NaN 路径需改用 ucomisd 分支实现。
                asmjit::x86::Xmm dst = m_regs[op.dst];
                asmjit::x86::Mem lo = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, op.imm);
                asmjit::x86::Mem hi = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, op.imm2);
                ASMJIT_PROPAGATE(m_cc.movsd(dst, m_regs[op.srcA]));
                ASMJIT_PROPAGATE(m_cc.minsd(dst, hi));
                return m_cc.maxsd(dst, lo);
            }
            case OpCode::Binary:
                return emitBinary(op);
            case OpCode::Lerp: {
                // clampedLerp(delta, start, end): delta<=0→start, delta>=1→end, else start+delta*(end-start)
                asmjit::x86::Xmm delta = m_regs[op.srcA];
                asmjit::x86::Xmm start = m_regs[op.srcB];
                asmjit::x86::Xmm endV = m_regs[op.srcC];
                asmjit::x86::Xmm dst = m_regs[op.dst];
                asmjit::x86::Mem zero = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, 0.0);
                asmjit::x86::Mem one = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, 1.0);
                asmjit::Label useStart = m_cc.newLabel();
                asmjit::Label useEnd = m_cc.newLabel();
                asmjit::Label doLerp = m_cc.newLabel();
                asmjit::Label end = m_cc.newLabel();
                // delta <= 0 → start
                ASMJIT_PROPAGATE(m_cc.ucomisd(delta, zero));
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kP, doLerp)); // NaN → lerp（C++ delta<=0 NaN 为 false）
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kBE, useStart));
                // delta >= 1 → end
                ASMJIT_PROPAGATE(m_cc.ucomisd(delta, one));
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kP, doLerp)); // NaN → lerp
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kAE, useEnd));
                // else: start + delta*(end-start)
                ASMJIT_PROPAGATE(m_cc.bind(doLerp));
                ASMJIT_PROPAGATE(m_cc.movsd(dst, endV));
                ASMJIT_PROPAGATE(m_cc.subsd(dst, start)); // end-start
                ASMJIT_PROPAGATE(m_cc.mulsd(dst, delta)); // delta*(end-start)
                ASMJIT_PROPAGATE(m_cc.addsd(dst, start)); // start+...
                ASMJIT_PROPAGATE(m_cc.jmp(end));
                ASMJIT_PROPAGATE(m_cc.bind(useStart));
                ASMJIT_PROPAGATE(m_cc.movsd(dst, start));
                ASMJIT_PROPAGATE(m_cc.jmp(end));
                ASMJIT_PROPAGATE(m_cc.bind(useEnd));
                ASMJIT_PROPAGATE(m_cc.movsd(dst, endV));
                ASMJIT_PROPAGATE(m_cc.bind(end));
                return asmjit::kErrorOk;
            }
            case OpCode::RangeChoice: {
                // input ∈ [imm, imm2) → jumpTarget（inRange），否则 jumpTarget2（outOfRange）
                asmjit::x86::Xmm input = m_regs[op.srcA];
                asmjit::Label inRange = m_labels[op.jumpTarget];
                asmjit::Label outOfRange = m_labels[op.jumpTarget2];
                asmjit::x86::Mem immMem = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, op.imm);
                asmjit::x86::Mem imm2Mem = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, op.imm2);
                // 先判 input < imm（含 NaN）→ outOfRange
                ASMJIT_PROPAGATE(m_cc.ucomisd(input, immMem));
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kP, outOfRange)); // NaN → outOfRange
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kB, outOfRange)); // input < imm → outOfRange
                // 再判 input >= imm2 → outOfRange
                ASMJIT_PROPAGATE(m_cc.ucomisd(input, imm2Mem));
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kP, outOfRange));  // NaN → outOfRange
                ASMJIT_PROPAGATE(m_cc.j(asmjit::x86::CondCode::kAE, outOfRange)); // input >= imm2 → outOfRange
                // inRange
                return m_cc.jmp(inRange);
            }
            case OpCode::Jump:
                return m_cc.jmp(m_labels[op.jumpTarget]);
            case OpCode::JumpIfCmp: {
                // cmpOp Gt: v>imm 跳；Lt: v<imm 跳。NaN 不跳（C++ 语义）。
                const CmpOp cmpOp = (op.opFlags & 0x0F) == static_cast<u8>(CmpOp::Gt) ? CmpOp::Gt : CmpOp::Lt;
                asmjit::Label target = m_labels[op.jumpTarget];
                asmjit::Label skip = m_cc.newLabel();
                ASMJIT_PROPAGATE(compareAndJump(m_regs[op.srcA], op.imm, cmpOp, target, skip));
                ASMJIT_PROPAGATE(m_cc.bind(skip));
                return asmjit::kErrorOk;
            }
            case OpCode::Copy:
                return m_cc.movsd(m_regs[op.dst], m_regs[op.srcA]);
            case OpCode::Marker: {
                // MARKER 维度级（cacheObj==null 透传 delegate）与区块级（cacheObj!=null 走缓存）
                // 共享同一 JIT 代码：判空在 C++ trampoline 内（jitMarkerDispatch）。
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitMarkerDispatch),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, u32, i32, i32, i32>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.objIdx));
                        call.setArg(2, asmjit::imm(op.subIdx));
                        call.setArg(3, m_xGp);
                        call.setArg(4, m_yGp);
                        call.setArg(5, m_zGp);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::Spline: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitSpline),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, f64, i32, i32, i32>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.objIdx));
                        call.setArg(2, m_regs[op.srcA]);
                        call.setArg(3, m_xGp);
                        call.setArg(4, m_yGp);
                        call.setArg(5, m_zGp);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
            case OpCode::FindTopSurface: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitFindTopSurface),
                    asmjit::FuncSignature::build<f64, const DensityEvalContext*, u32, f64, i32, i32, i32, i32>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, m_ctxGp);
                        call.setArg(1, asmjit::imm(op.subIdx));
                        call.setArg(2, m_regs[op.srcA]);
                        call.setArg(3, asmjit::imm(static_cast<i32>(op.imm)));  // lowerBound
                        call.setArg(4, asmjit::imm(static_cast<i32>(op.imm2))); // cellHeight
                        call.setArg(5, m_xGp);
                        call.setArg(6, m_zGp);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(m_regs[op.dst], ret);
            }
        }
        // 未识别 OpCode（不应发生，BytecodeGen 保证）。
        return asmjit::kErrorInvalidState;
    }

    asmjit::Error emitUnary(const Op& op)
    {
        const UnaryOp uop = static_cast<UnaryOp>(op.opFlags & 0x0F);
        const asmjit::x86::Xmm& v = m_regs[op.srcA];
        asmjit::x86::Xmm dst = m_regs[op.dst];
        switch (uop) {
            case UnaryOp::Abs: {
                // 清符号位取绝对值。掩码 0x7FFFFFFFFFFFFFFF 先经 movsd(标量,8 字节对齐足矣)
                // 加载到 Xmm 寄存器,再用寄存器-寄存器 andps。切勿直接 andps xmm,[mem]:
                // ANDPS 是 SSE packed 指令,其 m128 内存操作数要求 16 字节对齐,asmjit 常量池
                // 的 int64 条目仅 8 字节对齐,未对齐访存会触发 #GP(VEH 报 fault_addr 为垃圾值)。
                ASMJIT_PROPAGATE(m_cc.movsd(dst, v));
                asmjit::x86::Xmm mask = m_cc.newXmmSd();
                asmjit::x86::Mem maskMem =
                    m_cc.newInt64Const(asmjit::ConstPoolScope::kLocal, static_cast<i64>(0x7FFFFFFFFFFFFFFFull));
                ASMJIT_PROPAGATE(m_cc.movsd(mask, maskMem));
                return m_cc.andps(dst, mask);
            }
            case UnaryOp::Square: {
                ASMJIT_PROPAGATE(m_cc.movsd(dst, v));
                return m_cc.mulsd(dst, v);
            }
            case UnaryOp::Cube: {
                ASMJIT_PROPAGATE(m_cc.movsd(dst, v));
                ASMJIT_PROPAGATE(m_cc.mulsd(dst, v));
                return m_cc.mulsd(dst, v);
            }
            case UnaryOp::Squeeze: {
                // c = clamp(v,-1,1); return c/2 - c*c*c/24
                asmjit::x86::Xmm c = m_cc.newXmmSd();
                ASMJIT_PROPAGATE(m_cc.movsd(c, v));
                asmjit::x86::Mem one = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, 1.0);
                asmjit::x86::Mem negOne = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, -1.0);
                ASMJIT_PROPAGATE(m_cc.minsd(c, one));
                ASMJIT_PROPAGATE(m_cc.maxsd(c, negOne));
                // dst = c/2
                asmjit::x86::Mem half = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, 0.5);
                ASMJIT_PROPAGATE(m_cc.movsd(dst, c));
                ASMJIT_PROPAGATE(m_cc.mulsd(dst, half));
                // c3 = c*c*c
                asmjit::x86::Xmm c3 = m_cc.newXmmSd();
                ASMJIT_PROPAGATE(m_cc.movsd(c3, c));
                ASMJIT_PROPAGATE(m_cc.mulsd(c3, c));
                ASMJIT_PROPAGATE(m_cc.mulsd(c3, c));
                // c3/24
                asmjit::x86::Mem inv24 = m_cc.newDoubleConst(asmjit::ConstPoolScope::kLocal, 1.0 / 24.0);
                ASMJIT_PROPAGATE(m_cc.mulsd(c3, inv24));
                // dst = c/2 - c3/24
                return m_cc.subsd(dst, c3);
            }
            case UnaryOp::Sqrt:
                return m_cc.sqrtsd(dst, v);
            case UnaryOp::Sin: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitSin),
                    asmjit::FuncSignature::build<f64, f64>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, v);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(dst, ret);
            }
            case UnaryOp::Cos: {
                asmjit::x86::Xmm ret;
                ASMJIT_PROPAGATE(invokeTrampoline(ret,
                    reinterpret_cast<const void*>(&jitCos),
                    asmjit::FuncSignature::build<f64, f64>(),
                    [&](asmjit::InvokeNode& call) -> asmjit::Error {
                        call.setArg(0, v);
                        return asmjit::kErrorOk;
                    }));
                return m_cc.movsd(dst, ret);
            }
            case UnaryOp::Floor:
                return m_cc.roundsd(dst, v, uint32_t(asmjit::x86::RoundImm::kDown));
            case UnaryOp::Ceil:
                return m_cc.roundsd(dst, v, uint32_t(asmjit::x86::RoundImm::kUp));
        }
        return asmjit::kErrorInvalidState;
    }

    asmjit::Error emitBinary(const Op& op)
    {
        const BinaryOp bop = static_cast<BinaryOp>(op.opFlags & 0x0F);
        const asmjit::x86::Xmm& a = m_regs[op.srcA];
        const asmjit::x86::Xmm& b = m_regs[op.srcB];
        asmjit::x86::Xmm dst = m_regs[op.dst];
        switch (bop) {
            case BinaryOp::Add:
                return binary2(
                    dst, a, b, [&](const asmjit::x86::Xmm& d, const asmjit::x86::Xmm& s) { return m_cc.addsd(d, s); });
            case BinaryOp::Mul:
                return binary2(
                    dst, a, b, [&](const asmjit::x86::Xmm& d, const asmjit::x86::Xmm& s) { return m_cc.mulsd(d, s); });
            case BinaryOp::Div:
                return binary2(
                    dst, a, b, [&](const asmjit::x86::Xmm& d, const asmjit::x86::Xmm& s) { return m_cc.divsd(d, s); });
            case BinaryOp::Max:
                return binary2(
                    dst, a, b, [&](const asmjit::x86::Xmm& d, const asmjit::x86::Xmm& s) { return m_cc.maxsd(d, s); });
            case BinaryOp::Min:
                return binary2(
                    dst, a, b, [&](const asmjit::x86::Xmm& d, const asmjit::x86::Xmm& s) { return m_cc.minsd(d, s); });
        }
        return asmjit::kErrorInvalidState;
    }
};

} // namespace

DensityJitFn compileDensityJit(const std::vector<Op>& ops, u32 regCount) noexcept
{
    if (ops.empty()) {
        return nullptr;
    }
    try {
        asmjit::JitRuntime& rt = jitRuntime();
        asmjit::CodeHolder code;
        asmjit::Error err = code.init(rt.environment(), rt.cpuFeatures());
        if (err != asmjit::kErrorOk) {
            spdlog::warn("density JIT: CodeHolder init failed (asmjit error {})", static_cast<int>(err));
            return nullptr;
        }
        asmjit::x86::Compiler cc(&code);

        asmjit::FuncNode* funcNode =
            cc.addFunc(asmjit::FuncSignature::build<f64, const DensityEvalContext*, i32, i32, i32>());
        asmjit::x86::Gp ctxGp = cc.newIntPtr("ctx");
        asmjit::x86::Gp xGp = cc.newInt32("x");
        asmjit::x86::Gp yGp = cc.newInt32("y");
        asmjit::x86::Gp zGp = cc.newInt32("z");
        funcNode->setArg(0, ctxGp);
        funcNode->setArg(1, xGp);
        funcNode->setArg(2, yGp);
        funcNode->setArg(3, zGp);

        OpTranslator translator(cc, ops, regCount, ctxGp, xGp, yGp, zGp);
        err = translator.translate();
        if (err != asmjit::kErrorOk) {
            spdlog::warn("density JIT: translation failed (asmjit error {})", static_cast<int>(err));
            return nullptr;
        }

        err = cc.endFunc();
        if (err != asmjit::kErrorOk) {
            spdlog::warn("density JIT: endFunc failed (asmjit error {})", static_cast<int>(err));
            return nullptr;
        }
        err = cc.finalize();
        if (err != asmjit::kErrorOk) {
            spdlog::warn("density JIT: finalize failed (asmjit error {})", static_cast<int>(err));
            return nullptr;
        }

        DensityJitFn fn = nullptr;
        err = rt.add(&fn, &code);
        if (err != asmjit::kErrorOk || fn == nullptr) {
            spdlog::warn("density JIT: rt.add failed (asmjit error {})", static_cast<int>(err));
            return nullptr;
        }
        return fn;
    }
    catch (const std::exception& e) {
        spdlog::warn("density JIT: exception during compilation ({})", e.what());
        return nullptr;
    }
}

} // namespace mc::world::gen::density::ast

#else // !MC_DENSITY_JIT_ENABLED

// 非 Windows x64 平台（macOS ARM64 等）：JIT 暂未实现，返回 nullptr 回退解释器。
// TODO: macOS ARM64 用 a64::Compiler 落地 JIT（注意避免 fmadd 融合以保证与 x64 bit-exact）。

namespace mc::world::gen::density::ast {

DensityJitFn compileDensityJit(const std::vector<Op>& /*ops*/, u32 /*regCount*/) noexcept
{
    return nullptr;
}

} // namespace mc::world::gen::density::ast

#endif // MC_DENSITY_JIT_ENABLED
