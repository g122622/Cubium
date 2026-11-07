#include "probe.hpp"

#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "server/world/gen/density/DensityFunctions.hpp"
#include "server/world/gen/noise/NormalNoise.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using world::gen::density::DensityFunction;
using world::gen::density::ShiftNoise;
using world::gen::density::ShiftType;
using world::gen::noise::NormalNoise;

/**
 * @brief 顶点位移噪声的用例
 *
 * 覆盖 `ShiftA` / `ShiftB` / `Shift` 三种形态。它们共享同一套算术——把三个坐标各乘 0.25
 * 后喂给噪声、结果再乘 4——区别只在坐标如何映射到噪声的三个轴：
 *
 * - `ShiftA`：`(x, 0, z)`
 * - `ShiftB`：`(z, x, 0)`
 * - `Shift` ：`(x, y, z)`
 *
 * 这三种是 `ShiftedNoise` 位移函数的实际提供方，两者组合起来才是完整的地形扭曲链路。
 * 幅度用 4.0 的整数倍，因为原版就是为了把噪声抬回方块量级。
 *
 * 单元格式：`<类型>|<配置下标>@<采样点下标>`
 */
struct ShiftCase {
    const char* id;
    ShiftType type;
    i64 seed;
    i32 firstOctave;
    std::vector<f64> amplitudes;
};

const std::vector<ShiftCase>& shiftCases()
{
    static const std::vector<ShiftCase> table = {
        // 主世界 SHIFT 用的那种参数形状：多八度、中等振幅数
        {"a_main", ShiftType::ShiftA, 1234567, -8, {1.0, 1.0, 1.0, 1.0}},
        {"b_main", ShiftType::ShiftB, 1234567, -8, {1.0, 1.0, 1.0, 1.0}},
        {"c_main", ShiftType::Shift, 1234567, -8, {1.0, 1.0, 1.0, 1.0}},

        // 单八度：退化成最简形态，便于把差异定位到坐标映射本身
        {"a_single", ShiftType::ShiftA, 42, -4, {1.0}},
        {"b_single", ShiftType::ShiftB, 42, -4, {1.0}},
        {"c_single", ShiftType::Shift, 42, -4, {1.0}},

        // 递减振幅与含零振幅：覆盖层数与跳过逻辑
        {"a_mixed", ShiftType::ShiftA, 987654321, -6, {1.0, 0.5, 0.0, 0.25}},
        {"c_mixed", ShiftType::Shift, 987654321, -6, {1.0, 0.5, 0.0, 0.25}},

        // 不同首个倍频：覆盖 inputFactor 的 2 的幂序列
        {"a_oct", ShiftType::ShiftA, -42, -7, {1.0, 1.0, 1.0}},
    };
    return table;
}

/// 采样坐标：三轴互不相等，才能区分 ShiftA 与 ShiftB 的轴置换。
const std::vector<std::array<i32, 3>>& samplePoints()
{
    static const std::vector<std::array<i32, 3>> table = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {1, 2, 3},
        {3, 2, 1},
        {-1, -1, -1},
        {16, 64, -16},
        {100, -32, 200},
        {-1000, 5, 1000},
        {12345, 200, -6789},
    };
    return table;
}

std::unique_ptr<DensityFunction> buildFunction(const ShiftCase& shiftCase)
{
    math::JavaLegacyRandom rng(shiftCase.seed);
    auto noise = std::make_shared<NormalNoise>(rng, shiftCase.firstOctave, shiftCase.amplitudes);
    return std::make_unique<ShiftNoise>(noise, shiftCase.type);
}

void writeList(std::FILE* out)
{
    for (const auto& shiftCase : shiftCases()) {
        for (size_t i = 0; i < samplePoints().size(); ++i) {
            std::fprintf(out, "%s@%zu\n", shiftCase.id, i);
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
    const std::string caseId = requested.substr(0, at);
    const size_t index = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (index >= samplePoints().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    for (const auto& shiftCase : shiftCases()) {
        if (caseId != shiftCase.id) {
            continue;
        }
        // 直接构造生产代码的 ShiftNoise，不在这里重写坐标映射与缩放。
        const auto function = buildFunction(shiftCase);
        const auto& point = samplePoints()[index];
        const f64 value = function->compute(point[0], point[1], point[2]);
        return json{{"unit", requested},
            {"value", encodeFloat(value)},
            {"min", encodeFloat(function->minValue())},
            {"max", encodeFloat(function->maxValue())}}
            .dump();
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(shift_noise, &writeList, &runLine)

} // namespace mc::diff
