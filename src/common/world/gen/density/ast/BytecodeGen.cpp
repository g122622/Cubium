/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom this Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software, *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/gen/density/ast/BytecodeGen.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/ast/AstNodes.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <limits>
#include <unordered_map>

namespace mc::world::gen::density::ast {

namespace {

using Ptr = AstNode::Ptr;

/// 编译期寄存器分配结果：寄存器 slot 或内联常量。
/// 常量节点返回 isConst=true，父节点可把 constValue 内联到 Op 的 imm 字段（对齐 DFC
/// ValuesMethodDef.isConst 的常量折叠到调用点，避免独立 LOAD_CONST 指令）。
struct RegOrConst {
    bool isConst = false;
    f64 constValue = 0.0;
    u32 reg = 0;

    [[nodiscard]] static RegOrConst ofConst(f64 v) { return RegOrConst{true, v, 0}; }
    [[nodiscard]] static RegOrConst ofReg(u32 r) { return RegOrConst{false, 0.0, r}; }
};

/// 构造一个返回常量 value 的最小求值器（LOAD_CONST reg0, value; RETURN reg0）。
/// 样条常量值点用此封装，与嵌套子样条求值器统一调度。
[[nodiscard]] std::shared_ptr<CompiledDensityFunction> makeConstantEvaluator(f64 value)
{
    std::vector<Op> ops;
    Op load{};
    load.code = OpCode::LoadConst;
    load.dst = 0;
    load.imm = value;
    ops.push_back(load);
    Op ret{};
    ret.code = OpCode::Return;
    ret.dst = 0;
    ops.push_back(ret);
    auto evaluator = std::make_shared<CompiledDensityFunction>(std::move(ops),
        1,
        std::vector<RuntimeObject>{},
        std::vector<std::shared_ptr<CompiledDensityFunction>>{},
        std::vector<std::shared_ptr<CompiledSpline>>{},
        value,
        value,
        false,
        std::vector<std::unique_ptr<::mc::world::gen::density::DensityFunction>>{});
    // 常量求值器亦 JIT（2 指令，JIT 后 eval 直走机器码，省 switch 分发）。
    evaluator->compileJit();
    return evaluator;
}

/// 编译上下文。递归 emit AST 节点，累积 Op 序列 + 寄存器分配 + 运行时对象登记。
/// m_hasMarkerOrBeardifier 标记编译期是否产生 MARKER/BEARDIFIER 指令（emitMarker/emitBeardifier
/// 置 true，子求值器继承），驱动区块级 newInstance 是否递归替换。
class GenContext {
public:
    /// 编译 root，产出求值器。minValue/maxValue 从原 DensityFunction 取（编译期记录，
    /// 供 CompiledDensityFunctionAdapter::minValue/maxValue 返回）。
    std::shared_ptr<CompiledDensityFunction> compile(const Ptr& root, f64 minValue, f64 maxValue)
    {
        const RegOrConst result = emitNode(root);
        // 末尾 RETURN 指令：把根结果载入返回寄存器。
        const u32 retReg = asReg(result);
        Op ret{};
        ret.code = OpCode::Return;
        ret.dst = retReg;
        m_ops.push_back(ret);

        auto evaluator = std::make_shared<CompiledDensityFunction>(std::move(m_ops),
            m_nextReg,
            std::move(m_objects),
            std::move(m_subEvaluators),
            std::move(m_splines),
            minValue,
            maxValue,
            m_hasMarkerOrBeardifier,
            std::vector<std::unique_ptr<::mc::world::gen::density::DensityFunction>>{});
        // 维度级 JIT 编译一次：子求值器（Marker delegate / Spline value / SharedSubtree /
        // FindTopSurface density）经各自 BytecodeGen::compile 递归进入本函数，已各自编译。
        // 区块级 newInstance 复用本维度级 m_jitFn（ops 字节相同），不重复编译。
        evaluator->compileJit();
        return evaluator;
    }

private:
    std::vector<Op> m_ops;
    u32 m_nextReg = 0;
    std::vector<RuntimeObject> m_objects;
    std::vector<std::shared_ptr<CompiledDensityFunction>> m_subEvaluators;
    std::vector<std::shared_ptr<CompiledSpline>> m_splines;
    /// 是否含 MARKER/BEARDIFIER 指令（emitMarker/emitBeardifier 置 true，子求值器继承）。
    bool m_hasMarkerOrBeardifier = false;
    /// 地址级子树去重：同一 shared_ptr<AstNode> 共享同一寄存器（避免重复编译）。
    std::unordered_map<const AstNode*, RegOrConst> m_shared;
    /// SharedSubtree 去重：同 subTreeId 的共享子树共享同一子求值器（避免重复编译）。
    std::unordered_map<u64, u32> m_sharedSubtreeIdx;

