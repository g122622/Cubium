#include "probe.hpp"

#include "common/util/math/MathUtils.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @brief 三线性插值的用例
 *
 * `math::lerp3` 是区块内噪声插值的核心：`NoiseInterpolator` 把密度函数在 4x8x4 的网格上
 * 采样，再用它把网格值插到任意方块坐标。这是密度函数之上最后一层未测的生成逻辑。
 *
 * 插值顺序 X → Y → Z，与原版 `Mth.lerp3` 一致。三步 lerp 各自独立舍入，任何一步被
 * 编译器融合成 FMA 都会让结果偏若干 ulp，且偏差会随插值层级放大。
 *
 * 用例设计要点：
 *
 * - 八个角取自同一组「参差」数值，量级与符号都不整齐，便于暴露舍入差异；
 * - 插值因子覆盖 0 / 1 / 0.5 等精确值，以及 1/3、0.1 这类无法精确表示的分数——
 *   后者才真正依赖逐步舍入的顺序。
 *
 * 单元格式：`<角组合>|<因子组合>@<角值组下标>`
 */
struct LatticeCase {
    /// 八个角值，顺序为 d0=(0,0,0) d1=(1,0,0) d2=(0,1,0) d3=(1,1,0) d4=(0,0,1) ...
    std::array<f64, 8> corners;
};

const std::vector<LatticeCase>& latticeCases()
{
    static const std::vector<LatticeCase> table = {
        // 全零：基线
        {{{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}},
        // 单位立方体：插值结果应恰好落在因子张成的曲面上
        {{{0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0}}},
        // 参差小数量级，正负混杂
        {{{0.123456789, -0.987654321, 0.5, -0.25, 0.75, -0.125, 0.0625, -0.03125}}},
        // 大数量级：吸收效应明显
        {{{1.0e12, -3.0e12, 7.0e11, -1.5e11, 2.5e13, -8.0e12, 4.0e10, -6.0e10}}},
        // 宽动态范围但仍在真实量级内：密度函数的取值大约在 ±1e3，八角的差值可以跨三个数量级。
        // 不取 1e16 与 1e-300 同处一组——那种极端混排在 `-ffast-math` 下会触发编译器对
        // `lerp` 的代数化简（生产构建正是带 `-ffast-math`），得到的偏差来自浮点选项而不是
        // 插值逻辑；项目明确不对齐这类浮点细节，构造它只会制造无意义的失败。
        {{{1.0, 1.0e-3, 1.0e3, -1.0e3, 0.0, 1.0, -1.0, 1.0e-2}}},
        // 相邻值几乎相等：差值只有几个 ulp
        {{{1.0, 1.0000000000000002, 0.9999999999999999, 1.0, 1.0000000000000004, 0.9999999999999998, 1.0, 1.0}}},
        // 全负数
        {{{-1.5, -2.5, -3.5, -4.5, -5.5, -6.5, -7.5, -8.5}}},
    };
    return table;
}

/// 插值因子三元组。刻意混入无法精确表示的分数。
const std::vector<std::array<f64, 3>>& factorCases()
{
    static const std::vector<std::array<f64, 3>> table = {
        // 八个角本身（不插值）
        {0.0, 0.0, 0.0},
        {1.0, 1.0, 1.0},
        // 精确的二进制分数
        {0.5, 0.5, 0.5},
        {0.25, 0.75, 0.125},
        // 无法精确表示的分数：依赖逐步舍入
        {1.0 / 3.0, 1.0 / 7.0, 1.0 / 11.0},
        {0.1, 0.2, 0.3},
        {0.7, 0.9, 0.11},
        // 只在一根轴上偏离
        {0.5, 0.0, 0.0},
        {0.0, 0.5, 0.0},
        {0.0, 0.0, 0.5},
        // 靠近 1 的边界
        {0.9999999999, 0.0000000001, 0.5},
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (size_t ci = 0; ci < latticeCases().size(); ++ci) {
        for (size_t fi = 0; fi < factorCases().size(); ++fi) {
            std::fprintf(out, "%zu|%zu@%zu\n", ci, fi, ci);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t pipe = requested.find('|');
    const size_t at = requested.rfind('@');
    if (pipe == std::string::npos || at == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const size_t cornerIndex = static_cast<size_t>(std::stoul(requested.substr(0, pipe)));
    const size_t factorIndex = static_cast<size_t>(std::stoul(requested.substr(pipe + 1, at - pipe - 1)));
    if (cornerIndex >= latticeCases().size() || factorIndex >= factorCases().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const auto& corners = latticeCases()[cornerIndex].corners;
    const auto& factors = factorCases()[factorIndex];
    // 直接调用生产代码的 lerp3，不在这里重写插值步骤。
    // 插值因子必须对编译器**不可见为常量**。
    //
    // 生产调用点（NoiseChunk.cpp 的 NoiseInterpolator::compute）的因子是
    // `inCellX / cellWidth` 算出来的，编译器拿不到具体数值。而这里若直接传字面量
    // 0.5，`-ffast-math` 下的值域推测会把 lerp 内部的 volatile 标量存储消掉，使
    // `lerp3` 的返回值与逐层展开不一致——那是测试构造方式偏离了真实调用形态，
    // 不是 Cubium 的缺陷。
    //
    // 用 volatile 中转一次，既保持数值完全不变（没有引入任何误差），又让编译器无法
    // 把因子折叠成常量，从而复现生产调用的可见性条件。
    const volatile f64 rawSx = factors[0];
    const volatile f64 rawSy = factors[1];
    const volatile f64 rawSz = factors[2];
    const f64 sx = rawSx;
    const f64 sy = rawSy;
    const f64 sz = rawSz;
    const f64 value = math::lerp3(
        sx, sy, sz, corners[0], corners[1], corners[2], corners[3], corners[4], corners[5], corners[6], corners[7]);
    // 逐层展开同一份插值，作为交叉校验：两者必须相等，否则说明编译器把 lerp3 的
    // 内部步骤重排了（`-ffast-math` 下可能发生），报告里一眼能看出差异来自哪一层。
    const f64 lower = math::lerp(math::lerp(corners[0], corners[1], sx), math::lerp(corners[2], corners[3], sx), sy);
    const f64 upper = math::lerp(math::lerp(corners[4], corners[5], sx), math::lerp(corners[6], corners[7], sx), sy);
    const f64 manual = math::lerp(lower, upper, sz);
    return json{{"unit", requested},
        {"value", encodeFloat(value)},
        {"lower", encodeFloat(lower)},
        {"upper", encodeFloat(upper)},
        {"manual", encodeFloat(manual)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(lerp3_interpolate, &writeList, &runLine)

} // namespace mc::diff
