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

/**
 * @file DensityJitBaselineTest.cpp
 * @brief 密度求值器 JIT 机器码 vs switch 解释器数值一致性测试
 *
 * 验证 asmjit JIT 编译产物（CompiledDensityFunction::eval 走 m_jitFn 机器码）与原 switch
 * 解释器（evalInterpreter → evalImpl）对真实维度级 finalDensity 树求值逐点数值一致（1e-9）。
 *
 * 测试链路：McToAst::convert → OptoPasses::optimize → BytecodeGen::compile（末尾调 compileJit）
 * → 同一求值器分别用 eval（JIT）与 evalInterpreter（解释器）求值，逐点对比。
 *
 * 关键断言：
 *   1. hasJitCompiled() == true：JIT 编译成功，未回退解释器（Win x64 平台）。若失败说明
 *      asmjit 编译出错（记 spdlog::warn），需排查 OpTranslator 翻译。
 *   2. eval(x,y,z) ≈ evalInterpreter(x,y,z)（1e-9）：JIT 机器码数值与解释器逐位一致。
 *      逐条 Op 顺序翻译 + asmjit 不重排保证浮点累加顺序不变，预期 bit-exact。
 *
 * 平台门控：仅 Windows x64 启用 JIT（MC_DENSITY_JIT_ENABLED 等价条件）。非 Win x64 平台
 * （macOS ARM64 等）JIT 返回 nullptr，本测试 GTEST_SKIP（ARM64 JIT 留 TODO）。
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

/// JIT 启用平台判定（与 DensityJitCompiler.cpp 的 MC_DENSITY_JIT_ENABLED 一致）。
#if (defined(_WIN32) || defined(__linux__)) && (defined(__x86_64__) || defined(_M_X64))
inline constexpr bool kJitEnabled = true;
#else
inline constexpr bool kJitEnabled = false;
#endif

/// 采样点：与 DensityAstCompileTest 一致，覆盖区块内多相对位置、负坐标、多 Y 层。
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

/// 把维度级 finalDensity 树编译成求值器（BytecodeGen::compile 末尾已调 compileJit）。
std::shared_ptr<CompiledDensityFunction> compileFinalDensity(const DensityFunction& finalDensity)
{
    AstNode::Ptr ast = McToAst::convert(finalDensity);
    AstNode::Ptr optimized = OptoPasses::optimize(std::move(ast));
    return BytecodeGen::compile(optimized, finalDensity.minValue(), finalDensity.maxValue());
}

/// 校验单维度多 seed：JIT 求值与解释器求值逐点数值一致（1e-9），且 JIT 编译成功。
void expectJitMatchesInterpreter(
    const DimensionSettings& settings, const std::vector<u64>& seeds, const std::string& dimensionLabel)
{
    if (!kJitEnabled) {
        GTEST_SKIP() << "density JIT not enabled on this platform (non x64); skipping JIT baseline";
    }
    const auto& points = samplePoints();
    for (const u64 seed : seeds) {
        auto state = world::gen::RandomState::create(settings, seed);
        ASSERT_NE(state, nullptr) << dimensionLabel << " seed=" << seed << ": RandomState::create returned null";

        const DensityFunction& finalDensity = state->router().finalDensity();
        auto compiled = compileFinalDensity(finalDensity);
        ASSERT_NE(compiled, nullptr) << dimensionLabel << " seed=" << seed << ": compilation produced null";

        // 断言 JIT 编译成功（未回退解释器）。失败说明 asmjit 编译出错，需排查 OpTranslator。
        ASSERT_TRUE(compiled->hasJitCompiled())
            << dimensionLabel << " seed=" << seed << ": JIT compilation failed (fell back to interpreter)";

        for (size_t i = 0; i < points.size(); ++i) {
            const auto [x, y, z] = points[i];
            const f64 jitValue = compiled->eval(x, y, z);               // 走 JIT 机器码
            const f64 interpValue = compiled->evalInterpreter(x, y, z); // 走 switch 解释器
            EXPECT_NEAR(jitValue, interpValue, 1e-9)
                << dimensionLabel << " seed=" << seed << " point#" << i << " (" << x << "," << y << "," << z
                << ") jit=" << jitValue << " interpreter=" << interpValue;
        }
    }
}

} // namespace

// ============================================================================
// JIT 机器码 vs switch 解释器数值一致性：三维度多 seed 的 finalDensity 多点采样。
// compiled.eval（JIT）必须与 compiled.evalInterpreter（解释器）数值一致（1e-9）。
// 同时断言 JIT 编译成功（hasJitCompiled），证明未意外回退。
// ============================================================================

TEST(DensityJitBaselineTest, OverworldJitMatchesInterpreter)
{
    expectJitMatchesInterpreter(DimensionSettings::overworld(), {0ULL, 1ULL, 42ULL, 12345ULL}, "overworld");
}

TEST(DensityJitBaselineTest, NetherJitMatchesInterpreter)
{
    expectJitMatchesInterpreter(DimensionSettings::nether(), {0ULL, 7ULL, 99ULL}, "nether");
}

TEST(DensityJitBaselineTest, EndJitMatchesInterpreter)
{
    expectJitMatchesInterpreter(DimensionSettings::end(), {0ULL, 5ULL, 256ULL}, "end");
}
