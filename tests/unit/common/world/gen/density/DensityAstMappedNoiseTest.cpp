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
 * @file DensityAstMappedNoiseTest.cpp
 * @brief MappedNoise 的 AST 编译路径与解释执行等价
 *
 * MappedNoise 是"噪声采样后按 fromValue/toValue 线性重映射"的密度函数：
 *   compute = midpoint + noise(x*xzScale, y*yScale, z*xzScale) * halfAmplitude
 * 其中 midpoint = (from+to)/2、halfAmplitude = (to-from)/2。
 *
 * 当前数据包的三个维度噪声树都没有用到 `mapped` 类型（解析器也未提供该 JSON 类型），
 * 因此端到端的维度级测试覆盖不到它。本用例自建 MappedNoise 实例，校验
 * McToAst → OptoPasses → BytecodeGen → eval 与 compute 逐点一致，避免这条路径
 * 悄悄退化成 DelegateNode（退化会走虚函数并失去常量折叠，且不报错）。
 */

#include "common/core/Types.hpp"
#include "server/world/gen/density/DensityFunctions.hpp"
#include "server/world/gen/density/ast/BytecodeGen.hpp"
#include "server/world/gen/density/ast/CompiledDensityFunction.hpp"
#include "server/world/gen/density/ast/McToAst.hpp"
#include "server/world/gen/density/ast/OptoPasses.hpp"
#include "server/world/gen/noise/NormalNoise.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::gen::density;
using namespace mc::world::gen::density::ast;

namespace {

/// 测试专用 NormalNoise：与数据包里的噪声同构（seed + firstOctave + 振幅表）。
std::shared_ptr<const world::gen::noise::NormalNoise> makeTestNoise(
    u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
{
    return std::make_shared<world::gen::noise::NormalNoise>(seed, firstOctave, std::move(amplitudes));
}

/// 编译一棵 AST 密度函数：McToAst → OptoPasses → BytecodeGen。
std::shared_ptr<CompiledDensityFunction> compile(const DensityFunction& function)
{
    AstNode::Ptr ast = McToAst::convert(function);
    AstNode::Ptr optimized = OptoPasses::optimize(std::move(ast));
    return BytecodeGen::compile(optimized, function.minValue(), function.maxValue());
}

/// 采样点：覆盖原点、负坐标、深层/高空与区块边界。
const std::vector<std::tuple<i32, i32, i32>>& samplePoints()
{
    static const std::vector<std::tuple<i32, i32, i32>> points = {
        {0, 0, 0},
        {8, 64, 8},
        {31, -32, 17},
        {-8, 64, -8},
        {-123, -60, 456},
        {3, 320, 3},
    };
    return points;
}

} // namespace

/**
 * @test 编译产物与 compute 数值一致
 *
 * 选一组非平凡参数（xzScale != yScale、fromValue != 0），确保重映射的
 * midpoint/halfAmplitude 与坐标缩放都真正参与计算——若退化成直通噪声，
 * 数值会对不上。
 */
TEST(DensityAstMappedNoiseTest, CompiledMatchesCompute)
{
    const auto noise = makeTestNoise(42ULL, -7, {1.0, 1.0, 1.0, 1.0});
    const MappedNoise mapped(noise, 1.0 / 80.0, 1.0 / 160.0, 0.2, 1.8);

    auto compiled = compile(mapped);
    ASSERT_NE(compiled, nullptr);

    for (const auto& [x, y, z] : samplePoints()) {
        const f64 expected = mapped.compute(x, y, z);
        const f64 actual = compiled->eval(x, y, z);
        EXPECT_NEAR(actual, expected, 1e-9) << "point (" << x << "," << y << "," << z << ")";
    }
}

/**
 * @test 不同重映射参数必须编译成不同结果
 *
 * 同一噪声、不同 from/to 的输出完全不同。若节点比较/哈希漏掉重映射标量，
 * 优化阶段的公共子表达式合并会把两者误判为同一个节点，导致其中一个算错。
 */
TEST(DensityAstMappedNoiseTest, DifferentRangesProduceDifferentValues)
{
    const auto noise = makeTestNoise(7ULL, -9, {1.0, 0.5});

    const MappedNoise narrow(noise, 1.0 / 32.0, 1.0 / 64.0, 0.0, 1.0);
    const MappedNoise wide(noise, 1.0 / 32.0, 1.0 / 64.0, -10.0, 10.0);

    auto narrowCompiled = compile(narrow);
    auto wideCompiled = compile(wide);
    ASSERT_NE(narrowCompiled, nullptr);
    ASSERT_NE(wideCompiled, nullptr);

    bool anyDifference = false;
    for (const auto& [x, y, z] : samplePoints()) {
        const f64 narrowValue = narrowCompiled->eval(x, y, z);
        const f64 wideValue = wideCompiled->eval(x, y, z);
        EXPECT_NEAR(narrowValue, narrow.compute(x, y, z), 1e-9);
        EXPECT_NEAR(wideValue, wide.compute(x, y, z), 1e-9);
        if (std::abs(narrowValue - wideValue) > 1e-6) {
            anyDifference = true;
        }
    }
    EXPECT_TRUE(anyDifference) << "different from/to ranges must not collapse to the same result";
}

/**
 * @test 重映射的边界值与 minValue/maxValue 自洽
 *
 * 编译期 bounds 由节点自行计算，必须包住实际采样值——否则依赖 bounds 的短路段
 * 优化（MaxShort/MinShort、常量折叠）会得出错误结果。
 */
TEST(DensityAstMappedNoiseTest, CompiledValuesStayWithinBounds)
{
    const auto noise = makeTestNoise(1234ULL, -6, {1.0, 1.0, 1.0});
    const MappedNoise mapped(noise, 1.0 / 50.0, 1.0 / 100.0, -0.5, 0.5);

    auto compiled = compile(mapped);
    ASSERT_NE(compiled, nullptr);

    for (const auto& [x, y, z] : samplePoints()) {
        const f64 value = compiled->eval(x, y, z);
        EXPECT_GE(value, mapped.minValue() - 1e-9) << "point (" << x << "," << y << "," << z << ")";
        EXPECT_LE(value, mapped.maxValue() + 1e-9) << "point (" << x << "," << y << "," << z << ")";
    }
}