    /// 分配一个新寄存器 slot。
    [[nodiscard]] u32 allocReg() { return m_nextReg++; }

    /// 若 RegOrConst 是常量，发射 LOAD_CONST 到新寄存器并返回该寄存器；否则返回已有寄存器。
    [[nodiscard]] u32 asReg(const RegOrConst& v)
    {
        if (v.isConst) {
            const u32 r = allocReg();
            Op op{};
            op.code = OpCode::LoadConst;
            op.dst = r;
            op.imm = v.constValue;
            m_ops.push_back(op);
            return r;
        }
        return v.reg;
    }

    /// 登记运行时对象，返回 objIdx。
    [[nodiscard]] u32 addObject(RuntimeObject obj)
    {
        const u32 idx = static_cast<u32>(m_objects.size());
        m_objects.push_back(obj);
        return idx;
    }

    /// 登记子求值器，返回 subIdx。继承父的 hasMarkerOrBeardifier（子求值器独立编译时
    /// 自身 emitMarker/emitBeardifier 也会置位，此处合并）。
    [[nodiscard]] u32 addSubEvaluator(std::shared_ptr<CompiledDensityFunction> sub)
    {
        if (sub != nullptr && sub->hasMarkerOrBeardifier()) {
            m_hasMarkerOrBeardifier = true;
        }
        const u32 idx = static_cast<u32>(m_subEvaluators.size());
        m_subEvaluators.push_back(std::move(sub));
        return idx;
    }

    /// 登记样条数据，返回 splines 索引（存入 Op.objIdx，因 SPLINE 用 objIdx 索引 m_splines）。
    [[nodiscard]] u32 addSpline(std::shared_ptr<CompiledSpline> spline)
    {
        const u32 idx = static_cast<u32>(m_splines.size());
        m_splines.push_back(std::move(spline));
        return idx;
    }

    /// 递归 emit 一个 AST 节点，返回结果（常量或寄存器）。
    RegOrConst emitNode(const Ptr& node)
    {
        if (!node) {
            spdlog::warn("BytecodeGen: null AST node, treating as constant 0.0");
            return RegOrConst::ofConst(0.0);
        }

        // 地址级子树去重：同一 shared_ptr 共享。
        const auto it = m_shared.find(node.get());
        if (it != m_shared.end()) {
            return it->second;
        }

        RegOrConst result = emitNodeImpl(node);
        m_shared[node.get()] = result;
        return result;
    }

    /// 按节点 kind 分发 emit。
    RegOrConst emitNodeImpl(const Ptr& node)
    {
        switch (node->kind()) {
            case AstNodeKind::Constant:
                return RegOrConst::ofConst(static_cast<const ConstantNode*>(node.get())->value());
            case AstNodeKind::ConstantF32:
                // F32 常量：CubicSpline 全 f64（核查确认），按 f64 值内联。
                return RegOrConst::ofConst(static_cast<f64>(static_cast<const ConstantF32Node*>(node.get())->value()));
            case AstNodeKind::Coordinate:
                return emitCoordinate(node);
            case AstNodeKind::YClampedGradient:
                return emitYGradient(node);
            case AstNodeKind::EndIslands:
                return emitEndIslands(node);
            case AstNodeKind::GenericShiftedNoise:
                return emitGenericShiftedNoise(node);
            case AstNodeKind::WeirdScaledSampler:
                return emitWeirdScaledSampler(node);
            case AstNodeKind::Spline:
                return emitSpline(node);
            case AstNodeKind::Abs:
            case AstNodeKind::Square:
            case AstNodeKind::Cube:
            case AstNodeKind::Squeeze:
            case AstNodeKind::Sqrt:
            case AstNodeKind::Sin:
            case AstNodeKind::Cos:
            case AstNodeKind::Floor:
            case AstNodeKind::Ceil:
                return emitUnary(node);
            case AstNodeKind::NegMul:
                return emitNegMul(node);
            case AstNodeKind::Add:
            case AstNodeKind::Mul:
            case AstNodeKind::Div:
            case AstNodeKind::Max:
            case AstNodeKind::Min:
                return emitBinary(node);
            case AstNodeKind::MaxShort:
                return emitMaxShort(node);
            case AstNodeKind::MinShort:
                return emitMinShort(node);
            case AstNodeKind::Clamp:
                return emitClamp(node);
            case AstNodeKind::Lerp:
                return emitLerp(node);
            case AstNodeKind::RangeChoice:
                return emitRangeChoice(node);
            case AstNodeKind::Marker:
                return emitMarker(node);
            case AstNodeKind::Beardifier:
                return emitBeardifier();
            case AstNodeKind::SharedSubtreeRef:
                return emitSharedSubtreeRef(node);
            case AstNodeKind::FindTopSurface:
                return emitFindTopSurface(node);
            case AstNodeKind::Delegate:
                return emitDelegate(node);
            case AstNodeKind::BlendedNoise:
            case AstNodeKind::MappedNoise:
                // 阶段2 TODO：暂走 Delegate 退化（McToAst 已把它们包 DelegateNode，此处不应到达）。
                spdlog::warn(
                    "BytecodeGen: unhandled node kind {}, falling back to delegate", static_cast<int>(node->kind()));
                return RegOrConst::ofConst(0.0);
        }
        MC_ASSERT_RELEASE_MSG(false, "BytecodeGen: unhandled AstNodeKind");
        return RegOrConst::ofConst(0.0);
    }

