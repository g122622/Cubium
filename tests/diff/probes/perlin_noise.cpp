#include "probe.hpp"
#include <cmath>

#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "server/world/gen/noise/PerlinNoise.hpp"
#include <array>

#include <nlohmann/json.hpp>

#include <bit>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using world::gen::noise::PerlinNoise;

std::string doubleBits(f64 value)
{
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(std::bit_cast<u64>(value)));
    return buffer;
}

/**
 * @brief 一条噪声配置
 *
 * 每个用例固定「种子 + firstOctave + 各倍频振幅」，再在一组坐标上取值。
 * 振幅表取几种真实地形里出现的形状：单倍频、两倍频递减、以及原版默认的
 * 1/2 衰减链——后者各层的贡献量级相差很大，低位的舍入差异才会暴露出来。
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
        // 单倍频：最简情形，任何实现错误都会暴露
        {"single", 0ULL, 0, {1.0}},
        {"single_neg", 12345ULL, -3, {1.0}},

        // 两倍频递减：第二层振幅是 0.5
        {"two_halves", 0ULL, 0, {1.0, 0.5}},
        {"two_halves_seed", 987654321ULL, -2, {1.5, 0.75}},

        // 原版地形默认的衰减链（1, 1/2, 1/4, ...）
        {"octaves_4", 42ULL, -4, {1.0, 0.5, 0.25, 0.125}},
        {"octaves_6", 42ULL, -4, {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125}},

        // 非几何递减：各层贡献量级接近，更容易暴露累加顺序的差异
        {"flat_weights", 777ULL, 0, {1.0, 0.9, 0.8, 0.7}},
    };
    return table;
}

/**
 * @brief 采样坐标
 *
 * 特意混合了整数点（此时噪声取到网格顶点）、半整数点、以及大量小数位组成的
 * 任意点。整数点能验证插值在端点处的行为，小数点才能真正压到三线性插值内部。
 */
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
        // 直接构造生产代码的 PerlinNoise，不在这里另写一份噪声算法。
        // 走 legacy 构造路径（对应原版 createLegacyForBlendedNoise）：
        // 两侧必须用同一种随机源，否则比的是两套种子派生而不是噪声本身。
        // 对应原版 PerlinNoise.create(rng, firstOctave, amplitudes)：
        // 它用 rng.forkPositional() 派生出各倍频的种子再 fromHashOf("octave_N")。
        // 两侧必须用同一种随机源，否则比的是两套种子派生而不是噪声本身。
        math::JavaLegacyRandom rng(noiseCase.seed);
        const math::PositionalRandomFactory factory = rng.forkPositional();
        const PerlinNoise noise(factory, noiseCase.firstOctave, noiseCase.amplitudes);
        const auto& point = samplePoints()[pointIndex];
        const f64 value = noise.getValue(point[0], point[1], point[2]);
        // 同时输出标量路径的值：两条路径若不一致，说明 SoA 向量化引入了偏差，
        // 这比「两边都错」更容易定位。
        const f64 scalarValue = noise.getValueScalar(point[0], point[1], point[2]);
        // scaled：把值放大 2^40 后取整，让低位差异变成可见的整数差，便于在报告里读。
        const i64 scaled = static_cast<i64>(std::llround(value * 1099511627776.0));
        return json{
            {"unit", requested}, {"value", doubleBits(value)}, {"scalar", doubleBits(scalarValue)}, {"scaled", scaled}}
            .dump();
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(perlin_noise, &writeList, &runLine)

} // namespace mc::diff
