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

/**
 * @file OptoPassesTest.cpp
 * @brief AST 优化 pass 结构正确性单元测试（阶段3）
 *
 * 验证 McToAst 转换 + OptoPasses 优化（TreeNormalization/FoldConstants/BranchElimination
 * do-while 不动点）的局部重写逻辑。通过构造小型纯标量 DF 子树，转 AST 后断言优化产物的
 * 结构（节点类型/常量值/子节点数）符合预期。
 *
 * 不做端到端数值等价测试：阶段3 尚无求值器（阶段4 才编译为扁平指令序列），AST 节点本身
 * 不能 compute。端到端数值等价由阶段4 完成后覆盖，且 DensityAstBaselineTest 作为整体
 * 数值回归门禁全程守护。
 *
 * 测试聚焦"常量折叠/单位消除/分支消除/常量规范化/-0.0 符号处理"等 pass 逻辑正确性，
 * 避免把 pass 错误带入阶段4 求值器编译。纯标量子树无需 NoiseChunk/RandomState/NormalNoise
 * 设施，保持单元测试轻量。
 */

#include "common/world/gen/density/ast/OptoPasses.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/density/ast/AstNode.hpp"
#include "common/world/gen/density/ast/AstNodes.hpp"
#include "common/world/gen/density/ast/McToAst.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <memory>

using namespace mc;
using namespace mc::world::gen::density;
using namespace mc::world::gen::density::ast;
namespace dfactory = mc::world::gen::density::factory;

namespace {

using AstPtr = AstNode::Ptr;

/// 断言节点是 ConstantNode 且值为 expected。
void expectConstant(const AstPtr& node, f64 expected)
{
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->kind(), AstNodeKind::Constant) << "expected ConstantNode";
    if (node->kind() == AstNodeKind::Constant) {
        const auto* c = static_cast<const ConstantNode*>(node.get());
        EXPECT_DOUBLE_EQ(c->value(), expected);
    }
}

/// 取节点左/右子节点（BinaryNode 系）。
const AstPtr& binaryLeft(const AstPtr& node)
{
    return static_cast<const BinaryNode*>(node.get())->left();
}
const AstPtr& binaryRight(const AstPtr& node)
{
    return static_cast<const BinaryNode*>(node.get())->right();
}

/// 转换 + 优化：DF → AST → OptoPasses::optimize。
AstPtr toOptimized(const DensityFunction& df)
{
    AstPtr ast = McToAst::convert(df);
    return OptoPasses::optimize(ast);
}

/// 构造一个非常量 DF（YClampedGradient，minValue=-1 maxValue=1），用作非常量操作数。
std::unique_ptr<DensityFunction> nonConstant()
{
    return dfactory::yClampedGradient(-64, 320, -1.0, 1.0);
}

} // namespace

// ============================================================================
// FoldConstants：两常量二元运算合并
// ============================================================================

TEST(OptoPassesTest, FoldAddTwoConstants)
{
    auto df = dfactory::add(dfactory::constant(2.0), dfactory::constant(3.0));
    expectConstant(toOptimized(*df), 5.0);
}

TEST(OptoPassesTest, FoldMulTwoConstants)
{
    auto df = dfactory::mul(dfactory::constant(4.0), dfactory::constant(5.0));
    expectConstant(toOptimized(*df), 20.0);
}

TEST(OptoPassesTest, FoldMaxTwoConstants)
{
    auto df = dfactory::max(dfactory::constant(2.0), dfactory::constant(7.0));
    expectConstant(toOptimized(*df), 7.0);
}

TEST(OptoPassesTest, FoldMinTwoConstants)
{
    auto df = dfactory::min(dfactory::constant(2.0), dfactory::constant(7.0));
    expectConstant(toOptimized(*df), 2.0);
}

// ============================================================================
// FoldConstants：单位消除与归零
// ============================================================================

TEST(OptoPassesTest, FoldMulByOne)
{
    // mul(X, 1.0) → TreeNormalization 规范常量到左 → mul(1.0, X) → FoldConstants 消 1.0 → X
    auto df = dfactory::mul(nonConstant(), dfactory::constant(1.0));
    const AstPtr opt = toOptimized(*df);
    ASSERT_NE(opt, nullptr);
    EXPECT_EQ(opt->kind(), AstNodeKind::YClampedGradient) << "mul(X,1.0) should reduce to X";
}