    // ---- 叶子 ----

    RegOrConst emitCoordinate(const Ptr& node)
    {
        const auto* n = static_cast<const CoordinateNode*>(node.get());
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Coord;
        op.dst = dst;
        // 高 4 位存 Axis（与 eval 的 unpackAxis 读高 4 位一致：opFlags = axis << 4）。
        op.opFlags = static_cast<u8>(static_cast<u8>(n->axis()) << 4);
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitYGradient(const Ptr& node)
    {
        const auto* n = static_cast<const YClampedGradientNode*>(node.get());
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::YGradient;
        op.dst = dst;
        // eval 调用 clampedMap(y, fromY, toY, fromValue, toValue)（对齐 YClampedGradient::compute）。
        // 字段映射：imm3=fromY, imm4=toY, imm=fromValue, imm2=toValue。
        op.imm = n->fromValue();
        op.imm2 = n->toValue();
        op.imm3 = static_cast<f64>(n->fromY());
        op.imm4 = static_cast<f64>(n->toY());
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitEndIslands(const Ptr& node)
    {
        const auto* n = static_cast<const EndIslandsNode*>(node.get());
        const u32 objIdx = addObject(RuntimeObject{nullptr, n->endIslands(), nullptr});
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::EndIslands;
        op.dst = dst;
        op.objIdx = objIdx;
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitGenericShiftedNoise(const Ptr& node)
    {
        const auto* n = static_cast<const GenericShiftedNoiseNode*>(node.get());
        const u32 rx = asReg(emitNode(n->inputX()));
        const u32 ry = asReg(emitNode(n->inputY()));
        const u32 rz = asReg(emitNode(n->inputZ()));
        const u32 objIdx = addObject(RuntimeObject{n->noise(), nullptr, nullptr});
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::NoiseSample;
        op.dst = dst;
        op.srcA = rx;
        op.srcB = ry;
        op.srcC = rz;
        op.objIdx = objIdx;
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitWeirdScaledSampler(const Ptr& node)
    {
        const auto* n = static_cast<const WeirdScaledSamplerNode*>(node.get());
        const u32 rInput = asReg(emitNode(n->input()));
        const u32 objIdx = addObject(RuntimeObject{n->noise(), nullptr, nullptr});
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::WeirdSampler;
        op.dst = dst;
        op.srcA = rInput;
        op.objIdx = objIdx;
        // 低 4 位存 WeirdType（0=Type1, 1=Type2）。
        op.opFlags = static_cast<u8>(n->type() == WeirdType::Type1 ? 0 : 1);
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitDelegate(const Ptr& node)
    {
        const auto* n = static_cast<const DelegateNode*>(node.get());
        const u32 objIdx = addObject(RuntimeObject{nullptr, n->densityFunction(), nullptr});
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Delegate;
        op.dst = dst;
        op.objIdx = objIdx;
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitBeardifier()
    {
        // 方案X：Beardifier 不进编译产物（留 NoiseChunk OOP 层手工组装）。
        // 维度级 finalDensity 树不含 BeardifierMarker，故生产路径 BEARDIFIER 指令不触发。
        // 保留供 McToAst 映射 BeardifierNode 的完整性，登记 nullptr 占位，eval 返回 0.0。
        m_hasMarkerOrBeardifier = true;
        const u32 objIdx = addObject(RuntimeObject{nullptr, nullptr, nullptr});
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Beardifier;
        op.dst = dst;
        op.objIdx = objIdx;
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    // ---- 一元 ----

    RegOrConst emitUnary(const Ptr& node)
    {
        const auto* n = static_cast<const UnaryNode*>(node.get());
        const u32 src = asReg(emitNode(n->operand()));

        // 常量操作数折叠：一元运算在编译期直接求值（对齐 OptoPasses FoldConstants，避免运行时指令）。
        // 注：OptoPasses 已做过常量折叠，此处 node->operand() 通常非常量；保留折叠为防御性兜底。
        // （emitNode 已对常量返回 RegOrConst::ofConst，此处读 src 寄存器，不再折叠以保持简洁。）

        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Unary;
        op.dst = dst;
        op.srcA = src;
        op.opFlags = static_cast<u8>(static_cast<u8>(unaryOpOf(node->kind())));
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitNegMul(const Ptr& node)
    {
        const auto* n = static_cast<const NegMulNode*>(node.get());
        const u32 src = asReg(emitNode(n->operand()));
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::NegMul;
        op.dst = dst;
        op.srcA = src;
        op.imm = n->negMul();
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    // ---- 二元 ----

    RegOrConst emitBinary(const Ptr& node)
    {
        const auto* binary = static_cast<const BinaryNode*>(node.get());
        const u32 ra = asReg(emitNode(binary->left()));
        const u32 rb = asReg(emitNode(binary->right()));
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Binary;
        op.dst = dst;
        op.srcA = ra;
        op.srcB = rb;
        op.opFlags = static_cast<u8>(static_cast<u8>(binaryOpOf(node->kind())));
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    // MaxShort/MinShort 短路编译：先求 left，若 left 已超过 right 的编译期界限（rightMax/rightMin）
    // 则跳过 right 子树求值直接取 left（max/min 数学性质保证数值一致）。对齐 vanilla
    // DensityFunctions.Ap2：Max 当 d0 > arg2.maxValue() 取 d0；Min 当 d0 < arg2.minValue() 取 d0。
    // rightMax/rightMin 是 right 子树编译期 maxValue/minValue（McToAst 生成 Short 变体时内联），
    // 跳过 right 段省其全部指令（含 noise 采样，单次采样远贵于寄存器运算）。
    RegOrConst emitMaxShort(const Ptr& node)
    {
        const auto* n = static_cast<const MaxShortNode*>(node.get());
        const u32 dst = allocReg();
        const u32 leftReg = asReg(emitNode(n->left()));
        // 占位条件跳转：left > rightMax → 短路段（跳过 right）。
        const size_t jumpIfIdx = m_ops.size();
        Op jumpIf{};
        jumpIf.code = OpCode::JumpIfCmp;
        jumpIf.srcA = leftReg;
        jumpIf.imm = n->rightMax();
        jumpIf.opFlags = static_cast<u8>(CmpOp::Gt);
        m_ops.push_back(jumpIf); // jumpTarget 待回填
        // right 段：求 right，BINARY Max 写 dst。
        const u32 rightReg = asReg(emitNode(n->right()));
        Op bin{};
        bin.code = OpCode::Binary;
        bin.dst = dst;
        bin.srcA = leftReg;
        bin.srcB = rightReg;
        bin.opFlags = static_cast<u8>(BinaryOp::Max);
        m_ops.push_back(bin);
        // 跳到 end（占位）。
        const size_t jumpEndIdx = m_ops.size();
        Op jumpEnd{};
        jumpEnd.code = OpCode::Jump;
        m_ops.push_back(jumpEnd); // jumpTarget 待回填
        // 短路段：dst = left（left > rightMax ≥ right，故 max(left,right)=left）。
        const size_t shortCircuitIdx = m_ops.size();
        emitStoreToReg(RegOrConst::ofReg(leftReg), dst);
        const size_t endIdx = m_ops.size();
        // 回填跳转目标（目标 Op 下标；eval 用 --pc 抵消 ++pc）。
        m_ops[jumpIfIdx].jumpTarget = static_cast<u32>(shortCircuitIdx);
        m_ops[jumpEndIdx].jumpTarget = static_cast<u32>(endIdx);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitMinShort(const Ptr& node)
    {
        const auto* n = static_cast<const MinShortNode*>(node.get());
        const u32 dst = allocReg();
        const u32 leftReg = asReg(emitNode(n->left()));
        // 占位条件跳转：left < rightMin → 短路段（跳过 right）。
        const size_t jumpIfIdx = m_ops.size();
        Op jumpIf{};
        jumpIf.code = OpCode::JumpIfCmp;
        jumpIf.srcA = leftReg;
        jumpIf.imm = n->rightMin();
        jumpIf.opFlags = static_cast<u8>(CmpOp::Lt);
        m_ops.push_back(jumpIf); // jumpTarget 待回填
        // right 段：求 right，BINARY Min 写 dst。
        const u32 rightReg = asReg(emitNode(n->right()));
        Op bin{};
        bin.code = OpCode::Binary;
        bin.dst = dst;
        bin.srcA = leftReg;
        bin.srcB = rightReg;
        bin.opFlags = static_cast<u8>(BinaryOp::Min);
        m_ops.push_back(bin);
        // 跳到 end（占位）。
        const size_t jumpEndIdx = m_ops.size();
        Op jumpEnd{};
        jumpEnd.code = OpCode::Jump;
        m_ops.push_back(jumpEnd); // jumpTarget 待回填
        // 短路段：dst = left（left < rightMin ≤ right，故 min(left,right)=left）。
        const size_t shortCircuitIdx = m_ops.size();
        emitStoreToReg(RegOrConst::ofReg(leftReg), dst);
        const size_t endIdx = m_ops.size();
        m_ops[jumpIfIdx].jumpTarget = static_cast<u32>(shortCircuitIdx);
        m_ops[jumpEndIdx].jumpTarget = static_cast<u32>(endIdx);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitClamp(const Ptr& node)
    {
        const auto* n = static_cast<const ClampNode*>(node.get());
        const u32 src = asReg(emitNode(n->input()));
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Clamp;
        op.dst = dst;
        op.srcA = src;
        op.imm = n->min();
        op.imm2 = n->max();
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    RegOrConst emitLerp(const Ptr& node)
    {
        const auto* n = static_cast<const LerpNode*>(node.get());
        const u32 rd = asReg(emitNode(n->delta()));
        const u32 rs = asReg(emitNode(n->start()));
        const u32 re = asReg(emitNode(n->end()));
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Lerp;
        op.dst = dst;
        op.srcA = rd;
        op.srcB = rs;
        op.srcC = re;
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    // ---- 控制 ----

    RegOrConst emitRangeChoice(const Ptr& node)
    {
        const auto* n = static_cast<const RangeChoiceNode*>(node.get());
        const u32 rInput = asReg(emitNode(n->input()));
        const u32 dst = allocReg();

        // 结构：RANGE_CHOICE（占位跳转）→ whenInRange 段 → JUMP end（占位）→
        //       whenOutOfRange 段 → end。
        // 两段各自把结果归一到 dst（emitStoreToReg：常量发 LOAD_CONST，寄存器发 COPY）。
        // 跳转目标直接是目标 Op 下标；eval 用 --pc 抵消 for 的 ++pc，故 jumpTarget = 目标下标。
        const size_t branchOpIdx = m_ops.size();
        Op branch{};
        branch.code = OpCode::RangeChoice;
        branch.dst = dst;
        branch.srcA = rInput;
        branch.imm = n->minInclusive();
        branch.imm2 = n->maxExclusive();
        m_ops.push_back(branch); // jumpTarget/jumpTarget2 待回填

        // whenInRange 段：求值并归一到 dst。
        const RegOrConst inRange = emitNode(n->whenInRange());
        emitStoreToReg(inRange, dst);
        // 跳过 whenOutOfRange 段到 end（占位，回填）。
        const size_t jumpEndIdx = m_ops.size();
        Op jumpEnd{};
        jumpEnd.code = OpCode::Jump;
        m_ops.push_back(jumpEnd); // jumpTarget 待回填

        // whenOutOfRange 段：求值并归一到 dst。
        const RegOrConst outRange = emitNode(n->whenOutOfRange());
        emitStoreToReg(outRange, dst);

        const size_t endIdx = m_ops.size();

        // 回填跳转目标（直接是目标 Op 下标）。
        m_ops[branchOpIdx].jumpTarget = static_cast<u32>(branchOpIdx + 1); // whenInRange 段起点
        m_ops[branchOpIdx].jumpTarget2 = static_cast<u32>(jumpEndIdx + 1); // whenOutOfRange 段起点
        m_ops[jumpEndIdx].jumpTarget = static_cast<u32>(endIdx);           // 跳到 end

        return RegOrConst::ofReg(dst);
    }

    /// 把 RegOrConst 的值归一到指定寄存器 dst。
    /// 常量发 LOAD_CONST(dst, constValue)；寄存器且 != dst 发 COPY(dst, reg)；reg == dst 不操作。
    void emitStoreToReg(const RegOrConst& v, u32 dst)
    {
        if (v.isConst) {
            Op op{};
            op.code = OpCode::LoadConst;
            op.dst = dst;
            op.imm = v.constValue;
            m_ops.push_back(op);
        } else if (v.reg != dst) {
            Op op{};
            op.code = OpCode::Copy;
            op.dst = dst;
            op.srcA = v.reg;
            m_ops.push_back(op);
        }
        // v.reg == dst：无需操作。
    }

    // ---- Marker ----

    RegOrConst emitMarker(const Ptr& node)
    {
        const auto* n = static_cast<const MarkerNode*>(node.get());
        // delegate 子树独立编译为子求值器（对齐 emitSharedSubtreeRef/emitFindTopSurface 模式）。
        // 维度级 MARKER 透传 delegate 子求值器 eval；区块级 newInstance 据 markerType 把缓存对象
        // 注入 m_objects[objIdx].densityFunction，eval 检测非空走缓存对象 compute。
        // delegate 的 minValue/maxValue 取其原 DF 子树的值（Marker 透传，min/max == delegate 的 min/max）。
        const auto delegateSub = BytecodeGen::compile(n->delegate(), n->delegateMinValue(), n->delegateMaxValue());
        const u32 subIdx = addSubEvaluator(std::move(delegateSub));

        // objIdx 占位：维度级 densityFunction==nullptr（占位），区块级 newInstance 注入缓存对象。
        const u32 objIdx = addObject(RuntimeObject{nullptr, nullptr, nullptr});
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Marker;
        op.dst = dst;
        op.objIdx = objIdx;
        op.subIdx = subIdx;
        // 高 4 位存 MarkerType（低 4 位未用）；newInstance 据此创建对应缓存对象。
        op.opFlags = static_cast<u8>(static_cast<u8>(n->markerType()) << 4);
        m_ops.push_back(op);
        m_hasMarkerOrBeardifier = true;
        return RegOrConst::ofReg(dst);
    }

    // ---- 样条 ----

    RegOrConst emitSpline(const Ptr& node)
    {
        const auto* n = static_cast<const SplineNode*>(node.get());
        const u32 rLocation = asReg(emitNode(n->locationFunction()));

        // 预编译样条数据：locations/derivatives + 每个控制点值的子求值器。
        auto splineData = std::make_shared<CompiledSpline>();
        const auto& points = n->points();
        splineData->locations.reserve(points.size());
        splineData->derivatives.reserve(points.size());
        splineData->valueEvaluators.reserve(points.size());
        for (const auto& p : points) {
            splineData->locations.push_back(p.location);
            splineData->derivatives.push_back(p.derivative);
            if (std::holds_alternative<f64>(p.value)) {
                // 常量值点：编译为单条 LOAD_CONST + RETURN 的求值器。
                splineData->valueEvaluators.push_back(makeConstantEvaluator(std::get<f64>(p.value)));
            } else {
                // 嵌套子样条：递归编译为独立求值器。子样条经 evalSpline 调 eval，不经 Adapter，
                // min/max 无人消费，用哨兵值（子求值器无原 DF 可取 min/max）。
                const Ptr& childSpline = std::get<Ptr>(p.value);
                splineData->valueEvaluators.push_back(BytecodeGen::compile(
                    childSpline, std::numeric_limits<f64>::lowest(), std::numeric_limits<f64>::max()));
            }
        }

        const u32 splineIdx = addSpline(std::move(splineData));
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::Spline;
        op.dst = dst;
        op.srcA = rLocation;
        op.objIdx = splineIdx; // SPLINE 用 objIdx 索引 m_splines（见 eval）
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    // ---- SharedSubtreeRef ----

    RegOrConst emitSharedSubtreeRef(const Ptr& node)
    {
        const auto* n = static_cast<const SharedSubtreeRefNode*>(node.get());
        // subTreeId 去重：同一 SharedTopology 子树（同 subTreeId）共享同一子求值器，
        // 避免重复编译（对齐 DFC relaxedEquals 子树共享同段子程序）。
        const auto it = m_sharedSubtreeIdx.find(n->subTreeId());
        u32 subIdx;
        if (it != m_sharedSubtreeIdx.end()) {
            subIdx = it->second;
        } else {
            // 内部子树独立编译为子求值器（inner 不参与优化 pass，保持共享不可变语义）。
            // SharedSubtree 经 SHARED_SUBTREE_CALL 调 eval，不经 Adapter，min/max 无人消费，用哨兵。
            auto sub =
                BytecodeGen::compile(n->inner(), std::numeric_limits<f64>::lowest(), std::numeric_limits<f64>::max());
            subIdx = addSubEvaluator(std::move(sub));
            m_sharedSubtreeIdx[n->subTreeId()] = subIdx;
        }
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::SharedSubtreeCall;
        op.dst = dst;
        op.subIdx = subIdx;
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    // ---- FindTopSurface ----

    RegOrConst emitFindTopSurface(const Ptr& node)
    {
        const auto* n = static_cast<const FindTopSurfaceNode*>(node.get());
        // density 子树独立编译为子求值器（循环内多次求值，独立求值器复用）。
        // FindTopSurface density 经 FIND_TOP_SURFACE 调 eval，不经 Adapter，min/max 无人消费，用哨兵。
        auto densitySub =
            BytecodeGen::compile(n->density(), std::numeric_limits<f64>::lowest(), std::numeric_limits<f64>::max());
        const u32 subIdx = addSubEvaluator(std::move(densitySub));
        const u32 rUpper = asReg(emitNode(n->upperBound()));
        const u32 dst = allocReg();
        Op op{};
        op.code = OpCode::FindTopSurface;
        op.dst = dst;
        op.srcA = rUpper;
        op.subIdx = subIdx;
        op.imm = static_cast<f64>(n->lowerBound());
        op.imm2 = static_cast<f64>(n->cellHeight());
        m_ops.push_back(op);
        return RegOrConst::ofReg(dst);
    }

    // ---- 辅助 ----

    [[nodiscard]] static UnaryOp unaryOpOf(AstNodeKind kind) noexcept
    {
        switch (kind) {
            case AstNodeKind::Abs:
                return UnaryOp::Abs;
            case AstNodeKind::Square:
                return UnaryOp::Square;
            case AstNodeKind::Cube:
                return UnaryOp::Cube;
            case AstNodeKind::Squeeze:
                return UnaryOp::Squeeze;
            case AstNodeKind::Sqrt:
                return UnaryOp::Sqrt;
            case AstNodeKind::Sin:
                return UnaryOp::Sin;
            case AstNodeKind::Cos:
                return UnaryOp::Cos;
            case AstNodeKind::Floor:
                return UnaryOp::Floor;
            case AstNodeKind::Ceil:
                return UnaryOp::Ceil;
            default:
                MC_ASSERT_RELEASE_MSG(false, "not a unary kind");
                return UnaryOp::Abs;
        }
    }

    [[nodiscard]] static BinaryOp binaryOpOf(AstNodeKind kind) noexcept
    {
        switch (kind) {
            case AstNodeKind::Add:
                return BinaryOp::Add;
            case AstNodeKind::Mul:
                return BinaryOp::Mul;
            case AstNodeKind::Div:
                return BinaryOp::Div;
            case AstNodeKind::Max:
            case AstNodeKind::MaxShort:
                return BinaryOp::Max;
            case AstNodeKind::Min:
            case AstNodeKind::MinShort:
                return BinaryOp::Min;
            default:
                MC_ASSERT_RELEASE_MSG(false, "not a binary kind");
                return BinaryOp::Add;
        }
    }
};

} // namespace

std::shared_ptr<CompiledDensityFunction> BytecodeGen::compile(const Ptr& root, f64 minValue, f64 maxValue)
{
    GenContext ctx;
    return ctx.compile(root, minValue, maxValue);
}

} // namespace mc::world::gen::density::ast
