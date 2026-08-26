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
 * @file DensityAstCompileTest.cpp
 * @brief 维度级 AST 编译器端到端数值等价测试（阶段4）
 *
 * 验证 DFC 风格 AST 编译器对维度级 finalDensity 树的求值正确性：
 *   McToAst::convert(finalDensity) → OptoPasses::optimize → BytecodeGen::compile
 *   → CompiledDensityFunction::eval(x,y,z)
 * 必须与原版 finalDensity().compute(x,y,z) 逐点数值一致（EXPECT_NEAR 1e-9）。
 *
 * 这是阶段4 的核心门禁：证明扁平指令序列求值器（含 Marker 占位透传、SharedSubtree 子求值器、
 * Delegate 退化、样条 Hermite、FindTopSurface 循环、RangeChoice 分支）对真实维度级密度函数树
 * 求值数值正确。阶段5 区块级接入（Marker 原地替换 + Beardifier 叠加）在此正确性基础上进行。
 *
 * 采样维度级树（未经 NoiseChunk 包装）：维度级 m_router->finalDensity() 树
 * 含原始 Marker（Marker::compute 无条件透传 m_wrapped，维度级无 FlatCache/Cache2D 等具体缓存
 * 实例），故 finalDensity().compute 在非插值上下文 == 透传 delegate 子树 compute。编译产物
 * MARKER 占位同样透传 delegate，两者数值一致（详见阶段4 设计核查）。
 *
 * 注：维度级树不含 NoiseChunk 构造期叠加的 BeardifierMarker（那是区块级 add(finalDensity,
 * BeardifierMarker)），故本测试的期望值来自维度级树自身，与 DensityAstBaselineTest（走
 * NoiseChunk sampleFinalDensity）的基线数值不同——两者校验不同层级的正确性，互不替代。
 *
 * 依赖 tests/main.cpp 的 WorldGenRegistryEnvironment 预加载 NoiseSettingsRegistry 等数据包。
 */

#include "common/core/Types.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/density/ast/BytecodeGen.hpp"
#include "common/world/gen/density/ast/CompiledDensityFunction.hpp"
#include "common/world/gen/density/ast/McToAst.hpp"
#include "common/world/gen/density/ast/OptoPasses.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

using namespace mc;
using namespace mc::world::gen::density;
using namespace mc::world::gen::density::ast;

namespace {

/// 采样点：覆盖区块内多相对位置、负坐标、多 Y 层（含地下/海平面/高空/深层/顶层）。
const std::vector<std::tuple<i32, i32, i32>>& samplePoints()
{
    static const std::vector<std::tuple<i32, i32, i32>> points = {
        {0, 0, 0},     // 原点
        {8, 64, 8},    // 主世界海平面附近
        {16, 0, 16},   // 区块边界
        {31, 31, 31},  // 区块内偏角
        {-8, 64, -8},  // 负坐标
        {64, -32, 64}, // 深层
        {3, 100, 3},   // 高空
        {12, 320, 12}, // 主世界顶
        {4, -60, 4},   // 主世界底
    };
    return points;
}

/// 把维度级 finalDensity 树编译成求值器：McToAst → OptoPasses → BytecodeGen。
std::shared_ptr<CompiledDensityFunction> compileFinalDensity(const DensityFunction& finalDensity)
{
    AstNode::Ptr ast = McToAst::convert(finalDensity);
    AstNode::Ptr optimized = OptoPasses::optimize(std::move(ast));
    return BytecodeGen::compile(optimized, finalDensity.minValue(), finalDensity.maxValue());
}

/// 校验单维度多 seed：编译产物 eval 与原版 finalDensity().compute 逐点数值一致。
void expectCompiledMatchesOriginal(
    const DimensionSettings& settings, const std::vector<u64>& seeds, const std::string& dimensionLabel)
{
    const auto& points = samplePoints();
    for (const u64 seed : seeds) {
        auto state = world::gen::RandomState::create(settings, seed);
        ASSERT_NE(state, nullptr) << dimensionLabel << " seed=" << seed << ": RandomState::create returned null";

        const DensityFunction& finalDensity = state->router().finalDensity();
        auto compiled = compileFinalDensity(finalDensity);
        ASSERT_NE(compiled, nullptr) << dimensionLabel << " seed=" << seed << ": compilation produced null";

        for (size_t i = 0; i < points.size(); ++i) {
            const auto [x, y, z] = points[i];
            const f64 original = finalDensity.compute(x, y, z);
            const f64 compiledValue = compiled->eval(x, y, z);
            EXPECT_NEAR(compiledValue, original, 1e-9)
                << dimensionLabel << " seed=" << seed << " point#" << i << " (" << x << "," << y << "," << z
                << ") compiled=" << compiledValue << " original=" << original;
        }
    }
}

} // namespace

// ============================================================================
// 维度级 AST 编译器端到端数值等价：三维度多 seed 的 finalDensity 多点采样。
// compiled.eval(x,y,z) 必须与 finalDensity().compute(x,y,z) 数值一致（1e-9）。
// ============================================================================

TEST(DensityAstCompileTest, OverworldCompiledMatchesOriginal)
{
    expectCompiledMatchesOriginal(DimensionSettings::overworld(), {0ULL, 1ULL, 42ULL, 12345ULL}, "overworld");
}

TEST(DensityAstCompileTest, NetherCompiledMatchesOriginal)
{
    expectCompiledMatchesOriginal(DimensionSettings::nether(), {0ULL, 7ULL, 99ULL}, "nether");
}

TEST(DensityAstCompileTest, EndCompiledMatchesOriginal)
{
    expectCompiledMatchesOriginal(DimensionSettings::end(), {0ULL, 5ULL, 256ULL}, "end");
}