TEST(OptoPassesTest, FoldMulByZero)
{
    // mul(X, 0.0) → 规范到左 mul(0.0, X) → FoldConstants 归零 → Constant(0.0)
    auto df = dfactory::mul(nonConstant(), dfactory::constant(0.0));
    expectConstant(toOptimized(*df), 0.0);
}

TEST(OptoPassesTest, FoldAddZeroNegativeZero)
{
    // add(X, -0.0) → 规范到左 add(-0.0, X) → FoldConstants：-0.0 可安全消去 → X
    auto dfNeg = dfactory::add(nonConstant(), dfactory::constant(-0.0));
    const AstPtr optNeg = toOptimized(*dfNeg);
    ASSERT_NE(optNeg, nullptr);
    EXPECT_EQ(optNeg->kind(), AstNodeKind::YClampedGradient)
        << "add(X, -0.0) should reduce to X (-0.0 is safe to drop)";

    // add(X, +0.0) → 规范到左 add(+0.0, X) → FoldConstants：+0.0 会抹掉 X 的 -0.0 符号，不消
    auto dfPos = dfactory::add(nonConstant(), dfactory::constant(0.0));
    const AstPtr optPos = toOptimized(*dfPos);
    ASSERT_NE(optPos, nullptr);
    EXPECT_EQ(optPos->kind(), AstNodeKind::Add) << "add(X, +0.0) must NOT reduce (+0.0 + x drops -0.0 sign of x)";
    EXPECT_EQ(binaryLeft(optPos)->kind(), AstNodeKind::Constant);
    EXPECT_EQ(binaryRight(optPos)->kind(), AstNodeKind::YClampedGradient);
}

// ============================================================================
// FoldConstants：一元折叠
// ============================================================================

TEST(OptoPassesTest, FoldAbsConstant)
{
    auto df = dfactory::abs(dfactory::constant(-3.0));
    expectConstant(toOptimized(*df), 3.0);
}

TEST(OptoPassesTest, FoldSquareConstant)
{
    auto df = dfactory::square(dfactory::constant(4.0));
    expectConstant(toOptimized(*df), 16.0);
}

TEST(OptoPassesTest, FoldCubeConstant)
{
    auto df = dfactory::cube(dfactory::constant(3.0));
    expectConstant(toOptimized(*df), 27.0);
}

TEST(OptoPassesTest, FoldSqueezeConstant)
{
    // squeeze(0.5) = clamp(0.5,-1,1)/2 - clamp^3/24 = 0.25 - 0.125/24
    auto df = dfactory::squeeze(dfactory::constant(0.5));
    const f64 expected = 0.5 / 2.0 - 0.5 * 0.5 * 0.5 / 24.0;
    expectConstant(toOptimized(*df), expected);
}

// ============================================================================
// FoldConstants：NegMul（HalfNegative/QuarterNegative）
// compute = input > 0 ? input : input * negMul
// ============================================================================

TEST(OptoPassesTest, FoldHalfNegativePositive)
{
    // halfNegative(4.0) = 4.0 > 0 → 4.0
    auto df = dfactory::halfNegative(dfactory::constant(4.0));
    expectConstant(toOptimized(*df), 4.0);
}

TEST(OptoPassesTest, FoldHalfNegativeNegative)
{
    // halfNegative(-4.0) = -4.0 * 0.5 = -2.0
    auto df = dfactory::halfNegative(dfactory::constant(-4.0));
    expectConstant(toOptimized(*df), -2.0);
}

TEST(OptoPassesTest, FoldQuarterNegative)
{
    // quarterNegative(-4.0) = -4.0 * 0.25 = -1.0
    auto df = dfactory::quarterNegative(dfactory::constant(-4.0));
    expectConstant(toOptimized(*df), -1.0);
}

// ============================================================================
// FoldConstants：Div（Invert = 1/x → Div(Constant(1.0), input)）
// ============================================================================

TEST(OptoPassesTest, FoldInvertConstant)
{
    // invert(2.0) → Div(Constant(1.0), Constant(2.0)) → FoldConstants → Constant(0.5)
    auto df = dfactory::invert(dfactory::constant(2.0));
    expectConstant(toOptimized(*df), 0.5);
}

