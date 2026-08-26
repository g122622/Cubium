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

#include "common/world/gen/density/ast/OptoPasses.hpp"

#include "common/world/gen/density/ast/AstNodes.hpp"

#include <cmath>

namespace mc::world::gen::density::ast {

namespace {

using Ptr = AstNode::Ptr;

/// 判定 +0.0（对齐 DFC ZeroUtils.isPositiveZero）。x==0.0 且符号位为正。
/// 用于 Add 折叠的 -0.0 符号处理：+0.0 + x 会抹掉 x 的 -0.0 符号，故仅 -0.0 才可安全消去。
[[nodiscard]] bool isPositiveZero(f64 x) noexcept
{
    return x == 0.0 && !std::signbit(x);
}

/// 判定节点是否为 ConstantNode（F64 常量）。返回常量值出参。
[[nodiscard]] bool isConstant(const Ptr& node, f64& outValue) noexcept
{
    if (node && node->kind() == AstNodeKind::Constant) {
        outValue = static_cast<const ConstantNode*>(node.get())->value();
        return true;
    }
    return false;
}

// ============================================================================
// Pass 1: TreeNormalization
// 可交换二元节点把常量规范到左边（对齐 DFC TreeNormalization）。
// 仅当 right 是 Constant 且 left 不是 Constant 时交换。Min/Max/MaxShort/MinShort
// canSwapOperandsSafely()=false 不参与（BinaryNode::canSwapOperandsSafely 守卫）。
// ============================================================================

class TreeNormalization final : public AstTransformer {
public:
    Ptr transform(Ptr node) override
    {
        if (node && node->kind() != AstNodeKind::Constant) {
            // 所有可交换二元节点都是 BinaryNode 子类，canSwapOperandsSafely() 守护交换安全性。
            auto* binary = dynamic_cast<const BinaryNode*>(node.get());
            if (binary != nullptr && binary->canSwapOperandsSafely()) {
                const bool rightIsConst = (binary->right()->kind() == AstNodeKind::Constant);
                const bool leftIsConst = (binary->left()->kind() == AstNodeKind::Constant);
                if (rightIsConst && !leftIsConst) {
                    // fp add/mul/div 可交换：常量规范到左边，为 FoldConstants 的"常量在左"约定铺路。
                    return binary->newInstance(binary->right(), binary->left());
                }
            }
        }
        return node;
    }
};

// ============================================================================
// Pass 2: FoldConstants
// 常量折叠（对齐 DFC FoldConstants + Cubium 特有节点扩展）。
// ============================================================================

class FoldConstants final : public AstTransformer {
public:
    Ptr transform(Ptr node) override
    {
        if (!node) {
            return node;
        }
        switch (node->kind()) {
            case AstNodeKind::Add:
                return foldAdd(node);
            case AstNodeKind::Mul:
                return foldMul(node);
            case AstNodeKind::Div:
                return foldDiv(node);
            case AstNodeKind::Max:
                return foldMax(node);
            case AstNodeKind::Min:
                return foldMin(node);
            case AstNodeKind::MaxShort:
                return foldMaxShort(node);
            case AstNodeKind::MinShort:
                return foldMinShort(node);
            case AstNodeKind::Abs:
                return foldUnary(node, [](f64 v) { return std::abs(v); });
            case AstNodeKind::Square:
                return foldUnary(node, [](f64 v) { return v * v; });
            case AstNodeKind::Cube:
                return foldUnary(node, [](f64 v) { return v * v * v; });
            case AstNodeKind::Squeeze:
                return foldUnary(node, [](f64 v) {
                    const f64 c = std::clamp(v, -1.0, 1.0);
                    return c / 2.0 - c * c * c / 24.0;
                });
            case AstNodeKind::Sqrt:
                return foldUnary(node, [](f64 v) { return std::sqrt(v); });
            case AstNodeKind::Sin:
                return foldUnary(node, [](f64 v) { return std::sin(v); });
            case AstNodeKind::Cos:
                return foldUnary(node, [](f64 v) { return std::cos(v); });
            case AstNodeKind::Floor:
                return foldUnary(node, [](f64 v) { return std::floor(v); });
            case AstNodeKind::Ceil:
                return foldUnary(node, [](f64 v) { return std::ceil(v); });
            case AstNodeKind::NegMul:
                return foldNegMul(node);
            case AstNodeKind::Lerp:
                return foldLerp(node);
            case AstNodeKind::Clamp:
                return foldClamp(node);
            case AstNodeKind::Marker:
                return foldMarker(node);
            default:
                // 其余节点（Constant/Coordinate/Noise/Spline/RangeChoice/Beardifier/特有/Delegate）
                // 无常量可折叠，原样返回。
                return node;
        }
    }

private:
    /// 两常量取值；返回是否均为 Constant。
    [[nodiscard]] static bool bothConstant(const Ptr& a, const Ptr& b, f64& va, f64& vb) noexcept
    {
        return isConstant(a, va) && isConstant(b, vb);
    }

