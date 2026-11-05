#include "probe.hpp"

#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "server/world/gen/noise/NormalNoise.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <bit>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using world::gen::noise::NormalNoise;

std::string doubleBits(f64 value)
{
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(std::bit_cast<u64>(value)));
    return buffer;
}

/**
 * @brief 一条噪声参数
 *
 * `NormalNoise` 由两个 `PerlinNoise` 叠加而成，第二个的坐标乘 1.0181268882175227
 * 错开以避免相关性，最后再乘 `0.16666666666666666 / expectedDeviation(octaveRange)`
 * 归一化。因此它比单独的 `PerlinNoise` 多出三处易错点：
 *
 * 1. `octaveRange` 只统计**非零振幅**的最小/最大下标之差，不是振幅列表长度；
 * 2. `expectedDeviation(n) = 0.1 * (1.0 + 1.0 / (n + 1))` 里的 `1.0 / (n + 1)`；
 * 3. 归一化常数 `0.16666666666666666` 是十进制字面量，写成 `1.0 / 6.0` 在编译期
 *    常量折叠后恰好同值，但若改成运行期相除就会差 1 ulp。
 *
 * 下面的参数组特意包含「中间夹零振幅」「首尾都是零振幅」「全部非零」三种形状，
 * 因为它们会让 `octaveRange` 分别等于列表长度-1、真实跨度、以及跨度。
 */
struct NoiseCase {
    const char* id;
    u64 seed;
    i32 firstOctave;
    std::vector<f64> amplitudes;
};

const std::vector<NoiseCase>& noiseCases()
{
    static const std::vector<NoiseCase> table = {
        // 单倍频：octaveRange = 0，expectedDeviation 最大
        {"single", 0ULL, 0, {1.0}},

        // 原版地形最常见的 1/2 衰减链
        {"octaves_4", 42ULL, -4, {1.0, 0.5, 0.25, 0.125}},
        {"octaves_6", 42ULL, -4, {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125}},

        // 中间夹零振幅：octaveRange 仍覆盖到首尾非零
        {"gaps", 777ULL, -2, {1.0, 0.0, 0.5, 0.0, 0.25}},

        // 首尾为零振幅：octaveRange 只覆盖中间的非零段
        {"trimmed", 777ULL, -2, {0.0, 1.0, 0.5, 0.25, 0.0}},

        // 只有一个非零振幅且不在首尾：octaveRange = 0
        {"lonely", 5ULL, 0, {0.0, 0.0, 1.5, 0.0}},

        // 非几何递减，各层量级接近
        {"flat_weights", 12345ULL, -1, {1.0, 0.9, 0.8, 0.7}},
    };
    return table;
}

/// 采样坐标：混合整数点、半整数点与任意小数。
const std::vector<std::array<f64, 3>>& samplePoints()
{
    static const std::vector<std::array<f64, 3>> table = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.5, 0.5, 0.5},
        {0.25, 0.75, 0.125},
        {-1.0, -1.0, -1.0},
        {-0.5, 0.5, -0.5},
        {12.3456789, -7.654321, 3.14159265},
        {-100.0, 50.0, 200.0},
        {1000.0, -1000.0, 500.0},
        {1.0e-3, 2.0e-3, 3.0e-3},
        {123.456, 789.012, 345.678},
        {-0.123456789, 0.987654321, -0.5},
    };
    return table;
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
    const size_t pointIndex = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (pointIndex >= samplePoints().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    for (const auto& noiseCase : noiseCases()) {
        if (caseId != noiseCase.id) {
            continue;
        }
        // 直接构造生产代码的 NormalNoise（走 IRandom 重载，对应原版
        // NormalNoise.create(RandomSource, ...)），不在这里另写一份噪声算法。
        math::JavaLegacyRandom rng(noiseCase.seed);
        const NormalNoise noise(rng, noiseCase.firstOctave, noiseCase.amplitudes);
        const auto& point = samplePoints()[pointIndex];
        const f64 value = noise.getValue(point[0], point[1], point[2]);
        return json{{"unit", requested}, {"value", doubleBits(value)}}.dump();
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(normal_noise, &writeList, &runLine)

} // namespace mc::diff