TEST(OptoPassesTest, FoldInvertZeroNotFolded)
{
    // invert(0.0) → Div(1.0, 0.0)：除零不折叠（保持 1.0/0.0=+inf 路径，避免常量 inf 干扰）
    auto df = dfactory::invert(dfactory::constant(0.0));
    const AstPtr opt = toOptimized(*df);
    ASSERT_NE(opt, nullptr);
    EXPECT_EQ(opt->kind(), AstNodeKind::Div) << "Div by zero must not fold to constant inf";
}

// ============================================================================
// FoldConstants：Lerp（delta<=0→start, delta>=1→end, 三常量直接求值）
// ============================================================================

TEST(OptoPassesTest, FoldLerpAllConstants)
{
    // lerp(0.5, 10, 20) = 10 + 0.5*(20-10) = 15
    auto df = dfactory::lerp(dfactory::constant(0.5), dfactory::constant(10.0), dfactory::constant(20.0));
    expectConstant(toOptimized(*df), 15.0);
}

TEST(OptoPassesTest, FoldLerpDeltaAtLeastOne)
{
    // lerp(2.0, A, B) → delta>=1 → B
    auto df = dfactory::lerp(dfactory::constant(2.0), dfactory::constant(10.0), dfactory::constant(20.0));
    expectConstant(toOptimized(*df), 20.0);
}

TEST(OptoPassesTest, FoldLerpDeltaAtMostZero)
{
    // lerp(-1.0, A, B) → delta<=0 → A
    auto df = dfactory::lerp(dfactory::constant(-1.0), dfactory::constant(10.0), dfactory::constant(20.0));
    expectConstant(toOptimized(*df), 10.0);
}

// ============================================================================
// FoldConstants：Clamp（input 常量直接 clamp）
// ============================================================================

TEST(OptoPassesTest, FoldClampInRange)
{
    // clamp(5, 0, 10) = 5
    auto df = dfactory::clamp(dfactory::constant(5.0), 0.0, 10.0);
    expectConstant(toOptimized(*df), 5.0);
}

TEST(OptoPassesTest, FoldClampAboveRange)
{
    // clamp(15, 0, 10) = 10
    auto df = dfactory::clamp(dfactory::constant(15.0), 0.0, 10.0);
    expectConstant(toOptimized(*df), 10.0);
}

TEST(OptoPassesTest, FoldClampBelowRange)
{
    // clamp(-5, 0, 10) = 0
    auto df = dfactory::clamp(dfactory::constant(-5.0), 0.0, 10.0);
    expectConstant(toOptimized(*df), 0.0);
}

// ============================================================================
// FoldConstants：MinShort/MaxShort 退化（常量操作数 → 普通 Min/Max → 折叠）
// ============================================================================

TEST(OptoPassesTest, FoldMinShortDegradesAndFolds)
{
    // min(Constant(2), Constant(7))：arg1.minValue()=2 < arg2.minValue()=7 → MinShortNode(2,7,7)
    // FoldConstants：left=Constant → 退化为 MinNode → 两常量折叠 → Constant(2)
    auto df = dfactory::min(dfactory::constant(2.0), dfactory::constant(7.0));
    expectConstant(toOptimized(*df), 2.0);
}

TEST(OptoPassesTest, FoldMaxShortDegradesAndFolds)
{
    // max(Constant(2), Constant(7))：arg1.maxValue()=2 > arg2.maxValue()=7? 否(2<7) → MaxNode
    // → 两常量折叠 → Constant(7)
    auto df = dfactory::max(dfactory::constant(2.0), dfactory::constant(7.0));
    expectConstant(toOptimized(*df), 7.0);
}

// ============================================================================
// TreeNormalization：可交换节点常量规范到左
// ============================================================================

TEST(OptoPassesTest, TreeNormalizationConstantToLeft)
{
    // add(X, 5.0)：X 非常量，5.0 常量在右 → 交换 → add(5.0, X)
    // FoldConstants 不消（5.0≠0）→ 保持 add(Constant(5.0), X)
    auto df = dfactory::add(nonConstant(), dfactory::constant(5.0));
    const AstPtr opt = toOptimized(*df);
    ASSERT_NE(opt, nullptr);
    EXPECT_EQ(opt->kind(), AstNodeKind::Add);
    EXPECT_EQ(binaryLeft(opt)->kind(), AstNodeKind::Constant) << "constant should be normalized to left";
    EXPECT_EQ(binaryRight(opt)->kind(), AstNodeKind::YClampedGradient);
    if (binaryLeft(opt)->kind() == AstNodeKind::Constant) {
        EXPECT_DOUBLE_EQ(static_cast<const ConstantNode*>(binaryLeft(opt).get())->value(), 5.0);
    }
}