    [[nodiscard]] static Ptr constantNode(f64 v) { return std::make_shared<ConstantNode>(v); }

    // ---- Add ----
    [[nodiscard]] static Ptr foldAdd(const Ptr& node)
    {
        const auto* n = static_cast<const AddNode*>(node.get());
        f64 va, vb;
        if (bothConstant(n->left(), n->right(), va, vb)) {
            return constantNode(va + vb);
        }
        // TreeNormalization 后常量在左。+0.0 + x 会抹掉 x 的 -0.0 符号，仅 -0.0 可安全消去。
        f64 lc;
        if (isConstant(n->left(), lc) && lc == 0.0 && !isPositiveZero(lc)) {
            return n->right();
        }
        return node;
    }

    // ---- Mul ----
    [[nodiscard]] static Ptr foldMul(const Ptr& node)
    {
        const auto* n = static_cast<const MulNode*>(node.get());
        f64 va, vb;
        if (bothConstant(n->left(), n->right(), va, vb)) {
            return constantNode(va * vb);
        }
        f64 lc;
        if (isConstant(n->left(), lc)) {
            if (lc == 0.0) {
                // vanilla TwoArgument.Mul 的 v1==0.0 短路：x*0 → 0（含 -0.0 * x = -0.0 的微差，
                // 但 vanilla Mul 当 v1==0 直接返回 0.0 不计算 arg2，故折叠为 +0.0 对齐 vanilla）。
                return constantNode(0.0);
            }
            if (lc == 1.0) {
                return n->right();
            }
        }
        return node;
    }

    // ---- Div（Cubium 特有，Mapped(Invert)=1/x 来的；不可交换）----
    // compute = left / right（Invert 转换为 Div(Constant(1.0), input)）。
    [[nodiscard]] static Ptr foldDiv(const Ptr& node)
    {
        const auto* n = static_cast<const DivNode*>(node.get());
        f64 va, vb;
        if (bothConstant(n->left(), n->right(), va, vb)) {
            // 不折叠除零（保持与原版 1.0/0.0=+inf 语义一致的指令路径，避免常量 inf 干扰后续）。
            if (vb != 0.0) {
                return constantNode(va / vb);
            }
        }
        return node;
    }

    // ---- Max / Min ----
    [[nodiscard]] static Ptr foldMax(const Ptr& node)
    {
        const auto* n = static_cast<const MaxNode*>(node.get());
        f64 va, vb;
        if (bothConstant(n->left(), n->right(), va, vb)) {
            return constantNode(std::max(va, vb));
        }
        return node;
    }

    [[nodiscard]] static Ptr foldMin(const Ptr& node)
    {
        const auto* n = static_cast<const MinNode*>(node.get());
        f64 va, vb;
        if (bothConstant(n->left(), n->right(), va, vb)) {
            return constantNode(std::min(va, vb));
        }
        return node;
    }

    // ---- MaxShort / MinShort（对齐 DFC：常量操作数退化普通 Max/Min；左常量超界直接取左）----
    [[nodiscard]] static Ptr foldMaxShort(const Ptr& node)
    {
        const auto* n = static_cast<const MaxShortNode*>(node.get());
        f64 lc;
        // 左常量 >= rightMax 时 max(lc, anything<=rightMax) = lc。
        if (isConstant(n->left(), lc) && lc >= n->rightMax()) {
            return n->left();
        }
        // 任一操作数常量则短路信息无效，退化为普通 Max（relaxedEquals 不再含 rightMax）。
        if (n->left()->kind() == AstNodeKind::Constant || n->right()->kind() == AstNodeKind::Constant) {
            return std::make_shared<MaxNode>(n->left(), n->right());
        }
        return node;
    }

    [[nodiscard]] static Ptr foldMinShort(const Ptr& node)
    {
        const auto* n = static_cast<const MinShortNode*>(node.get());
        f64 lc;
        if (isConstant(n->left(), lc) && lc <= n->rightMin()) {
            return n->left();
        }
        if (n->left()->kind() == AstNodeKind::Constant || n->right()->kind() == AstNodeKind::Constant) {
            return std::make_shared<MinNode>(n->left(), n->right());
        }
        return node;
    }

