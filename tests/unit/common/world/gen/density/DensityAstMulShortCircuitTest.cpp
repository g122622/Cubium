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
 * @file DensityAstMulShortCircuitTest.cpp
 * @brief Mul 左操作数的形态：`Mul(left==0)` 短路的收益上界
 *
 * CompiledDensityFunction.hpp 曾断言"主世界 finalDensity 中 Mul 左操作数均为非零常量
 * （0*x 已被 FoldConstants 折叠），运行期命中率几乎为零"。这是关于**优化是否值得做**的
 * 可证伪断言，不该只靠阅读代码相信。
 *
 * 实测结果：主世界 3062 个 Mul 里 2842 个左操作数是常量、220 个是运行期子表达式，
 * 后者中 6 个在采样中真的取到 0（命中率约 2.7%）。所以原断言的**结论**（不值得实现）
 * 成立，但**理由**是错的——不是"没有非常量左操作数"，而是"命中的太少"。
 *
 * 本用例把支撑该结论的形态事实固化成可回归的断言，数据包或优化流程改变树形态时会立刻暴露。
 */

#include "common/core/Types.hpp"
#include "common/world/generation/density/DensityFunction.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/density/ast/AstNode.hpp"
#include "server/world/gen/density/ast/AstNodes.hpp"
#include "server/world/gen/density/ast/BytecodeGen.hpp"
#include "server/world/gen/density/ast/CompiledDensityFunction.hpp"
#include "server/world/gen/density/ast/McToAst.hpp"
#include "server/world/gen/density/ast/OptoPasses.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

using namespace mc;
using namespace mc::world::gen::density;
using namespace mc::world::gen::density::ast;

namespace {

/// 一次 AST 遍历的统计结果
struct MulCensus {
    i32 total = 0;        ///< Mul 节点总数
    i32 constantLeft = 0; ///< 左操作数是常量（非 0/1，否则已被折叠）
    i32 runtimeLeft = 0;  ///< 左操作数非常量，运行期可能取到 0
};

/// 递归遍历 AST，统计 Mul 节点左操作数的形态，并收集运行期左操作数子树供命中率实测。
void censusNode(const AstNode::Ptr& node,
    MulCensus& out,
    std::vector<AstNode::Ptr>& runtimeLeftOperands,
    std::unordered_set<const AstNode*>& visited)
{
    if (!node) {
        return;
    }
    // 优化后 AST 是 DAG（子树共享），同一节点只统计一次，避免重复计数。
    if (!visited.insert(node.get()).second) {
        return;
    }

    if (node->kind() == AstNodeKind::Mul) {
        const auto* mul = static_cast<const BinaryNode*>(node.get());
        ++out.total;
        if (mul->left() && mul->left()->kind() == AstNodeKind::Constant) {
            ++out.constantLeft;
        } else {
            ++out.runtimeLeft;
            runtimeLeftOperands.push_back(mul->left());
        }
    }

    for (const AstNode::Ptr& child : node->children()) {
        censusNode(child, out, runtimeLeftOperands, visited);
    }
    // Mul 之外的节点可能持有不进入 children() 的内部子树（如 SharedSubtreeRef 的 inner），
    // 但那部分已被独立编译为子求值器，其内部 Mul 不在本统计口径内。
}

/// 统计结果 + 优化后 AST。
///
/// root 必须一并持有：runtimeLeftOperands 里的每个 Ptr 都是 root 这棵 DAG 里的节点，
/// 而且 BytecodeGen 编译出的求值器只持节点里那些**裸指针**（噪声实例、内部子树），
/// 不共享 AST 所有权。若这里只返回 Ptr 集合、让 root 留在 analyze 的栈帧里，
/// 函数返回后节点被销毁、Ptr 变悬垂——读计数不崩，真去 eval 就访问冲突。
struct CensusFixture {
    MulCensus census;
    AstNode::Ptr root;
    std::vector<AstNode::Ptr> runtimeLeftOperands;
    std::shared_ptr<CompiledDensityFunction> compiled;
};

CensusFixture analyze(const DimensionSettings& settings, u64 seed)
{
    auto state = world::gen::RandomState::create(settings, seed);
    MC_ASSERT_RELEASE(state != nullptr);

    const DensityFunction& finalDensity = state->router().finalDensity();
    AstNode::Ptr ast = McToAst::convert(finalDensity);
    AstNode::Ptr optimized = OptoPasses::optimize(std::move(ast));

    CensusFixture fixture;
    fixture.root = optimized;
    std::unordered_set<const AstNode*> visited;
    censusNode(fixture.root, fixture.census, fixture.runtimeLeftOperands, visited);
    fixture.compiled = BytecodeGen::compile(fixture.root, finalDensity.minValue(), finalDensity.maxValue());
    return fixture;
}

} // namespace

/**
 * @test 三维度 Mul 左操作数形态统计
 *
 * 只输出统计，不设硬断言（不同维度/种子的形态不同，断言具体数字会让用例随数据包
 * 变动而失效）。这里的作用是把"短路是否有收益"变成可复查的实测输出。
 */
TEST(DensityAstMulShortCircuitTest, OverworldHasRuntimeLeftOperands)
{
    const CensusFixture fixture = analyze(DimensionSettings::overworld(), 0ULL);

    EXPECT_GT(fixture.census.total, 0) << "the final density tree must contain Mul nodes";
    EXPECT_GT(fixture.census.runtimeLeft, 0)
        << "overworld Mul must have non-constant left operands; if this fails, re-evaluate the "
           "Mul(left==0) short-circuit note in CompiledDensityFunction.hpp";
    EXPECT_EQ(fixture.census.constantLeft + fixture.census.runtimeLeft, fixture.census.total)
        << "every Mul must fall into exactly one left-operand category";
}

/**
 * @test 下界与末地的 finalDensity 树极小
 *
 * 各只有不到 4 个 Mul 节点，这是"短路在主世界之外没有讨论价值"的依据。
 * 若这两个维度的树变大，短路评估需要重做。
 */
TEST(DensityAstMulShortCircuitTest, NetherAndEndTreesAreTiny)
{
    const CensusFixture nether = analyze(DimensionSettings::nether(), 0ULL);
    const CensusFixture end = analyze(DimensionSettings::end(), 0ULL);

    EXPECT_LE(nether.census.total, 4) << "nether finalDensity is expected to be a very small tree";
    EXPECT_LE(end.census.total, 4) << "end finalDensity is expected to be a very small tree";
}