TEST(OptoPassesTest, TreeNormalizationMinNotSwapped)
{
    // min(X, 5.0)：Min 不可交换 → MinShort 退化 MinNode(X, 5.0)，常量仍在右
    // X=YClampedGradient minValue=-1 < 5 → MinShortNode(X, Constant(5), 5) → 退化 MinNode(X, Constant(5))
    auto df = dfactory::min(nonConstant(), dfactory::constant(5.0));
    const AstPtr opt = toOptimized(*df);
    ASSERT_NE(opt, nullptr);
    EXPECT_EQ(opt->kind(), AstNodeKind::Min) << "MinShort with non-const left degrades to Min";
    EXPECT_EQ(binaryLeft(opt)->kind(), AstNodeKind::YClampedGradient);
    EXPECT_EQ(binaryRight(opt)->kind(), AstNodeKind::Constant) << "Min not swapped: constant stays right";
}

// ============================================================================
// BranchElimination：RangeChoice 输入常量选支 / 两支相等消分支
// ============================================================================

TEST(OptoPassesTest, BranchEliminationInRange)
{
    // rangeChoice(0.5, 0, 1, 10, 20) → 0.5∈[0,1) → whenInRange=10
    auto df =
        dfactory::rangeChoice(dfactory::constant(0.5), 0.0, 1.0, dfactory::constant(10.0), dfactory::constant(20.0));
    expectConstant(toOptimized(*df), 10.0);
}

TEST(OptoPassesTest, BranchEliminationOutOfRange)
{
    // rangeChoice(2.0, 0, 1, 10, 20) → 2.0∉[0,1) → whenOutOfRange=20
    auto df =
        dfactory::rangeChoice(dfactory::constant(2.0), 0.0, 1.0, dfactory::constant(10.0), dfactory::constant(20.0));
    expectConstant(toOptimized(*df), 20.0);
}

TEST(OptoPassesTest, BranchEliminationEqualBranches)
{
    // rangeChoice(X, 0, 1, A, A)：两支相等（同 Constant(7)）→ 消分支 → A=7
    auto df = dfactory::rangeChoice(nonConstant(), 0.0, 1.0, dfactory::constant(7.0), dfactory::constant(7.0));
    expectConstant(toOptimized(*df), 7.0);
}

// ============================================================================
// 不动点：多层嵌套常量折叠到收敛
// ============================================================================

TEST(OptoPassesTest, FixedPointNestedFold)
{
    // add(add(1, 2), add(3, 4)) → add(3, 7) → 10（需多轮 transform 收敛）
    auto df = dfactory::add(dfactory::add(dfactory::constant(1.0), dfactory::constant(2.0)),
        dfactory::add(dfactory::constant(3.0), dfactory::constant(4.0)));
    expectConstant(toOptimized(*df), 10.0);
}

TEST(OptoPassesTest, FixedPointMixedFoldAndBranch)
{
    // rangeChoice(add(1,1), 0, 1, add(2,2), add(3,3))
    // → RangeChoice input 折成 Constant(2) → BranchElimination 选 outOfRange=add(3,3) → 折 Constant(6)
    auto df = dfactory::rangeChoice(dfactory::add(dfactory::constant(1.0), dfactory::constant(1.0)),
        0.0,
        1.0,
        dfactory::add(dfactory::constant(2.0), dfactory::constant(2.0)),
        dfactory::add(dfactory::constant(3.0), dfactory::constant(3.0)));
    expectConstant(toOptimized(*df), 6.0);
}

// ============================================================================
// Persistence sharing：无可优化时根节点引用不变
// ============================================================================

TEST(OptoPassesTest, NoChangePreservesIdentity)
{
    // add(X, Y)：两非常量，无可优化 → optimize 返回同一引用（persistence sharing）
    auto df = dfactory::add(nonConstant(), nonConstant());
    const AstPtr ast = McToAst::convert(*df);
    const AstPtr opt = OptoPasses::optimize(ast);
    EXPECT_EQ(opt.get(), ast.get()) << "unchanged tree should preserve root identity";
}