    // ---- 一元折叠通用模板 ----
    using UnaryFn = f64 (*)(f64);

    [[nodiscard]] static Ptr foldUnary(const Ptr& node, UnaryFn fn)
    {
        const auto* n = static_cast<const UnaryNode*>(node.get());
        f64 v;
        if (isConstant(n->operand(), v)) {
            return constantNode(fn(v));
        }
        return node;
    }

    // ---- NegMul（compute = input <= 0 ? input*negMul : input）----
    [[nodiscard]] static Ptr foldNegMul(const Ptr& node)
    {
        const auto* n = static_cast<const NegMulNode*>(node.get());
        f64 v;
        if (isConstant(n->operand(), v)) {
            return constantNode(v > 0.0 ? v : v * n->negMul());
        }
        return node;
    }

    // ---- Lerp（compute = delta<=0?start : delta>=1?end : start+delta*(end-start)）----
    [[nodiscard]] static Ptr foldLerp(const Ptr& node)
    {
        const auto* n = static_cast<const LerpNode*>(node.get());
        f64 d, s, e;
        if (bothConstant(n->delta(), n->start(), d, s) && isConstant(n->end(), e)) {
            // 三参皆常量：直接求值。对齐 vanilla Lerp 的 clampedLerp 语义。
            if (d <= 0.0) {
                return constantNode(s);
            }
            if (d >= 1.0) {
                return constantNode(e);
            }
            return constantNode(s + d * (e - s));
        }
        f64 delta;
        if (isConstant(n->delta(), delta)) {
            if (delta <= 0.0) {
                return n->start();
            }
            if (delta >= 1.0) {
                return n->end();
            }
        }
        return node;
    }

    // ---- Clamp（compute = std::clamp(input, min, max)）----
    [[nodiscard]] static Ptr foldClamp(const Ptr& node)
    {
        const auto* n = static_cast<const ClampNode*>(node.get());
        f64 v;
        if (isConstant(n->input(), v)) {
            return constantNode(std::clamp(v, n->min(), n->max()));
        }
        return node;
    }

    // ---- Marker：若 delegate 折成常量，缓存常量无意义，整 Marker 折叠为该常量 ----
    // 对齐 DFC FoldConstants 的 CacheLikeNode 分支（c2me$isActualCache && delegate is Constant）。
    // Cubium 所有 MarkerType 都是实际缓存（无纯透传型），故 delegate 为常量即可折叠。
    [[nodiscard]] static Ptr foldMarker(const Ptr& node)
    {
        const auto* n = static_cast<const MarkerNode*>(node.get());
        if (n->delegate()->kind() == AstNodeKind::Constant) {
            return n->delegate();
        }
        return node;
    }
};

// ============================================================================
// Pass 3: BranchElimination
// RangeChoice 输入常量按区间选支；两支 relaxedEquals 相等则消分支（对齐 DFC）。
// ============================================================================

class BranchElimination final : public AstTransformer {
public:
    Ptr transform(Ptr node) override
    {
        if (!node || node->kind() != AstNodeKind::RangeChoice) {
            return node;
        }
        const auto* n = static_cast<const RangeChoiceNode*>(node.get());
        f64 v;
        if (isConstant(n->input(), v)) {
            if (v >= n->minInclusive() && v < n->maxExclusive()) {
                return n->whenInRange();
            }
            return n->whenOutOfRange();
        }
        // 两支拓扑等价则分支无意义（用 relaxedEquals 比 equals 更激进：只要结构相同结果必相同）。
        if (n->whenInRange()->relaxedEquals(*n->whenOutOfRange())) {
            return n->whenInRange();
        }
        return node;
    }
};

} // namespace

// ============================================================================
// OptoPasses::optimize — do-while 不动点（对齐 DFC OptoPasses.optimize0）
// ============================================================================

Ptr OptoPasses::optimize(Ptr root)
{
    TreeNormalization passNorm;
    FoldConstants passFold;
    BranchElimination passBranch;
    AstTransformer* const passes[] = {&passNorm, &passFold, &passBranch};

    Ptr res = root;
    Ptr prev;
    do {
        prev = res;
        for (AstTransformer* pass : passes) {
            res = res->transform(*pass);
        }
    } while (res != prev); // persistence sharing：引用未变即收敛
    return res;
}

} // namespace mc::world::gen::density::ast
