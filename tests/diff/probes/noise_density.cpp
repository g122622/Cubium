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
using world::gen::density::MappedNoise;
using world::gen::density::NoiseDensity;
using world::gen::noise::NormalNoise;

/**
 * @brief 噪声型密度函数的用例
 *
 * 覆盖 `NoiseDensity`（`noise * scale`）与 `MappedNoise`（把噪声重映射到 [from, to]）。
 * 两者都直接决定主世界与下界的洞穴形状，是最常见的两种噪声密度写法。
 *
 * 单元格式：`<类型>|<配置下标>@<采样点下标>`
 *
 * 配置里同时给出种子、八度参数与缩放，两侧按同一份参数各自构造 `NormalNoise`。
 *
 * 缩放特意用非整数、非 2 的幂（如 0.25 是精确的，0.1、1.0181268882175227 不是），
 * 因为 `x * xzScale` 这一步的舍入会直接传到噪声输入。
 */
struct NoiseCase {
    const char* id;
    i64 seed;
    i32 firstOctave;
    std::vector<f64> amplitudes;
    f64 xzScale;
    f64 yScale;
    f64 fromValue; ///< MappedNoise 专用
    f64 toValue;   ///< MappedNoise 专用
};

const std::vector<NoiseCase>& noiseCases()
{
    static const std::vector<NoiseCase> table = {
        // 单八度，单位缩放：只验证噪声本体在缩放为 1 时的取值
        {"single_unit", 0, -7, {1.0}, 1.0, 1.0, 0.0, 0.0},

        // 多八度，主世界洞穴用的那种缩放
        {"octaves_4", 1234567, -8, {1.0, 1.0, 1.0, 1.0}, 0.25, 0.25, 0.0, 0.0},

        // 非 2 的幂缩放：x * scale 的舍入会传到噪声输入
        {"scale_0_1", 987654321, -6, {1.0, 0.5}, 0.1, 0.1, 0.0, 0.0},

        // 主世界 density 常用的小缩放
        {"scale_small", -42, -7, {1.0, 1.0, 1.0}, 0.011, 0.011, 0.0, 0.0},

        // XZ 与 Y 缩放不同（下界常见的各向异性）
        {"anisotropic", 555, -8, {1.0, 1.0, 0.5, 0.25}, 0.5, 0.125, 0.0, 0.0},

        // MappedNoise：把噪声重映射到 [from, to]
        {"mapped_sym", 24680, -7, {1.0, 1.0}, 0.5, 0.5, -0.5, 0.5},
        {"mapped_pos", 13579, -6, {1.0, 0.5, 0.25}, 0.25, 0.25, 0.0, 1.0},
        {"mapped_asym", -100, -7, {1.0, 1.0, 1.0}, 0.31, 0.17, -2.5, 0.75},
    };
    return table;
}

/// 采样坐标：覆盖原点、各区间的正负、以及远离原点的大坐标。
const std::vector<std::array<i32, 3>>& samplePoints()
{
    static const std::vector<std::array<i32, 3>> table = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {-1, -1, -1},
        {16, 64, -16},
        {100, -32, 200},
        {-1000, 5, 1000},
        {12345, 200, -6789},
    };
    return table;
}

std::shared_ptr<const NormalNoise> buildNoise(const NoiseCase& noiseCase)
{
    math::JavaLegacyRandom rng(noiseCase.seed);
    return std::make_shared<NormalNoise>(rng, noiseCase.firstOctave, noiseCase.amplitudes);
}

std::unique_ptr<DensityFunction> buildFunction(const NoiseCase& noiseCase)
{
    const auto noise = buildNoise(noiseCase);
    if (std::string_view(noiseCase.id).starts_with("mapped_")) {
        return std::make_unique<MappedNoise>(
            noise, noiseCase.xzScale, noiseCase.yScale, noiseCase.fromValue, noiseCase.toValue);
    }
    return std::make_unique<NoiseDensity>(noise, noiseCase.xzScale, noiseCase.yScale);
}

void writeList(std::FILE* out)
{
    for (const auto& noiseCase : noiseCases()) {
        for (size_t i = 0; i < samplePoints().size(); ++i) {
            std::fprintf(out, "%s@%zu\n", noiseCase.id, i);
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

    for (const auto& noiseCase : noiseCases()) {
        if (caseId != noiseCase.id) {
            continue;
        }
        // 直接构造生产代码的密度函数，不在这里重写缩放与重映射的算术。
        const auto function = buildFunction(noiseCase);
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

MC_DIFF_DEFINE_PROBE(noise_density, &writeList, &runLine)

} // namespace mc::diff
