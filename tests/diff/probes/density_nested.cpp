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

#include "probe.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "server/world/gen/density/DensityFunctions.hpp"

namespace mc::diff {

namespace {

using json = nlohmann::json;

using world::gen::density::Clamp;
using world::gen::density::Constant;
using world::gen::density::DensityFunction;
using world::gen::density::Lerp;
using world::gen::density::Mapped;
using world::gen::density::MappedType;
using world::gen::density::RangeChoice;
using world::gen::density::TwoArgument;
using world::gen::density::TwoArgumentType;
using world::gen::density::YClampedGradient;

/**
 * @file density_nested.cpp
 * @brief 嵌套密度函数树的求值与区间（原版 DensityFunctions 组合）。
 *
 * `density_composite` 只测了「两个常量喂给一个二元算子」这一层；真实地形里密度函数
 * 从不这么用——主世界的 `final_density` 是一棵多层树：噪声项经 Mapped 整形、与梯度
 * 相加、被 Clamp 截断、再经 RangeChoice 在洞穴与地形之间切换。
 *
 * 组合型密度函数有三个易错点，单层测试抓不到，必须嵌套起来才暴露：
 *
 *   1. **区间传播**：`minValue/maxValue` 是自底向上合成的。`Mul` 的区间不是两个区间
 *      直接相乘（负负得正），`Lerp` 要看 delta 的符号，`RangeChoice` 要合并两个分支。
 *      区间只用于优化与调度，算错不会崩，但会让缓存与插值的判断出现偏差。
 *   2. **短路语义**：`Mul` 一边为 0 时另一侧即使是 NaN/inf 也应短路成 0；`Max`/`Min`
 *      对 NaN 的处理也依赖运算顺序。
 *   3. **梯度与 Clamp 的相对位置**：先 Clamp 再 Mapped 与先 Mapped 再 Clamp 结果不同，
 *      顺序写反会让地形整体变平或长出尖刺。
 *
 * 探针直接构造生产代码的密度函数类，不重写 compute——要测的正是这份实现。
 */

/// 采样点：覆盖主世界高度范围与若干越界/负坐标。
const std::vector<std::array<i32, 3>>& samplePoints()
{
    static const std::vector<std::array<i32, 3>> table = {
        {0, -64, 0},
        {0, 0, 0},
        {0, 64, 0},
        {0, 319, 0},
        {15, 128, 15},
        {-16, 0, -16},
        {-1875000, 100, 1875000},
        {1000000, -192, -1000000},
        {7, 63, -3},
        {-1, 62, 1},
    };
    return table;
}

/// 一棵命名树：只需要 id 与构造方式，具体类由 buildTree 决定。
const std::vector<const char*>& treeNames()
{
    static const std::vector<const char*> table = {
        // 单层基线：确认构造与新包装没有改变原值
        "constant",
        "gradient",
        // Mapped 套梯度：主世界 offset 这类「先把梯度整形」的形态
        "mapped_gradient_abs",
        "mapped_gradient_square",
        "mapped_gradient_squeeze",
        // 二元算子套两层梯度：add / mul / min / max
        "add_two_gradients",
        "mul_two_gradients",
        "min_two_gradients",
        "max_two_gradients",
        // Clamp 套在二元算子外面：先合成再截断
        "clamp_after_add",
        "clamp_after_mul",
        // Mapped 与 Clamp 顺序对调：结果必须不同，顺序写反会立刻显形
        "mapped_after_clamp",
        "clamp_after_mapped",
        // Lerp 三分支：delta 取值 <0 / 0..1 / >1，区间合并要跟着变
        "lerp_three_gradients",
        // RangeChoice：命中与不命中两个分支，边界取 [lo, hi)
        "range_choice_gradient",
        // Mul 一边恒为 0：短路必须吞掉另一侧的 NaN/inf
        "mul_by_zero",
        // 深层嵌套：Mapped(Mapped(Lerp(Clamp, Add, Mul)))
        "deep_mix",
    };
    return table;
}

/// 构造一棵树。返回 nullptr 表示名字不认识（两侧都必须给出同样的树）。
std::unique_ptr<DensityFunction> buildTree(std::string_view name)
{
    // 主世界常见梯度：Y=-64 处 1.0，Y=320 处 -1.0。
    auto overworldGradient = [] {
        return std::make_unique<YClampedGradient>(-64, 320, 1.0, -1.0);
    };
    // 反向梯度：从 0 到 1，用来让 add 的结果跨过零点。
    auto risingGradient = [] {
        return std::make_unique<YClampedGradient>(-64, 320, 0.0, 1.0);
    };

    if (name == "constant") {
        return std::make_unique<Constant>(0.25);
    }
    if (name == "gradient") {
        return overworldGradient();
    }
    if (name == "mapped_gradient_abs") {
        return std::make_unique<Mapped>(overworldGradient(), MappedType::Abs);
    }
    if (name == "mapped_gradient_square") {
        return std::make_unique<Mapped>(overworldGradient(), MappedType::Square);
    }
    if (name == "mapped_gradient_squeeze") {
        return std::make_unique<Mapped>(overworldGradient(), MappedType::Squeeze);
    }
    if (name == "add_two_gradients") {
        return std::make_unique<TwoArgument>(overworldGradient(), risingGradient(), TwoArgumentType::Add);
    }
    if (name == "mul_two_gradients") {
        return std::make_unique<TwoArgument>(overworldGradient(), risingGradient(), TwoArgumentType::Mul);
    }
    if (name == "min_two_gradients") {
        return std::make_unique<TwoArgument>(overworldGradient(), risingGradient(), TwoArgumentType::Min);
    }
    if (name == "max_two_gradients") {
        return std::make_unique<TwoArgument>(overworldGradient(), risingGradient(), TwoArgumentType::Max);
    }
    if (name == "clamp_after_add") {
        return std::make_unique<Clamp>(
            std::make_unique<TwoArgument>(overworldGradient(), risingGradient(), TwoArgumentType::Add), -0.5, 0.5);
    }
    if (name == "clamp_after_mul") {
        return std::make_unique<Clamp>(
            std::make_unique<TwoArgument>(overworldGradient(), risingGradient(), TwoArgumentType::Mul), -0.25, 0.75);
    }
    if (name == "mapped_after_clamp") {
        return std::make_unique<Mapped>(
            std::make_unique<Clamp>(overworldGradient(), -0.5, 0.5), MappedType::HalfNegative);
    }
    if (name == "clamp_after_mapped") {
        return std::make_unique<Clamp>(
            std::make_unique<Mapped>(overworldGradient(), MappedType::HalfNegative), -0.5, 0.5);
    }
    if (name == "lerp_three_gradients") {
        // delta 用反向梯度（1 → -1），穿越 0 与 1，让三分支都被走到。
        return std::make_unique<Lerp>(overworldGradient(), risingGradient(), std::make_unique<Constant>(-3.0));
    }
    if (name == "range_choice_gradient") {
        // 输入取反向梯度（1 → -1），区间 [0, 0.5) 内走 whenInRange。
        return std::make_unique<RangeChoice>(overworldGradient(),
            0.0,
            0.5,
            std::make_unique<Constant>(10.0),
            std::make_unique<Mapped>(risingGradient(), MappedType::Cube));
    }
    if (name == "mul_by_zero") {
        return std::make_unique<TwoArgument>(
            std::make_unique<Constant>(0.0), overworldGradient(), TwoArgumentType::Mul);
    }
    if (name == "deep_mix") {
        auto inner = std::make_unique<Lerp>(risingGradient(),
            std::make_unique<Clamp>(overworldGradient(), -0.5, 0.5),
            std::make_unique<TwoArgument>(overworldGradient(), risingGradient(), TwoArgumentType::Mul));
        auto middle = std::make_unique<Mapped>(std::move(inner), MappedType::Squeeze);
        return std::make_unique<Mapped>(std::move(middle), MappedType::Invert);
    }
    return nullptr;
}

void writeList(std::FILE* out)
{
    for (const char* tree : treeNames()) {
        for (size_t i = 0; i < samplePoints().size(); ++i) {
            std::fprintf(out, "%s@%zu\n", tree, i);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t at = requested.rfind('@');
    if (at == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string treeName = requested.substr(0, at);
    const size_t pointIndex = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (pointIndex >= samplePoints().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const auto tree = buildTree(treeName);
    if (tree == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const auto& point = samplePoints()[pointIndex];
    return json{{"unit", requested},
        {"value", encodeFloat(tree->compute(point[0], point[1], point[2]))},
        {"min", encodeFloat(tree->minValue())},
        {"max", encodeFloat(tree->maxValue())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(density_nested, &writeList, &runLine)

} // namespace mc::diff
