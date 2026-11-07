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
using world::gen::density::Constant;
using world::gen::density::DensityFunction;
using world::gen::density::WeirdScaledSampler;
using world::gen::density::WeirdScaledSamplerType;
using world::gen::noise::NormalNoise;

/**
 * @brief 奇异缩放采样器的用例
 *
 * 覆盖 `WeirdScaledSampler` 的两种 rarity 映射。它不是纯算术——先拿输入值查表选出一个
 * 「稀有度」系数，再用 `坐标 / 系数` 去采样噪声、结果取绝对值后乘回系数。主世界的
 * weirdness / ridges 用它把噪声在空间上分层。
 *
 * 单元格式：`<类型>|<输入值>|<配置下标>@<采样点下标>`
 *
 * 输入值刻意取在各阈值上与其两侧，因为查表用的是严格小于（`value < 阈值`），
 * 边界上取等还是取不等决定了落在哪个档：
 *
 * - Type1 阈值：-0.5, 0.0, 0.5
 * - Type2 阈值：-0.75, -0.5, 0.5, 0.75
 */
struct NoiseCase {
    i64 seed;
    i32 firstOctave;
    std::vector<f64> amplitudes;
};

/// 噪声配置：主世界那种多八度形状，外加一个单八度退化形态。
const std::vector<NoiseCase>& noiseCases()
{
    static const std::vector<NoiseCase> table = {
        {1234567, -8, {1.0, 1.0, 1.0, 1.0}},
        {42, -4, {1.0}},
    };
    return table;
}

/// Type1 / Type2 的全部阈值与两侧、以及若干普通值。
const std::vector<f64>& inputValues()
{
    static const std::vector<f64> table = {
        // Type2 最低档以下
        -1.5,
        -0.7500001,
        -0.75,
        -0.7499999,
        -0.6,
        // 两类型共用区
        -0.5000001,
        -0.5,
        -0.4999999,
        -0.25,
        // 零点附近：Type1 在此分档
        -0.0000001,
        0.0,
        0.0000001,
        0.25,
        // Type1 与 Type2 共同的高档边界
        0.4999999,
        0.5,
        0.5000001,
        0.6,
        // Type2 最高档
        0.7499999,
        0.75,
        0.7500001,
        1.5,
    };
    return table;
}

/// 采样坐标：三轴互不相等，且含零与大坐标（除以系数后量级变化明显）。
const std::vector<std::array<i32, 3>>& samplePoints()
{
    static const std::vector<std::array<i32, 3>> table = {
        {0, 0, 0},
        {1, 2, 3},
        {-3, 2, -1},
        {16, 64, -16},
        {-1000, 5, 1000},
    };
    return table;
}

std::unique_ptr<DensityFunction> buildFunction(WeirdScaledSamplerType type, f64 inputValue, const NoiseCase& noiseCase)
{
    math::JavaLegacyRandom rng(noiseCase.seed);
    auto noise = std::make_shared<NormalNoise>(rng, noiseCase.firstOctave, noiseCase.amplitudes);
    return std::make_unique<WeirdScaledSampler>(std::make_unique<Constant>(inputValue), std::move(noise), type);
}

void writeList(std::FILE* out)
{
    for (int typeIndex = 0; typeIndex < 2; ++typeIndex) {
        for (const f64 inputValue : inputValues()) {
            for (size_t configIndex = 0; configIndex < noiseCases().size(); ++configIndex) {
                for (size_t i = 0; i < samplePoints().size(); ++i) {
                    std::fprintf(
                        out, "%s|%.7f|%zu@%zu\n", typeIndex == 0 ? "type1" : "type2", inputValue, configIndex, i);
                }
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t at = requested.rfind('@');
    const size_t pipe1 = requested.find('|');
    const size_t pipe2 = requested.find('|', pipe1 + 1);
    if (at == std::string::npos || pipe1 == std::string::npos || pipe2 == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string typeName = requested.substr(0, pipe1);
    const f64 inputValue = std::stod(requested.substr(pipe1 + 1, pipe2 - pipe1 - 1));
    const size_t configIndex = static_cast<size_t>(std::stoul(requested.substr(pipe2 + 1, at - pipe2 - 1)));
    const size_t pointIndex = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (configIndex >= noiseCases().size() || pointIndex >= samplePoints().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    WeirdScaledSamplerType type;
    if (typeName == "type1") {
        type = WeirdScaledSamplerType::Type1;
    } else if (typeName == "type2") {
        type = WeirdScaledSamplerType::Type2;
    } else {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    // 直接构造生产代码的 WeirdScaledSampler，不在这里重写 rarity 查表。
    const auto function = buildFunction(type, inputValue, noiseCases()[configIndex]);
    const auto& point = samplePoints()[pointIndex];
    const f64 value = function->compute(point[0], point[1], point[2]);
    return json{{"unit", requested},
        {"value", encodeFloat(value)},
        {"min", encodeFloat(function->minValue())},
        {"max", encodeFloat(function->maxValue())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(weird_scaled_sampler, &writeList, &runLine)

} // namespace mc::diff
